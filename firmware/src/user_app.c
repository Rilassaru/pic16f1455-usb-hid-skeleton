/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
#include <xc.h>
#include <string.h>
#include "usb.h"
#include "user_app.h"

#define LED_LAT             LATCbits.LATC3

// Reserve two 32-byte pages for user data in Flash, starting at address 0x1FC0.
// Together, these pages occupy the 64-byte range from 0x1FC0 through 0x1FFF.
// To expose more user Flash, increase USER_PAGE_MAX; this array reserves
// (USER_PAGE_MAX + 1) pages, each FLASH_ROW_SIZE bytes. Keep the expanded range
// within available Flash and clear of the program code.
// The USB host sends a page index, not a Flash address. The firmware converts
// that index to an address with USER_FLASH_BASE + page * FLASH_ROW_SIZE.
const uint8_t g_user_flash_data[USER_PAGE_MAX + 1][FLASH_ROW_SIZE] __at(USER_FLASH_BASE) = {
    // Page 0 (0x1FC0-0x1FDF): initial example data (32 bytes).
    {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
    },
    // Page 1 (0x1FE0-0x1FFF): initial test data (32 bytes).
    {
        0xAA, 0x55, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};

static uint8_t ReadState;
#define STATE_IDLE          0
#define STATE_BUSY          1

static UsbPacket PacketFromPC;
static UsbPacket PacketToPC;
AppStatus g_app_status = {0};

static void unlock_and_activate(void);
static void flash_read_page(uint16_t addr);
static void flash_write_page(uint16_t addr, const uint8_t *data);

/**
 * @brief Initialize application state when the USB interface is configured.
 *
 * Resets the LED state, user value, loop counter, and HID receive state.
 */
void user_init(void)
{
    ReadState = STATE_IDLE;
    g_app_status.led_state = 0;
    g_app_status.user_val = 0;
    g_app_status.loop_count = 0;
    LED_LAT = 0;
}

/**
 * @brief Receive and process one HID command from the host.
 *
 * Call this function repeatedly from the main loop. It waits until USB is
 * configured and awake, receives a report, handles its command, and sends a
 * response when the IN endpoint is available.
 */
void hid_user_interface(void)
{
    uint8_t page;
    uint16_t addr;

    if ((USBGetDeviceState() != CONFIGURED_STATE) || (USBIsDeviceSuspended() == 1)) {
        return;
    }

    // Receive a host report first, then handle it when the IN endpoint is ready.
    if (ReadState == STATE_IDLE) {
        if (!mHIDRxIsBusy()) {
            HIDRxReport((char *)&PacketFromPC, USB_PACKET_SIZE);
            ReadState = STATE_BUSY;
            // Clear the response first so unused bytes cannot contain old data.
            memset(PacketToPC.Contents, 0, USB_PACKET_SIZE);
        }
    } else {
        if (!mHIDTxIsBusy()) {
            // Add a new case here to define another PC-to-PIC command:
            // 1. Give it a command value in user_app.h.
            // 2. Read any request parameters from PacketFromPC.
            // 3. Put the result in PacketToPC and send it with hid_tx_report().
            switch (PacketFromPC.Command) {
                case CMD_GET_STATUS:
                    // Response bytes 1-2 are LED and user values; bytes 3-6 hold the counter.
                    // The 32-bit counter is sent most-significant byte first.
                    PacketToPC.Contents[0] = RET_SUCCESS;
                    PacketToPC.Contents[1] = g_app_status.led_state;
                    PacketToPC.Contents[2] = g_app_status.user_val;
                    PacketToPC.Contents[3] = (uint8_t)(g_app_status.loop_count >> 24);
                    PacketToPC.Contents[4] = (uint8_t)(g_app_status.loop_count >> 16);
                    PacketToPC.Contents[5] = (uint8_t)(g_app_status.loop_count >> 8);
                    PacketToPC.Contents[6] = (uint8_t)(g_app_status.loop_count);

                    hid_tx_report((char *)&PacketToPC, USB_PACKET_SIZE);
                    ReadState = STATE_IDLE;
                    break;

                case CMD_SET_STATUS:
                    // Request bytes 1 and 2 contain the LED command and new user value.
                    switch (PacketFromPC.Contents[1]) {
                        case LED_CMD_OFF:
                            LED_LAT = 0;
                            g_app_status.led_state = 0;
                            break;
                        case LED_CMD_ON:
                            LED_LAT = 1;
                            g_app_status.led_state = 1;
                            break;
                        case LED_CMD_TOGGLE:
                            LED_LAT = !LED_LAT;
                            g_app_status.led_state = LED_LAT;
                            break;
                        default:
                            break;
                    }

                    g_app_status.user_val = PacketFromPC.Contents[2];

                    PacketToPC.Contents[0] = RET_SUCCESS;
                    PacketToPC.Contents[1] = g_app_status.led_state;
                    PacketToPC.Contents[2] = g_app_status.user_val;

                    hid_tx_report((char *)&PacketToPC, USB_PACKET_SIZE);
                    ReadState = STATE_IDLE;
                    break;

                case CMD_GET_DATA:
                    // Byte 1 selects a Flash page; reject indexes outside the reserved range.
                    page = PacketFromPC.Contents[1];
                    if (page > USER_PAGE_MAX) {
                        PacketToPC.Contents[0] = RET_FAIL;
                        hid_tx_report((char *)&PacketToPC, USB_PACKET_SIZE);
                    } else {
                        addr = USER_FLASH_BASE + (page * FLASH_ROW_SIZE);
                        flash_read_page(addr);
                        PacketToPC.Contents[1] = page;
                        hid_tx_report((char *)&PacketToPC, USB_PACKET_SIZE);
                    }
                    ReadState = STATE_IDLE;
                    break;

                case CMD_SET_DATA:
                    // Data starts at byte 4: byte 0 is the command, followed by page and reserved bytes.
                    page = PacketFromPC.Contents[1];
                    if (page > USER_PAGE_MAX) {
                        PacketToPC.Contents[0] = RET_FAIL;
                        hid_tx_report((char *)&PacketToPC, USB_PACKET_SIZE);
                    } else {
                        addr = USER_FLASH_BASE + (page * FLASH_ROW_SIZE);
                        flash_write_page(addr, &PacketFromPC.Contents[4]);
                        flash_read_page(addr);
                        PacketToPC.Contents[1] = page;
                        hid_tx_report((char *)&PacketToPC, USB_PACKET_SIZE);
                    }
                    ReadState = STATE_IDLE;
                    break;

                default:
                    PacketToPC.Contents[0] = RET_FAIL;
                    hid_tx_report((char *)&PacketToPC, USB_PACKET_SIZE);
                    ReadState = STATE_IDLE;
                    break;
            }
        }
    }
}

/**
 * @brief Run the PIC-required unlock sequence to start a Flash operation.
 *
 * The 0x55 and 0xAA keys must be written to PMCON2 before setting the WR bit.
 */
static void unlock_and_activate(void)
{
    PMCON1bits.WREN = 1;
    PMCON2 = 0x55;
    PMCON2 = 0xAA;
    PMCON1bits.WR = 1;
    NOP();
    NOP();
    PMCON1bits.WREN = 0;
}

/**
 * @brief Read one 32-byte Flash row into the outgoing HID response.
 *
 * The response stores the starting address in bytes 2-3 and the row data in
 * bytes 4-35. Interrupts are disabled while the Flash read registers are used.
 *
 * @param addr Starting program-memory address of the row to read.
 */
static void flash_read_page(uint16_t addr)
{
    uint8_t i;
    bool gie_state = INTCONbits.GIE;
    INTCONbits.GIE = 0;

    PMADR = addr;
    PacketToPC.Contents[0] = RET_SUCCESS;
    PacketToPC.Contents[2] = (uint8_t)(addr >> 8);
    PacketToPC.Contents[3] = (uint8_t)(addr & 0xFF);

    for (i = 0; i < FLASH_ROW_SIZE; i++) {
        PMCON1bits.CFGS = 0;
        PMCON1bits.RD = 1;
        NOP();
        NOP();
        PacketToPC.Contents[i + 4] = PMDATL;
        PMADR++;
    }

    if (gie_state) {
        INTCONbits.GIE = 1;
    }
}

/**
 * @brief Erase and program one 32-byte Flash row.
 *
 * Flash must be erased before new data is programmed. The data bytes are
 * loaded into the write latches in order; the final byte starts programming.
 * The caller must provide at least FLASH_ROW_SIZE bytes.
 *
 * @param addr Starting program-memory address of the row to write.
 * @param data Pointer to the 32 bytes to write.
 */
static void flash_write_page(uint16_t addr, const uint8_t *data)
{
    uint8_t i;
    bool gie_state = INTCONbits.GIE;
    // Keep Flash programming uninterrupted, then restore the previous interrupt state.
    INTCONbits.GIE = 0;

    // 1. Erase the 32-byte Flash row before programming new data.
    PMADR = addr;
    PMCON1bits.CFGS = 0;
    PMCON1bits.FREE = 1;
    unlock_and_activate();

    // 2. Load each byte into the write latches; the final byte starts programming.
    PMCON1bits.CFGS = 0;
    PMCON1bits.FREE = 0;

    for (i = 0; i < FLASH_ROW_SIZE; i++) {
        if (i == (FLASH_ROW_SIZE - 1)) {
            PMCON1bits.LWLO = 0; // Clear LWLO on the last byte to start the Flash write.
        } else {
            PMCON1bits.LWLO = 1; // Keep loading bytes without starting the write yet.
        }
        PMDATL = data[i];
        PMDATH = 0x34;          // Store the data byte in a RETLW instruction.
        unlock_and_activate();
        PMADR++;
    }

    if (gie_state) {
        INTCONbits.GIE = 1;
    }
}