/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
#ifndef USB_CONFIG_H
#define USB_CONFIG_H

#define MAX_EP_NUMBER           1
#define MAX_NUM_INT             1
#define EP0_BUFF_SIZE           8
#define USB_MAX_NUM_CONFIG_DSC  1
#define CONFIG_DESC_TOTAL_LEN   41

#define MODE_PP                 _PPBM1
#define UCFG_VAL                (_PUEN | _TRINT | _FS | MODE_PP)

#define HID_INTF_ID             0x00
#define HID_UEP                 UEP1
#define HID_BD_OUT              ep1Bo
#define HID_BD_IN               ep1Bi

#define HID_INT_OUT_EP_SIZE     64
#define HID_INT_IN_EP_SIZE      64
#define HID_NUM_OF_DSC          1
#define HID_RPT01_SIZE          29

#endif // USB_CONFIG_H