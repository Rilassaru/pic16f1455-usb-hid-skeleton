/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
#include "usb.h"
#include "user_app.h"

/** @brief Application hook called when the USB device wakes from suspend. */
void usb_cb_wake_from_suspend(void)
{
}

/** @brief Application hook called when the USB device enters suspend. */
void usb_cb_suspend(void)
{
}

/** @brief Initialize HID and application state when a USB configuration is selected. */
void usb_cb_init_ep(uint8_t ConfigurationIndex)
{
    if (ConfigurationIndex == 1) {
        hid_init_ep();
        user_init();
    }
}

/** @brief Pass non-standard USB requests to the HID request handler. */
void usb_cb_check_other_req(void)
{
    usb_check_hid_request();
}