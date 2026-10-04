# Copyright (c) 2026 Rilassaru (https://rilassaru.blog.jp/)
# SPDX-License-Identifier: BSD-2-Clause
#
# Released under the 2-Clause BSD License (see LICENSE file).
# Parts of the USB driver are based on Microchip Technology Inc. framework.

import time
import hid

# Device identifiers; these must match usb_descriptors.c in the firmware.
VENDOR_ID = 0x04D8
PRODUCT_ID = 0x003C

# Command values used by the device's HID protocol.
CMD_GET_STATUS = 0x10
CMD_SET_STATUS = 0x11
CMD_GET_DATA   = 0x20
CMD_SET_DATA   = 0x21

LED_OFF    = 0
LED_ON     = 1
LED_TOGGLE = 2


def send_cmd(dev, cmd, payload=None):
    """
    Send a 64-byte report and read the 64-byte response.

    HIDAPI on Windows expects the Report ID (0x00) before the 64 report bytes,
    so the write call sends 65 bytes in total.
    """
    # The first report byte is the command; any remaining unused bytes stay zero.
    buf = [0] * 64
    buf[0] = cmd
    if payload:
        for i, val in enumerate(payload):
            if i + 1 < 64:
                buf[i + 1] = val

    # [Report ID: 0x00] + [64 bytes payload]
    dev.write([0x00] + buf)
    res = dev.read(64, timeout_ms=1000)
    return res


def main():
    print(f"Connecting to PIC16F1455 (VID: 0x{VENDOR_ID:04X}, PID: 0x{PRODUCT_ID:04X})...")
    
    try:
        dev = hid.device()
        dev.open(VENDOR_ID, PRODUCT_ID)
    except Exception as e:
        print(f"Error: Device not found. Check the USB connection. ({e})")
        return

    print("Connected successfully!\n")

    try:
        # 1. Request the current status and decode the response fields.
        res = send_cmd(dev, CMD_GET_STATUS)
        status, led, user_val = res[0], res[1], res[2]
        loop_cnt = (res[3] << 24) | (res[4] << 16) | (res[5] << 8) | res[6]
        print(f"[GET_STATUS] Result: {status}, LED: {led}, UserVal: {user_val}, LoopCount: {loop_cnt}")

        # 2. Turn on the LED and set the device's sample user value.
        print("\nTurning on the LED and writing 0x77 to UserVal...")
        # The payload starts after the command: LED state, then user value.
        res = send_cmd(dev, CMD_SET_STATUS, [LED_ON, 0x77])
        print(f"[SET_STATUS] Result: {res[0]}, LED after update: {res[1]}, UserVal after update: {res[2]}")

        time.sleep(1)

        print("Toggling the LED off...")
        res = send_cmd(dev, CMD_SET_STATUS, [LED_TOGGLE, 0x77])
        print(f"[SET_STATUS] Result: {res[0]}, LED after update: {res[1]}")

        # -------------------------------------------------------------
        # 3. Read the 32-byte data block from Flash page 0.
        # -------------------------------------------------------------
        print("\nReading Flash data from page 0 (0x1FC0~)...")
        # The first payload byte selects the Flash page.
        res = send_cmd(dev, CMD_GET_DATA, [0x00])
        status = res[0]
        page = res[1]
        addr = (res[2] << 8) | res[3]
        data = res[4:36]
        print(f"[GET_DATA] Page: {page}, Address: 0x{addr:04X}")
        print("Data:", " ".join(f"{b:02X}" for b in data))

        # -------------------------------------------------------------
        # 4. Erase and write a test block to Flash page 1, then verify it.
        # -------------------------------------------------------------
        print("\nWriting new test data to page 1 (0x1FE0~)...")
        # The payload is page number, two reserved bytes, then 32 data bytes.
        new_data = [0x55, 0xAA] + [i for i in range(30)]
        payload = [0x01, 0x00, 0x00] + new_data

        res = send_cmd(dev, CMD_SET_DATA, payload)
        status = res[0]
        page = res[1]
        verify_data = res[4:36]
        print(f"[SET_DATA] Write to page {page} completed")
        print("Verify:", " ".join(f"{b:02X}" for b in verify_data))

        if list(verify_data) == new_data:
            print("=> Verification passed! Flash write and read are working correctly.")
        else:
            print("=> Verification failed!")

    finally:
        dev.close()
        print("\nDisconnected.")


if __name__ == "__main__":
    main()