 # PIC16F1455 Minimal USB HID Skeleton

A lightweight, crystal-less USB HID starter template for the **Microchip PIC16F1455**, featuring non-volatile Flash self-read/write and cross-platform WebHID support.

No external crystal, no proprietary driver installation, and no high-voltage programming required.

---

## ✨ Features

- **Crystal-less Operation (No External Oscillator)**  
  Leverages internal 16MHz HFINTOSC with 3x PLL (48MHz CPU clock) and Active Clock Tuning (ACT) to automatically sync with USB host SOF packets.
- **Low-Voltage Programming (LVP Enabled)**  
  Compatible with budget programmers such as MPLAB SNAP and PICkit 3/4/5 without requiring high-voltage MCLR switching. The MCLR pin is freed up for standard I/O (RA3).
- **Driverless Cross-Platform USB HID**  
  Uses standard OS HID drivers out of the box (Windows, macOS, Linux). No WinUSB, libusb, or driver INF setup required.
- **On-Chip Flash Self-Read / Self-Write**  
  Easily store and retrieve 32-byte data blocks directly to/from program memory (`0x1FC0`–`0x1FFF`).
- **Zero-Install WebHID Interface**  
  Test and control the MCU directly from Google Chrome or Microsoft Edge via a single standalone HTML file.

---

## 📌 Minimal Hardware Setup (Breadboard Friendly)

Only **two capacitors** and a USB connector are needed to get started:

```text
               PIC16F1455 (DIP-14)
                   +---v---+
      USB VBUS ----| 1  14 |---- GND (VSS)
             RA5 --| 2  13 |-- White (D-)
             RA4 --| 3  12 |-- Green (D+)
             RA3 --| 4  11 |---- 0.47uF Cap to GND (VUSB3V3)
             RC5 --| 5  10 |-- RC0
             RC4 --| 6   9 |-- RC1
 LED (RC3) <-[R]---| 7   8 |-- RC2
                   +-------+
