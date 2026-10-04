/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
#include "usb.h"

void usb_check_std_request(void);
void usb_suspend(void);
void usb_protocol_reset_handler(void);
void usb_wake_from_suspend(void);
void usb_std_get_dsc_handler(void);
void usb_std_set_cfg_handler(void);
void usb_std_get_status_handler(void);
void usb_std_feature_req_handler(void);
void usb_ctrl_trf_setup_handler(void);
void usb_ctrl_trf_in_handler(void);
void usb_ctrl_trf_tx_service(void);
void usb_ctrl_ep_service_complete(void);
void load_bdt_and_set_uown(uint8_t BDTIndexToLoad);

#if !defined(ENABLE_CONTROL_TRANSFERS_WITH_OUT_DATA_STAGE)
#define usb_ctrl_trf_out_handler(a)
#endif

uint8_t bTRNIFCount;
uint8_t ctrl_trf_state;
uint8_t ctrl_trf_session_owner;
POINTER pSrc;
POINTER pDst;
WORD_VAL wCount;
uint8_t short_pkt_status;
CTRL_TRF_SETUP SetupPkt;
bool EP0OutOddNeedsArmingNext;
BDT TempBDT;
uint8_t usb_device_state;
USB_DEVICE_STATUS usb_stat;
uint8_t usb_active_cfg;
uint8_t usb_alt_intf[MAX_NUM_INT];

uint8_t USTATSave;
bool DeviceIsSoftDetached;

#define BDT_ADDR                0x20
#define USB_RAM_BUFF_ADDR       (BDT_ADDR + 12 + (MAX_EP_NUMBER * 8))

#define BDT_ADDR_TAG_EP0O_EVEN   (BDT_ADDR)
#define BDT_ADDR_TAG_EP0O_ODD    (BDT_ADDR + 4)
#define BDT_ADDR_TAG_EP0I        (BDT_ADDR + 8)
#define BDT_ADDR_TAG_EP1O        (BDT_ADDR + 12)
#define BDT_ADDR_TAG_EP1I        (BDT_ADDR + 16)

#define USB_EP0_BUFF_ADDR        (USB_RAM_BUFF_ADDR)
#define USB_EP0_BUFF_ADDR2       (USB_EP0_BUFF_ADDR + EP0_BUFF_SIZE)
#define USB_CTRL_TRF_DATA_ADDR   (USB_EP0_BUFF_ADDR2 + EP0_BUFF_SIZE)
#define USB_HID_BUFF_OUT_ADDR    (0xA0)
#define USB_HID_BUFF_IN_ADDR     (0x120)

volatile BDT ep0BoEven __at(BDT_ADDR_TAG_EP0O_EVEN);
volatile BDT ep0BoOdd  __at(BDT_ADDR_TAG_EP0O_ODD);
volatile BDT ep0Bi     __at(BDT_ADDR_TAG_EP0I);
volatile BDT ep1Bo     __at(BDT_ADDR_TAG_EP1O);
volatile BDT ep1Bi     __at(BDT_ADDR_TAG_EP1I);

volatile uint8_t EP0OutEvenBuf[EP0_BUFF_SIZE] __at(USB_EP0_BUFF_ADDR);
volatile uint8_t EP0OutOddBuf[EP0_BUFF_SIZE]  __at(USB_EP0_BUFF_ADDR2);
volatile CTRL_TRF_DATA CtrlTrfData            __at(USB_CTRL_TRF_DATA_ADDR);

volatile unsigned char hid_report_out[HID_INT_OUT_EP_SIZE] __at(USB_HID_BUFF_OUT_ADDR);
volatile unsigned char hid_report_in[HID_INT_IN_EP_SIZE]   __at(USB_HID_BUFF_IN_ADDR);

#if !defined(USE_USB_BUS_SENSE_IO)
    #define usb_bus_sense       1
#endif

#if !defined(USE_SELF_POWER_SENSE_IO)
    #define self_power          0
#endif

/** @brief Start USB device support and check whether the bus can be attached. */
void usb_device_init(void)
{
    if (UCONbits.USBEN == 1) {
        usb_disable_with_long_delay();
    }
    DeviceIsSoftDetached = false;
    usb_check_bus_status();
}

