/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
#include "usb.h"

static uint8_t idle_rate;
static uint8_t active_protocol;
uint8_t hid_rpt_rx_len;

void hid_get_report_handler(void);
void hid_set_report_handler(void);

/** @brief Handle HID class requests and provide the HID descriptors. */
void usb_check_hid_request(void)
{
    if (SetupPkt.Recipient != RCPT_INTF) return;
    if (SetupPkt.bIntfID != HID_INTF_ID) return;

    if (SetupPkt.bRequest == GET_DSC) {
        switch (SetupPkt.bDscType) {
            case DSC_HID:
                ctrl_trf_session_owner = MUID_HID;
                pSrc.bRom = &CFG01[18];
                wCount.Val = sizeof(USB_HID_DSC);
                break;
            case DSC_RPT:
                ctrl_trf_session_owner = MUID_HID;
                mUSBGetHIDRptDscAdr(pSrc.bRom);
                mUSBGetHIDRptDscSize(wCount.Val);
                break;
            default:
                break;
        }
        usb_stat.ctrl_trf_mem = _ROM;
    }

    if (SetupPkt.RequestType != CLASS) return;

    switch (SetupPkt.bRequest) {
        case GET_REPORT:
            hid_get_report_handler();
            break;
        case SET_REPORT:
            hid_set_report_handler();
            break;
        case GET_IDLE:
            ctrl_trf_session_owner = MUID_HID;
            pSrc.bRam = (uint8_t*)&idle_rate;
            usb_stat.ctrl_trf_mem = _RAM;
            wCount.v[0] = 1;
            break;
        case SET_IDLE:
            ctrl_trf_session_owner = MUID_HID;
            idle_rate = (uint8_t)(SetupPkt.wValue >> 8);
            break;
        case GET_PROTOCOL:
            ctrl_trf_session_owner = MUID_HID;
            pSrc.bRam = (uint8_t*)&active_protocol;
            usb_stat.ctrl_trf_mem = _RAM;
            wCount.v[0] = 1;
            break;
        case SET_PROTOCOL:
            ctrl_trf_session_owner = MUID_HID;
            active_protocol = (uint8_t)(SetupPkt.wValue & 0xFF);
            break;
        default:
            break;
    }
}

/** @brief Placeholder for handling an optional HID Get Report request. */
void hid_get_report_handler(void)
{
}

/** @brief Placeholder for handling an optional HID Set Report request. */
void hid_set_report_handler(void)
{
}

/** @brief Configure the HID interrupt endpoints and their report buffers. */
void hid_init_ep(void)
{
    hid_rpt_rx_len = 0;

    HID_UEP = EP_OUT_IN | HSHK_EN;

    HID_BD_OUT.Cnt = sizeof(hid_report_out);
    HID_BD_OUT.ADR = (uint8_t*)&hid_report_out;
    HID_BD_OUT.Stat._byte = _DAT0 | _DTSEN;
    HID_BD_OUT.Stat._byte |= _USIE;

    HID_BD_IN.ADR = (uint8_t*)&hid_report_in;
    HID_BD_IN.Stat._byte = _UCPU | _DAT1;
}

/** @brief Copy a report to the HID IN buffer and start sending it to the host. */
void hid_tx_report(char *buffer, uint8_t len)
{
    uint8_t i;

    if (len > HID_INT_IN_EP_SIZE) {
        len = HID_INT_IN_EP_SIZE;
    }

    for (i = 0; i < len; i++) {
        hid_report_in[i] = buffer[i];
    }

    HID_BD_IN.Cnt = len;
    mUSBBufferReady(HID_BD_IN);
}

/** @brief Copy a received HID report to the application buffer and return its length. */
uint8_t HIDRxReport(char *buffer, uint8_t len)
{
    hid_rpt_rx_len = 0;

    if (!mHIDRxIsBusy()) {
        if (len > HID_BD_OUT.Cnt) {
            len = HID_BD_OUT.Cnt;
        }

        for (hid_rpt_rx_len = 0; hid_rpt_rx_len < len; hid_rpt_rx_len++) {
            buffer[hid_rpt_rx_len] = hid_report_out[hid_rpt_rx_len];
        }

        HID_BD_OUT.Cnt = sizeof(hid_report_out);
        mUSBBufferReady(HID_BD_OUT);
    }

    return hid_rpt_rx_len;
}