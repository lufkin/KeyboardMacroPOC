# Pico Native USB Keyboard Macro Injector

This is a native C firmware for the Raspberry Pi Pico WH using the Pico SDK and TinyUSB. It implements a standalone USB HID keyboard with a single button that triggers a two-line test macro.

## Key Differences from CircuitPython

- **Native TinyUSB stack**: Direct USB HID control, no CircuitPython overhead or composite device behavior.
- **Standard USB descriptor**: Legitimate Pico VID/PID, clean boot-protocol keyboard.
- **Firmware control of pacing**: Character delays are handled in C firmware, not Python.
- **No storage, serial, or MIDI**: Only HID keyboard interface, matching what the Switch 2 expects.

## Prerequisites

1. **Pico SDK**: Install the Raspberry Pi Pico SDK:
   ```bash
   git clone https://github.com/raspberrypi/pico-sdk.git
   cd pico-sdk
   git submodule update --init
   ```

2. **Build tools**:
   ```bash
   # On Windows (with MSVC build tools installed):
   # Install CMake, Python, and a C compiler (e.g., arm-none-eabi-gcc)
   
   # On macOS/Linux:
   # Install cmake, arm-none-eabi-gcc, and pkg-config
   ```

3. **Set PICO_SDK_PATH environment variable**:
   ```bash
   export PICO_SDK_PATH=/path/to/pico-sdk
   ```

## Build

```bash
mkdir build
cd build
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
make
```

The resulting `.uf2` file is `pico_keyboard_macro.uf2` in the build directory.

## Flash

1. Hold `BOOTSEL` on the Pico WH while connecting its USB cable.
2. It appears as `RPI-RP2` drive.
3. Copy `pico_keyboard_macro.uf2` to the drive.
4. The Pico reboots and runs the firmware.

There is **no `CIRCUITPY` drive** when this native firmware is active. Programming requires holding `BOOTSEL` again and copying a new `.uf2`.

## Wiring

Connect a momentary button between:
- **GP15** (physical pin 20)
- **GND** (physical pin 18 or similar)

The firmware enables internal pull-up, so no external resistor is needed.

## Testing

### PC Test
1. Connect Pico to PC via USB.
2. Open Notepad and click inside.
3. Press the button.

You should see `/echo Pico macro test` typed twice, with deliberate pacing between characters and a one-second pause between lines.

### Switch 2 Test
1. Disconnect from PC.
2. Connect to USB port on Switch 2 dock.
3. Open FFXIV chat and focus the input field.
4. Press the button.

The device presents as a standard USB HID boot-protocol keyboard, which the Switch 2 should recognize.

## Customization

Edit `main.c` to change:
- `MACRO_TEXT`: The string to type (currently `/echo Pico macro test`).
- `macro_line_count`: Number of times to send the text (currently 2).
- `KEY_DELAY_MS`: Milliseconds between each character (currently 100).
- `LINE_DELAY_MS`: Milliseconds after Enter before the next line (currently 1000).
- `BUTTON_PIN`: GPIO pin for the button (currently 15).

Rebuild and flash after changes.

## Next Steps

Once this basic two-line test works on the Switch 2, we can extend it to:
1. Store multiple macro slots in onboard flash.
2. Add a Wi-Fi configuration interface (for Pico W).
3. Implement a local HTTP API for macro uploads.
4. Support multiple buttons for different macro slots.