/** @brief Enable the USB module and attach the device to the USB bus. */
void usb_soft_attach(void)
{
    if (DeviceIsSoftDetached == true) {
        usb_disable_with_long_delay();
    }

    UCON = 0;
    UCFG = UCFG_VAL;
    UIE = 0;
    UCONbits.USBEN = 1;

    usb_protocol_reset_handler();
    usb_device_state = ATTACHED_STATE;
    DeviceIsSoftDetached = false;
}

/** @brief Disable the USB module and mark the device as detached. */
void usb_soft_detach(void)
{
    UCONbits.SUSPND = 0;
    UCON = 0x00;
    usb_device_state = DETACHED_STATE;
    DeviceIsSoftDetached = true;
}

/** @brief Attach to the bus when USB is enabled and not intentionally detached. */
void usb_check_bus_status(void)
{
    if (DeviceIsSoftDetached == true) {
        return;
    }

    if (UCONbits.USBEN == 0) {
        usb_soft_attach();
    }
}

/** @brief Poll USB events and service pending control-endpoint transactions. */
void usb_device_tasks(void)
{
    static volatile BDT* pBDTEntry;
    static uint8_t i;

    usb_check_bus_status();

    if (usb_device_state == DETACHED_STATE) {
        return;
    }

    if (UIRbits.ACTVIF) usb_wake_from_suspend();

    if (UCONbits.SUSPND == 1) {
        return;
    }
    if (UIRbits.URSTIF) usb_protocol_reset_handler();

    if (usb_device_state < DEFAULT_STATE) return;

    for (bTRNIFCount = 0; bTRNIFCount < 4; bTRNIFCount++) {
        if (UIRbits.TRNIF) {
            USTATSave = USTAT;
            if ((USTAT & 0x7C) == EP00_OUT) {
                if (USTATbits.PPBI == 0) {
                    pBDTEntry = &ep0BoEven;
                } else {
                    pBDTEntry = &ep0BoOdd;
                }

                UIRbits.TRNIF = 0;

                if (pBDTEntry->Stat.PID == SETUP_TOKEN) {
                    for (i = 0; i < sizeof(CTRL_TRF_SETUP); i++) {
                        SetupPkt._byte[i] = *pBDTEntry->ADR++;
                    }
                    usb_ctrl_trf_setup_handler();
                } else {
                    usb_ctrl_trf_out_handler(USTATSave);
                }
            } else if (USTAT == EP00_IN) {
                UIRbits.TRNIF = 0;
                usb_ctrl_trf_in_handler();
            } else {
                UIRbits.TRNIF = 0;
            }
        } else {
            break;
        }
    }
}

/** @brief Enter USB suspend mode and notify the application callback. */
void usb_suspend(void)
{
    static unsigned char UIESave;

    UIESave = UIE;
    UIE = 0b00000100;
    UIRbits.IDLEIF = 0;
    UCONbits.SUSPND = 1;

    USBIF_FLAG = 0;
    USBIE_BIT = 1;

    usb_cb_suspend();

    USBIE_BIT = 0;
    UIE |= UIESave;
}

/** @brief Resume USB operation and clear the activity event. */
void usb_wake_from_suspend(void)
{
    usb_cb_wake_from_suspend();

    UCONbits.SUSPND = 0;
    UIEbits.ACTVIE = 0;
    while (UIRbits.ACTVIF) {
        UIRbits.ACTVIF = 0;
    }
}

/** @brief Reset USB device state and endpoint control data after a bus reset. */
void usb_protocol_reset_handler(void)
{
    usb_device_state = DEFAULT_STATE;
    UEIE = 0;
    UIR = 0;
    UIE = 0b01111011;
    UADDR = 0x00;
    mDisableEP1to7();
    UEP0 = EP_CTRL | HSHK_EN;
    UCONbits.PPBRST = 1;
    while (UIRbits.TRNIF == 1) {
        UIRbits.TRNIF = 0;
        CLRWDT();
    }
    UCONbits.PPBRST = 0;
    UCONbits.PKTDIS = 0;

    TempBDT.Stat._byte = _DAT0 | _BSTALL;
    load_bdt_and_set_uown(EP0_OUT_EVEN_BDT_INDEX);
    EP0OutOddNeedsArmingNext = true;
    usb_stat._byte = 0x00;
    usb_active_cfg = 0;
    usb_cb_init_ep(0);
}

