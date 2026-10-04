/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
/*
 * USB identity settings for beginners:
 * - Change the Vendor ID and Product ID values in device_dsc below.
 * - Change the manufacturer name in sd001 and the product name in sd002.
 *   The iMFR and iProduct fields in device_dsc select these string descriptors
 *   (currently 1 for sd001 and 2 for sd002).
 * - If you change a name's character count, update its string[] array size too.
 */
#include "usb.h"

ROM USB_DEV_DSC device_dsc = {
    .bLength = sizeof(USB_DEV_DSC),
    .bDscType = DSC_DEV,
    .bcdUSB = 0x0200,
    .bDevCls = 0x00,
    .bDevSubCls = 0x00,
    .bDevProtocol = 0x00,
    .bMaxPktSize0 = EP0_BUFF_SIZE,
    .idVendor = 0x04D8,
    .idProduct = 0x003C,
    .bcdDevice = 0x0100,
    .iMFR = 0x01,
    .iProduct = 0x02,
    .iSerialNum = 0x00,
    .bNumCfg = 0x01
};

ROM uint8_t CFG01[CONFIG_DESC_TOTAL_LEN] = {
    sizeof(USB_CFG_DSC), DSC_CFG,
    (uint8_t)CONFIG_DESC_TOTAL_LEN, (uint8_t)(CONFIG_DESC_TOTAL_LEN >> 8),
    1, 1, 0, _DEFAULT, 50,

    sizeof(USB_INTF_DSC), DSC_INTF,
    0, 0, 2, HID_INTF, 0, 0, 0,

    sizeof(USB_HID_DSC), DSC_HID,
    0x11, 0x01, 0x00, HID_NUM_OF_DSC, DSC_RPT,
    (uint8_t)HID_RPT01_SIZE, (uint8_t)(HID_RPT01_SIZE >> 8),

    sizeof(USB_EP_DSC), DSC_EP, _EP01_IN, _INT,
    HID_INT_IN_EP_SIZE, 0x00, 0x01,

    sizeof(USB_EP_DSC), DSC_EP, _EP01_OUT, _INT,
    HID_INT_OUT_EP_SIZE, 0x00, 0x01
};

static ROM struct {
    uint8_t bLength;
    uint8_t bDscType;
    uint16_t string[1];
} sd000 = { sizeof(sd000), DSC_STR, { 0x0409 } };

static ROM struct {
    uint8_t bLength;
    uint8_t bDscType;
    uint16_t string[7];
} sd001 = {
    sizeof(sd001), DSC_STR,
    { 'D', 'I', 'Y', ' ', 'L', 'a', 'b' }
};

static ROM struct {
    uint8_t bLength;
    uint8_t bDscType;
    uint16_t string[18];
} sd002 = {
    sizeof(sd002), DSC_STR,
    { 'P', 'I', 'C', '1', '6', 'F', '1', '4', '5', '5', ' ', 'U', 'S', 'B', ' ', 'H', 'I', 'D' }
};

ROM uint8_t hid_rpt01[HID_RPT01_SIZE] = {
    0x06, 0x00, 0xFF,
    0x09, 0x01,
    0xA1, 0x01,
    0x19, 0x01,
    0x29, 0x40,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x95, 0x40,
    0x81, 0x00,
    0x19, 0x01,
    0x29, 0x40,
    0x91, 0x00,
    0xC0
};

ROM unsigned char* ROM USB_SD_Ptr[] = {
    (ROM unsigned char *ROM)&sd000,
    (ROM unsigned char *ROM)&sd001,
    (ROM unsigned char *ROM)&sd002
};