# STM32F401CE Black Pill DFU Flashing Guide

This project uses the STM32F401CE Black Pill microcontroller programmed through the built-in USB DFU (Device Firmware Upgrade) bootloader. This method does not require an external ST-Link programmer.

The procedure below explains how to configure the Arduino IDE and upload code to the STM32 Black Pill using DFU mode.

---

## 1. Install Arduino IDE

Download and install the Arduino IDE:

https://www.arduino.cc/en/software

---

## 2. Install STM32 Board Support Package

Open the Arduino IDE.

Navigate to:

```text
File → Preferences
```

In the “Additional Boards Manager URLs” field, add:

```text
https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
```

Click **OK**.

---

## 3. Install STM32 Boards

Navigate to:

```text
Tools → Board → Boards Manager
```

Search for:

```text
STM32
```

Install:

```text
STM32 MCU based boards
```

published by STMicroelectronics.

---

## 4. Connect the Black Pill

Connect the STM32 Black Pill to the PC using a USB-C cable.

### Important Notes

- Some USB cables are power-only and do not support data transfer.
- If the board is not detected, try another cable.

---

## 5. Enter DFU Bootloader Mode

To upload code using DFU mode:

### Step 1 — Hold BOOT0 High

Press and hold the:

```text
BOOT0
```

button on the Black Pill.

---

### Step 2 — Reset the Board

While holding BOOT0:

- press and release the RESET button

or

- unplug and reconnect USB power.

---

### Step 3 — Release BOOT0

Release the BOOT0 button.

The STM32 should now enter DFU bootloader mode.

---

## 6. Verify DFU Detection

Open:

```text
Tools → Port
```

The board should appear as something similar to:

```text
STM32 BOOTLOADER
```

or

```text
DFU in FS Mode
```

If no DFU device appears:

- repeat the BOOT0 + RESET process
- try another USB cable
- ensure STM32 drivers are installed correctly

---

## 7. Configure Arduino IDE Board Settings

Use the following settings:

### Board

```text
STM32 MCU based boards → Generic STM32F4 series
```

### Board Part Number

```text
BlackPill F401CE
```

### Upload Method

```text
DFU
```

### USB Support

```text
CDC (Generic Serial Supersede U(S)ART)
```

---

## 8. Upload Example Code

Open or create a sketch:

```cpp
void setup() {

    pinMode(PC13, OUTPUT);

}

void loop() {

    digitalWrite(PC13, LOW);
    delay(500);

    digitalWrite(PC13, HIGH);
    delay(500);

}
```

Click:

```text
Upload
```

The IDE will compile and upload the firmware through USB DFU.

---

## 9. Exit DFU Mode

After uploading:

- press RESET once

or

- power-cycle the board.

The uploaded firmware should now begin executing.

---

## 10. Serial Monitor Setup

To use the Serial Monitor:

```cpp
Serial.begin(115200);
```

Open:

```text
Tools → Serial Monitor
```

and set the baud rate to:

```text
115200 baud
```

---

# Common Problems

## Board Not Detected

Possible causes:

- incorrect USB cable
- drivers not installed
- board not in DFU mode

---

## Upload Fails

Ensure:

- Upload Method = DFU
- BOOT0 sequence performed correctly
- no other application is using the USB port

---

## Serial Monitor Not Working

After upload:

- press RESET once
- reconnect USB if necessary
- verify correct COM port selected

---

# Advantages of DFU Uploading

Using DFU mode provides several advantages:

- no external programmer required
- simple USB-only workflow
- lower development cost
- faster iteration during testing

This made DFU programming suitable for rapid prototyping and debugging during development of the Optical Sediment Trap system.