/** @brief Prepare endpoint 0 and dispatch a newly received SETUP request. */
void usb_ctrl_trf_setup_handler(void)
{
    ep0Bi.Stat._byte = _UCPU;
    short_pkt_status = SHORT_PKT_NOT_SENT;

    if (ep0BoEven.Stat.UOWN == 1) {
        ep0BoEven.Stat._byte = _UCPU;
        EP0OutOddNeedsArmingNext = false;
    }
    if (ep0BoOdd.Stat.UOWN == 1) {
        ep0BoOdd.Stat._byte = _UCPU;
        EP0OutOddNeedsArmingNext = true;
    }
    ctrl_trf_state = WAIT_SETUP;
    ctrl_trf_session_owner = MUID_NULL;
    wCount.Val = 0;
    UCONbits.PKTDIS = 0;

    usb_check_std_request();
    usb_cb_check_other_req();

    usb_ctrl_ep_service_complete();
}

/** @brief Process an endpoint 0 IN event and continue a control transfer. */
void usb_ctrl_trf_in_handler(void)
{
    if (usb_device_state == ADR_PENDING_STATE) {
        UADDR = SetupPkt.bDevADR;
        if (UADDR > 0)
            usb_device_state = ADDRESS_STATE;
        else
            usb_device_state = DEFAULT_STATE;
    }

    if (ctrl_trf_state == CTRL_TRF_TX) {
        usb_ctrl_trf_tx_service();

        if (short_pkt_status == SHORT_PKT_SENT) {
            ep0Bi.Stat._byte = _BSTALL;
            ep0Bi.Stat._byte |= _USIE;
        } else {
            if (ep0Bi.Stat.DTS == 0)
                ep0Bi.Stat._byte = _DAT1 | _DTSEN;
            else
                ep0Bi.Stat._byte = _DAT0 | _DTSEN;

            ep0Bi.Stat._byte |= _USIE;
        }
    }
}

/** @brief Copy the next control-transfer data chunk into the USB transmit buffer. */
void usb_ctrl_trf_tx_service(void)
{
    static uint8_t bytes_to_send;

    bytes_to_send = EP0_BUFF_SIZE;
    if (wCount.Val < EP0_BUFF_SIZE) {
        bytes_to_send = (uint8_t)wCount.Val;
        if (short_pkt_status == SHORT_PKT_NOT_SENT) {
            short_pkt_status = SHORT_PKT_PENDING;
        } else if (short_pkt_status == SHORT_PKT_PENDING) {
            short_pkt_status = SHORT_PKT_SENT;
        }
    }

    ep0Bi.Cnt = bytes_to_send;
    wCount.Val -= bytes_to_send;

    pDst.bRam = (uint8_t*)&CtrlTrfData;
    if (usb_stat.ctrl_trf_mem == _ROM) {
        while (bytes_to_send) {
            *pDst.bRam = *pSrc.bRom;
            pDst.bRam++;
            pSrc.bRom++;
            bytes_to_send--;
        }
    } else {
        while (bytes_to_send) {
            *pDst.bRam = *pSrc.bRam;
            pDst.bRam++;
            pSrc.bRam++;
            bytes_to_send--;
        }
    }
}

