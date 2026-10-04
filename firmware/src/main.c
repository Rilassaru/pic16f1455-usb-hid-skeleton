/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
#include <xc.h>
#include "usb.h"
#include "user_app.h"

#pragma config FOSC = INTOSC
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = OFF
#pragma config CP = OFF
#pragma config BOREN = ON
#pragma config CLKOUTEN = OFF
#pragma config IESO = OFF
#pragma config FCMEN = OFF

#pragma config WRT = OFF
#pragma config CPUDIV = NOCLKDIV
#pragma config USBLSCLK = 48MHz
#pragma config PLLMULT = 3x
#pragma config PLLEN = ENABLED
#pragma config STVREN = ON
#pragma config BORV = LO
#pragma config LPBOR = OFF
#pragma config LVP = ON

/** @brief Configure the clock, digital I/O, and LED output pin. */
static void sys_init(void)
{
    OSCCON = 0b11111100;
    ACTCON = 0b10010000;

    ANSELA = 0x00;
    ANSELC = 0x00;

    TRISCbits.TRISC3 = 0;
    LATCbits.LATC3 = 0;
}

/** @brief Initialize the PIC and repeatedly service USB and application tasks. */
void main(void)
{
    sys_init();
    usb_device_init();

    while (1) {
        usb_device_tasks();
        hid_user_interface();
        g_app_status.loop_count++;
    }
}