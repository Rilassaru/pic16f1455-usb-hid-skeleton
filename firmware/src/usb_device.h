/*
 * Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Released under the 2-Clause BSD License (see LICENSE file).
 * Parts of the USB driver are based on Microchip Technology Inc. framework.
 */
#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "usb_config.h"

#define USBIF_FLAG              PIR2bits.USBIF
#define USBIE_BIT               PIE2bits.USBIE

#define GET_STATUS              0
#define CLR_FEATURE             1
#define SET_FEATURE             3
#define SET_ADR                 5
#define GET_DSC                 6
#define SET_DSC                 7
#define GET_CFG                 8
#define SET_CFG                 9
#define GET_INTF                10
#define SET_INTF                11
#define SYNCH_FRAME             12

#define DEVICE_REMOTE_WAKEUP    0x01
#define ENDPOINT_HALT           0x00

#define _PPBM0                  0x00
#define _PPBM1                  0x01
#define _PPBM2                  0x02
#define _LS                     0x00
#define _FS                     0x04
#define _TRINT                  0x00
#define _PUEN                   0x10

#define EP_CTRL                 0x06
#define EP_OUT                  0x0C
#define EP_IN                   0x0A
#define EP_OUT_IN               0x0E
#define HSHK_EN                 0x10

#define OUT                     0
#define IN                      1

#define EP00_OUT                ((0x00 << 3) | (OUT << 2))
#define EP00_IN                 ((0x00 << 3) | (IN  << 2))
#define EP01_OUT                ((0x01 << 3) | (OUT << 2))
#define EP01_IN                 ((0x01 << 3) | (IN  << 2))

#define EP0_OUT_EVEN_BDT_INDEX  0
#define EP0_OUT_ODD_BDT_INDEX   1

#define _BSTALL                 0x04
#define _DTSEN                  0x08
#define _DAT0                   0x00
#define _DAT1                   0x40
#define _DTSMASK                0x40
#define _USIE                   0x80
#define _UCPU                   0x00

#define DETACHED_STATE          0
#define ATTACHED_STATE          1
#define POWERED_STATE           2
#define DEFAULT_STATE           3
#define ADR_PENDING_STATE       4
#define ADDRESS_STATE           5
#define CONFIGURED_STATE        6

#define _RAM                    0
#define _ROM                    1

#define DSC_DEV                 0x01
#define DSC_CFG                 0x02
#define DSC_STR                 0x03
#define DSC_INTF                0x04
#define DSC_EP                  0x05

#define _EP01_OUT               0x01
#define _EP01_IN                0x81
#define _INT                    0x03
#define _DEFAULT                (0x01 << 7)

#define WAIT_SETUP              0
#define CTRL_TRF_TX             1
#define CTRL_TRF_RX             2

#define SHORT_PKT_NOT_SENT      0
#define SHORT_PKT_PENDING       1
#define SHORT_PKT_SENT          2

#define SETUP_TOKEN             0b00001101
#define OUT_TOKEN               0b00000001
#define IN_TOKEN                0b00001001

#define HOST_TO_DEV             0
#define DEV_TO_HOST             1

#define STANDARD                0x00
#define CLASS                   0x01
#define VENDOR                  0x02

#define RCPT_DEV                0
#define RCPT_INTF               1
#define RCPT_EP                 2
#define RCPT_OTH                3

#define MUID_NULL               0
#define MUID_USB9               1
#define MUID_HID                2

typedef union _USB_DEVICE_STATUS
{
    uint8_t _byte;
    struct
    {
        unsigned RemoteWakeup:1;
        unsigned ctrl_trf_mem:1;
    };
} USB_DEVICE_STATUS;

typedef union _BD_STAT
{
    uint8_t _byte;
    struct {
        unsigned BC8:1;
        unsigned BC9:1;
        unsigned BSTALL:1;
        unsigned DTSEN:1;
        unsigned INCDIS:1;
        unsigned KEN:1;
        unsigned DTS:1;
        unsigned UOWN:1;
    };
    struct {
        unsigned :2;
        unsigned PID:4;
        unsigned :2;
    };
} BD_STAT;

typedef union _BDT
{
    struct
    {
        BD_STAT Stat;
        uint8_t Cnt;
        uint8_t ADRL;
        uint8_t ADRH;
    };
    struct
    {
        unsigned :8;
        unsigned :8;
        uint8_t* ADR;
    };
} BDT;

