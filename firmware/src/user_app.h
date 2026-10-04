/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
#ifndef USER_APP_H
#define USER_APP_H

#include <stdint.h>
#include <stdbool.h>

#define USB_PACKET_SIZE     64

#define CMD_GET_STATUS      0x10
#define CMD_SET_STATUS      0x11
#define CMD_GET_DATA        0x20
#define CMD_SET_DATA        0x21

#define RET_SUCCESS         0x00
#define RET_FAIL            0xFF

#define LED_CMD_OFF         0x00
#define LED_CMD_ON          0x01
#define LED_CMD_TOGGLE      0x02

// Reserve two 32-byte Flash pages for application data.
// Page 0 uses 0x1FC0-0x1FDF; page 1 uses 0x1FE0-0x1FFF.
#define FLASH_ROW_SIZE      32
#define USER_FLASH_BASE     0x1FC0
#define USER_PAGE_MAX       1

// The command and payload share the same 64-byte HID report buffer.
typedef union {
    uint8_t Command;
    uint8_t Contents[USB_PACKET_SIZE];
} UsbPacket;

typedef struct {
    uint8_t  led_state;
    uint8_t  user_val;
    uint32_t loop_count;
} AppStatus;

extern AppStatus g_app_status;

void user_init(void);
void hid_user_interface(void);

#endif // USER_APP_H