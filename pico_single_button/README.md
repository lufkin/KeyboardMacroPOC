# Pico WH single-button macro test

This is a focused hardware proof: pressing one momentary button causes a Pico
WH to act as a USB keyboard, type the same FFXIV chat command on 14 lines, and
press Enter after every line.

## Wiring

Connect the button between the Pico WH pins below:

| Button terminal | Pico WH pin |
| --- | --- |
| 1 | `GP15` (physical pin 20) |
| 2 | `GND` (for example, physical pin 18) |

The script enables the Pico's internal pull-up, so no resistor is required.
The button is pressed when it connects `GP15` to ground.

## Install

1. Install the current CircuitPython UF2 for the **Raspberry Pi Pico W** from
   [circuitpython.org/board/raspberry_pi_pico_w](https://circuitpython.org/board/raspberry_pi_pico_w/).
   The Pico WH is the header-equipped Pico W.
2. Put the board in bootloader mode: hold `BOOTSEL`, connect its USB cable to
   the computer, then release `BOOTSEL`.
3. Copy the downloaded UF2 onto the `RPI-RP2` drive. It restarts as
   `CIRCUITPY`.
4. Copy the CircuitPython `adafruit_hid` library folder into
   `CIRCUITPY/lib/`. Download the matching library bundle from
   [circuitpython.org/libraries](https://circuitpython.org/libraries).
5. Copy both `boot.py` and `code.py` from this folder to the root of
   `CIRCUITPY`. The `boot.py` configuration limits its HID functions to a
   standard USB keyboard, which is the most compatible HID setup for console
   tests.

## Switch boot-keyboard test

For a Switch test, hold the wired button down while connecting the Pico to a
USB port on the dock. This enables a boot-protocol USB keyboard and disables
the `CIRCUITPY` drive only for that connection. After the Switch has started,
release the button, then press it once to send the two test lines.

To return to normal programming mode, unplug the Pico and reconnect it without
holding the button. The `CIRCUITPY` drive will return.

Open a plain text editor, select its input area, and press the button. It
should type `/echo Pico macro test` and submit it 14 times. Change
`MACRO_TEXT`, `MACRO_LINE_COUNT`, or `LINE_DELAY_SECONDS` in `code.py` only
after that basic behavior works.

## Console test

After confirming the text-editor test, disconnect the Pico, connect it to the
console, focus the game's chat input, and press the button once. Do not keep
the programming computer attached: the Pico has one USB device port.

This firmware intentionally performs one fixed action only. It is the
foundation for later slot storage, uploader, cancellation, and pacing work.