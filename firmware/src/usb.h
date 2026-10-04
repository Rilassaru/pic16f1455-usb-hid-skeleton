/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
#ifndef USB_H
#define USB_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef ClrWdt
#define ClrWdt() CLRWDT()
#endif

#ifndef ROM
#define ROM const
#endif

#ifndef BYTE
typedef uint8_t BYTE;
#endif

#ifndef WORD
typedef uint16_t WORD;
#endif

typedef union _WORD_VAL
{
    uint16_t Val;
    uint8_t v[2];
    struct
    {
        uint8_t LB;
        uint8_t HB;
    } byte;
} WORD_VAL;

#include "usb_config.h"
#include "usb_device_hid.h"
#include "usb_device.h"

void usb_cb_wake_from_suspend(void);
void usb_cb_suspend(void);
void usb_cb_init_ep(uint8_t ConfigurationIndex);
void usb_cb_check_other_req(void);

#endif // USB_H