/** @brief Set up endpoint 0 after a control request has been examined. */
void usb_ctrl_ep_service_complete(void)
{
    if (ctrl_trf_session_owner == MUID_NULL) {
        ep0Bi.Stat._byte = _BSTALL;
        ep0Bi.Stat._byte |= _USIE;
        TempBDT.Stat._byte = _BSTALL;
        if (EP0OutOddNeedsArmingNext == true) {
            load_bdt_and_set_uown(EP0_OUT_ODD_BDT_INDEX);
            EP0OutOddNeedsArmingNext = false;
        } else {
            load_bdt_and_set_uown(EP0_OUT_EVEN_BDT_INDEX);
            EP0OutOddNeedsArmingNext = true;
        }
    } else {
        if (SetupPkt.DataDir == DEV_TO_HOST) {
            ctrl_trf_state = CTRL_TRF_TX;

            if (SetupPkt.wLength < wCount.Val)
                wCount.Val = SetupPkt.wLength;

            usb_ctrl_trf_tx_service();

            TempBDT.Stat._byte = _DAT1 | _DTSEN;
            load_bdt_and_set_uown(EP0_OUT_ODD_BDT_INDEX);
            load_bdt_and_set_uown(EP0_OUT_EVEN_BDT_INDEX);

            ep0Bi.ADR = (uint8_t*)&CtrlTrfData;
            ep0Bi.Stat._byte = _DAT1 | _DTSEN;
            ep0Bi.Stat._byte |= _USIE;
        } else {
            ctrl_trf_state = CTRL_TRF_RX;
            TempBDT.Stat._byte = _BSTALL;
            if (SetupPkt.wLength == 0) {
                TempBDT.Stat._byte = _DAT1 | _DTSEN;
            }
            if (EP0OutOddNeedsArmingNext == true) {
                load_bdt_and_set_uown(EP0_OUT_ODD_BDT_INDEX);
                EP0OutOddNeedsArmingNext = false;
            } else {
                load_bdt_and_set_uown(EP0_OUT_EVEN_BDT_INDEX);
                EP0OutOddNeedsArmingNext = true;
            }

            if (SetupPkt.wLength == 0) {
                ep0Bi.Cnt = 0;
                ep0Bi.Stat._byte = _DAT1 | _DTSEN;
                ep0Bi.Stat._byte |= _USIE;
            }
        }
    }
}

/** @brief Dispatch a standard USB request to its matching handler. */
void usb_check_std_request(void)
{
    if (SetupPkt.RequestType != STANDARD) return;

    switch (SetupPkt.bRequest) {
        case SET_ADR:
            ctrl_trf_session_owner = MUID_USB9;
            usb_device_state = ADR_PENDING_STATE;
            break;
        case GET_DSC:
            usb_std_get_dsc_handler();
            break;
        case SET_CFG:
            usb_std_set_cfg_handler();
            break;
        case GET_CFG:
            ctrl_trf_session_owner = MUID_USB9;
            pSrc.bRam = (uint8_t*)&usb_active_cfg;
            usb_stat.ctrl_trf_mem = _RAM;
            wCount.v[0] = 1;
            break;
        case GET_STATUS:
            usb_std_get_status_handler();
            break;
        case CLR_FEATURE:
        case SET_FEATURE:
            usb_std_feature_req_handler();
            break;
        case GET_INTF:
            ctrl_trf_session_owner = MUID_USB9;
            pSrc.bRam = (uint8_t*)&usb_alt_intf + SetupPkt.bIntfID;
            usb_stat.ctrl_trf_mem = _RAM;
            wCount.v[0] = 1;
            break;
        case SET_INTF:
            ctrl_trf_session_owner = MUID_USB9;
            usb_alt_intf[SetupPkt.bIntfID] = SetupPkt.bAltID;
            break;
        default:
            break;
    }
}

/** @brief Select a device, configuration, or string descriptor for the host. */
void usb_std_get_dsc_handler(void)
{
    if (SetupPkt.bmRequestType == 0x80) {
        switch (SetupPkt.bDscType) {
            case DSC_DEV:
                ctrl_trf_session_owner = MUID_USB9;
                pSrc.bRom = (ROM uint8_t*)&device_dsc;
                wCount.v[0] = sizeof(device_dsc);
                break;
            case DSC_CFG:
                if (SetupPkt.bDscIndex < USB_MAX_NUM_CONFIG_DSC) {
                    ctrl_trf_session_owner = MUID_USB9;
                    pSrc.bRom = (ROM BYTE*)&CFG01;
                    wCount.Val = sizeof(CFG01);
                }
                break;
            case DSC_STR:
                ctrl_trf_session_owner = MUID_USB9;
                pSrc.bRom = *(USB_SD_Ptr + SetupPkt.bDscIndex);
                wCount.Val = *pSrc.bRom;
                break;
        }
        usb_stat.ctrl_trf_mem = _ROM;
    }
}

