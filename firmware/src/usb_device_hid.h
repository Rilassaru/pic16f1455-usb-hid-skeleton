/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
#ifndef HID_H
#define HID_H

#include <stdint.h>
#include <stdbool.h>

#define GET_REPORT              0x01
#define GET_IDLE                0x02
#define GET_PROTOCOL            0x03
#define SET_REPORT              0x09
#define SET_IDLE                0x0A
#define SET_PROTOCOL            0x0B

#define DSC_HID                 0x21
#define DSC_RPT                 0x22
#define DSC_PHY                 0x23

#define BOOT_PROTOCOL           0x00
#define RPT_PROTOCOL            0x01

#define HID_INTF                0x03
#define BOOT_INTF_SUBCLASS      0x01

#define mHIDRxIsBusy()          HID_BD_OUT.Stat.UOWN
#define mHIDTxIsBusy()          HID_BD_IN.Stat.UOWN
#define mHIDGetRptRxLength()    hid_rpt_rx_len

#define mUSBGetHIDRptDscAdr(ptr) \
{ \
    if (usb_active_cfg == 1) \
        ptr = (ROM uint8_t*)&hid_rpt01; \
}

#define mUSBGetHIDRptDscSize(count) \
{ \
    if (usb_active_cfg == 1) \
        count = sizeof(hid_rpt01); \
}

typedef struct _USB_HID_DSC_HEADER
{
    uint8_t  bDscType;
    uint16_t wDscLength;
} USB_HID_DSC_HEADER;

typedef struct _USB_HID_DSC
{
    uint8_t  bLength;
    uint8_t  bDscType;
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDsc;
    USB_HID_DSC_HEADER hid_dsc_header[HID_NUM_OF_DSC];
} USB_HID_DSC;

extern uint8_t hid_rpt_rx_len;
extern ROM uint8_t hid_rpt01[HID_RPT01_SIZE];

void hid_init_ep(void);
void usb_check_hid_request(void);
void hid_tx_report(char *buffer, uint8_t len);
uint8_t HIDRxReport(char *buffer, uint8_t len);

#endif // HID_H