typedef union _CTRL_TRF_SETUP
{
    struct {
        uint8_t _byte[EP0_BUFF_SIZE];
    };
    struct {
        uint8_t  bmRequestType;
        uint8_t  bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
    };
    struct {
        unsigned :8;
        unsigned :8;
        WORD_VAL W_Value;
        WORD_VAL W_Index;
        WORD_VAL W_Length;
    };
    struct {
        unsigned Recipient:5;
        unsigned RequestType:2;
        unsigned DataDir:1;
        unsigned :8;
        uint8_t  bFeature;
        unsigned :8;
        unsigned :8;
        unsigned :8;
    };
    struct {
        unsigned :8;
        unsigned :8;
        uint8_t  bDscIndex;
        uint8_t  bDscType;
        uint16_t wLangID;
    };
    struct {
        unsigned :8;
        unsigned :8;
        uint8_t  bDevADR;
    };
    struct {
        unsigned :8;
        unsigned :8;
        uint8_t  bCfgValue;
    };
    struct {
        unsigned :8;
        unsigned :8;
        uint8_t  bAltID;
        uint8_t  bAltID_H;
        uint8_t  bIntfID;
    };
    struct {
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned EPNum:4;
        unsigned :3;
        unsigned EPDir:1;
    };
} CTRL_TRF_SETUP;

typedef union _CTRL_TRF_DATA
{
    uint8_t  _byte[EP0_BUFF_SIZE];
    struct {
        uint8_t _byte0;
        uint8_t _byte1;
        uint8_t _byte2;
        uint8_t _byte3;
        uint8_t _byte4;
        uint8_t _byte5;
        uint8_t _byte6;
        uint8_t _byte7;
    };
} CTRL_TRF_DATA;

typedef union _POINTER
{
    uint8_t*       bRam;
    uint16_t*      wRam;
    const uint8_t* bRom;
    const uint16_t* wRom;
} POINTER;

typedef struct _USB_DEV_DSC
{
    uint8_t  bLength;
    uint8_t  bDscType;
    uint16_t bcdUSB;
    uint8_t  bDevCls;
    uint8_t  bDevSubCls;
    uint8_t  bDevProtocol;
    uint8_t  bMaxPktSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iMFR;
    uint8_t  iProduct;
    uint8_t  iSerialNum;
    uint8_t  bNumCfg;
} USB_DEV_DSC;

typedef struct _USB_CFG_DSC
{
    uint8_t  bLength;
    uint8_t  bDscType;
    uint16_t wTotalLength;
    uint8_t  bNumIntf;
    uint8_t  bCfgValue;
    uint8_t  iCfg;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} USB_CFG_DSC;

typedef struct _USB_INTF_DSC
{
    uint8_t bLength;
    uint8_t bDscType;
    uint8_t bIntfNum;
    uint8_t bAltSetting;
    uint8_t bNumEPs;
    uint8_t bIntfCls;
    uint8_t bIntfSubCls;
    uint8_t bIntfProtocol;
    uint8_t iIntf;
} USB_INTF_DSC;

typedef struct _USB_EP_DSC
{
    uint8_t  bLength;
    uint8_t  bDscType;
    uint8_t  bEPAdr;
    uint8_t  bmAttributes;
    uint16_t wMaxPktSize;
    uint8_t  bInterval;
} USB_EP_DSC;

#define mUSBBufferReady(buffer_dsc) \
{ \
    buffer_dsc.Stat._byte &= _DTSMASK; \
    buffer_dsc.Stat.DTS = !buffer_dsc.Stat.DTS; \
    buffer_dsc.Stat._byte |= _DTSEN; \
    buffer_dsc.Stat._byte |= _USIE; \
}

#define mDisableEP1to7()        UEP1=0x00; UEP2=0x00; UEP3=0x00; UEP4=0x00; UEP5=0x00; UEP6=0x00; UEP7=0x00;

extern uint8_t ctrl_trf_session_owner;
extern POINTER pSrc;
extern POINTER pDst;
extern WORD_VAL wCount;
extern uint8_t usb_device_state;
extern USB_DEVICE_STATUS usb_stat;
extern uint8_t usb_active_cfg;
extern uint8_t usb_alt_intf[MAX_NUM_INT];

extern volatile BDT ep0BoEven;
extern volatile BDT ep0BoOdd;
extern volatile BDT ep0Bi;
extern volatile BDT ep1Bo;
extern volatile BDT ep1Bi;

extern CTRL_TRF_SETUP SetupPkt;
extern volatile CTRL_TRF_DATA CtrlTrfData;

extern volatile unsigned char hid_report_out[HID_INT_OUT_EP_SIZE];
extern volatile unsigned char hid_report_in[HID_INT_IN_EP_SIZE];

extern ROM USB_DEV_DSC device_dsc;
extern ROM uint8_t CFG01[CONFIG_DESC_TOTAL_LEN];
extern ROM unsigned char* ROM USB_SD_Ptr[];

void usb_device_init(void);
void usb_check_bus_status(void);
void usb_soft_attach(void);
void usb_soft_detach(void);
void usb_device_tasks(void);
void usb_disable_with_long_delay(void);
void delay_routine(unsigned int DelayAmount);

#define USBGetDeviceState()     usb_device_state
#define USBIsDeviceSuspended()  UCONbits.SUSPND

#endif // USB_DEVICE_H