/** @brief Apply the host-selected USB configuration and update device state. */
void usb_std_set_cfg_handler(void)
{
    static unsigned char i;

    ctrl_trf_session_owner = MUID_USB9;
    mDisableEP1to7();
    for (i = 0; i < MAX_NUM_INT; i++) {
        usb_alt_intf[i] = 0;
    }

    usb_active_cfg = SetupPkt.bCfgValue;
    usb_cb_init_ep(usb_active_cfg);

    if (SetupPkt.bCfgValue == 0) {
        usb_device_state = ADDRESS_STATE;
    } else {
        usb_device_state = CONFIGURED_STATE;
    }
}

/** @brief Build the status response for the requested device, interface, or endpoint. */
void usb_std_get_status_handler(void)
{
    CtrlTrfData._byte0 = 0;
    CtrlTrfData._byte1 = 0;

    switch (SetupPkt.Recipient) {
        case RCPT_DEV:
            ctrl_trf_session_owner = MUID_USB9;
            if (self_power == 1)
                CtrlTrfData._byte0 |= 0b00000001;
            if (usb_stat.RemoteWakeup == 1)
                CtrlTrfData._byte0 |= 0b00000010;
            break;
        case RCPT_INTF:
            ctrl_trf_session_owner = MUID_USB9;
            break;
        case RCPT_EP:
            ctrl_trf_session_owner = MUID_USB9;
            pDst.bRam = (uint8_t*)&ep0BoEven + (SetupPkt.EPNum * 8) + (SetupPkt.EPDir * 4) + 4;
            if (*pDst.bRam & _BSTALL)
                CtrlTrfData._byte0 = 0x01;
            break;
    }

    if (ctrl_trf_session_owner == MUID_USB9) {
        pSrc.bRam = (uint8_t*)&CtrlTrfData;
        usb_stat.ctrl_trf_mem = _RAM;
        wCount.v[0] = 2;
    }
}

/** @brief Set or clear remote-wakeup and endpoint-halt features. */
void usb_std_feature_req_handler(void)
{
    if ((SetupPkt.bFeature == DEVICE_REMOTE_WAKEUP) && (SetupPkt.Recipient == RCPT_DEV)) {
        ctrl_trf_session_owner = MUID_USB9;
        if (SetupPkt.bRequest == SET_FEATURE)
            usb_stat.RemoteWakeup = 1;
        else
            usb_stat.RemoteWakeup = 0;
    }

    if ((SetupPkt.bFeature == ENDPOINT_HALT) && (SetupPkt.Recipient == RCPT_EP) && (SetupPkt.EPNum != 0)) {
        ctrl_trf_session_owner = MUID_USB9;
        pDst.bRam = (uint8_t*)&ep0BoEven + (SetupPkt.EPNum * 8) + (SetupPkt.EPDir * 4) + 4;

        if (SetupPkt.bRequest == SET_FEATURE) {
            *pDst.bRam = _BSTALL;
            *pDst.bRam |= _USIE;
        } else {
            if (SetupPkt.EPDir == 1)
                *pDst.bRam = _UCPU | _DAT1;
            else {
                *pDst.bRam = _DAT0 | _DTSEN;
                *pDst.bRam |= _USIE;
            }
        }
    }
}

/** @brief Load an endpoint 0 buffer descriptor and give its buffer to USB hardware. */
void load_bdt_and_set_uown(uint8_t BDTIndexToLoad)
{
    static volatile BDT* pBDTEntry;

    TempBDT.Cnt = EP0_BUFF_SIZE;
    TempBDT.ADR = (uint8_t*)&EP0OutOddBuf[0];
    if (BDTIndexToLoad == EP0_OUT_EVEN_BDT_INDEX) {
        TempBDT.ADR = (uint8_t*)&EP0OutEvenBuf[0];
        pBDTEntry = (volatile BDT*)BDT_ADDR;
    } else {
        pBDTEntry = (volatile BDT*)(BDT_ADDR + 4);
    }

    *pBDTEntry = TempBDT;
    pBDTEntry->Stat.UOWN = 1;
}

/** @brief Disable USB, wait briefly, and mark the device as detached. */
void usb_disable_with_long_delay(void)
{
    UCONbits.SUSPND = 0;
    UCON = 0x00;
    delay_routine(0xFFFF);
    usb_device_state = DETACHED_STATE;
}

/** @brief Wait for the requested loop count while periodically clearing the watchdog. */
void delay_routine(unsigned int DelayAmount)
{
    while (DelayAmount) {
        CLRWDT();
        DelayAmount--;
    }
}