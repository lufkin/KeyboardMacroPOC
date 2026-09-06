# Technical Product Specification: Universal FFXIV Crafting Macro Injector Stick
**Project Type:** Standalone Hardware Emulation Appliance + Browser Integration  
**Architecture:** Web-to-Microcontroller (WebUSB/WebBLE) ➔ USB HID Keyboard Emulation  
**Target Platform:** Nintendo Switch 2 / PlayStation 5 (FFXIV Console Client)  
**Development Target:** Python/CircuitPython Workspace for VS Code & Gemini AI  

---

## 1. Executive Summary & Design Goals
The objective of this project is to build a lightweight, cross-platform Python utility that bypasses restrictive web graphical interfaces (VIA/Keychron Launcher) to dynamically inject lengthy, multi-line macro configurations over a raw USB pipeline. 

### Key Engineering Constraints:
* **Target Application:** Automated string injection into *Final Fantasy XIV (FFXIV)* running on standard console platforms (e.g., Nintendo Switch / PlayStation 5).
* **Game Engine Rules:** In-game text macros are strictly capped at **15 rows maximum**.
* **Device Portability & Durability:** The architecture must scale gracefully across diverse QMK/VIA-compatible mechanical keyboard hardware matrix definitions, relying purely on low-level USB protocols without demanding proprietary manufacturer-specific handshakes.

---


### 2. Detailed System Architecture Topology

```text
                   +──────────────────────────────────+

                   |    Raphael-XIV / Teamcraft Web   |
                   |   (Generates multi-line macro)   |
                   +────────────────┬─────────────────+
                                    |
                                    | (Browser Link Connection)
                                    v
                   +──────────────────────────────────+

                   |   Modern Browser Layout Port     |
                   | (Uses Native WebUSB / WebBLE API)|
                   +────────────────┬─────────────────+
                                    |
                                    | (Sends raw string byte arrays)
                                    v
+─────────────────────────────────────────────────────────────────────+

|               Target Emulator Hardware Appliance                    |
|                (e.g., Raspberry Pi Pico Dongle)                     |
|                                                                     |
|    +──────────────────────+              +──────────────────────+   |
|    | CircuitPython File   |              |  Local Flash VFS     |   |
|    |   Engine Runtime     |              |    Storage System    |   |
|    |                      |              |                      |   |
|    | Intercepts payload   ├─────────────►| Commits and saves to |   |
|    | data streams natively|              | macro_0.txt / 1.txt  |   |
|    +──────────────────────+              +──────────────────────+   |
|               |                                                     |
|               | (User presses physical button to trigger script)    |
|               v                                                     |
|    +──────────────────────+                                         |
|    |   USB HID Keyboard   |                                         |
|    |   Emulation Layer    |                                         |
|    |                      |                                         |
|    | Controls character   |                                         |
|    | delays at 45ms loops |                                         |
|    +──────────────────────+                                         |
+───────────────────────────────┬─────────────────────────────────────+
                                |
                                | (Standard USB Lead Wired Bridge)
                                v
                   +──────────────────────────────────+

                   | Nintendo Switch 2 Console Dock   |
                   |                                  |
                   | Logs dongle as generic hardware, |
                   | typing lines without text bugs.  |
                   +──────────────────────────────────+
```



---

## 3. Component Breakdown & Data Flow

### 3.1 Python Data Orchestration Engine
The pipeline is designed as an automated sequence splitter that parses an arbitrary string block into a deterministic array of sub-macros matching target boundaries.

1. **The Parsing Algorithm (14-Line Ceiling):**
   * Splitting a raw string payload into chunk increments of precisely **14 lines**.
   * Reserving row **15** explicitly for a context-aware navigation string (`/hotbar change X` or `/crosshotbar change X`) to allow chaining separate hardware macro execution slots together sequentially.
2. **The Formatting Engine:**
   * Sanitizing line-ending variations (`\r\n` vs `\n`).
   * Stripping unsupported characters and ensuring string termination arrays (`\0`) pack out the remainder of fixed 64-byte payload windows.

### 3.2 Hardware Direct Communication Layer (VIA / QMK HID)
Standard configurations are decoupled by targeting bare communication endpoints directly. The tool interfaces natively with target keyboards using vendor tracking identifiers.

* **Target USB Vendor Identifiers (Keychron Baseline):**
  * Vendor ID (VID): `0x2432` / `0x3434`
  * Product ID (PID): `0x12A2` (K10 Max variant or equivalent)
* **The Raw USB Protocol:** Communication executes across **Interface 1 (Usage Page: `0xFF60`, Usage: `0x61`)**, passing 64-byte packet blocks.

---

## 4. Hardware Firmware Setup (QMK C Layer)

To accept dynamic external macro overriding commands, your keyboard layout code requires standard routing functions enabled in its source tree. 

### `config.h` (Environment Constants)
```c
#pragma once

// Activate the low-level data pipeline 
#define RAW_ENABLE

// Expand raw storage capacity bounds to safely house dense text blocks
#define DYNAMIC_KEYMAP_EEPROM_MAX_ADDR 8191
#define VIA_EEPROM_LAYOUT_OPTIONS_SIZE 2
```

### `keymap.c` (Hardware Message Hook)
```c
#include QMK_KEYBOARD_H

#ifdef RAW_ENABLE
/**
 * Callback triggered whenever the Python engine transmits a 64-byte USB payload frame.
 */
void raw_hid_receive(uint8_t *data, uint8_t length) {
    // Command code 0x32 indicates a custom macro buffer overwrite instruction
    if (data[0] == 0x32) {
        uint8_t target_macro_id = data[1];
        uint8_t payload_chunk[62];
        
        // Isolate message body payload from standard protocol headers
        memcpy(payload_chunk, &data[2], 62);
        
        // Safely commit string chunk directly into the custom hardware EEPROM map
        dynamic_keymap_macro_set_buffer(target_macro_id, payload_chunk);
    }
}
#endif
```

---

## 4. Hardware Playback & Timing Engine (CircuitPython/C++)

The device firmware remains static. It acts strictly as a dedicated script playback engine. When the physical onboard button is pressed (or triggered by a remote event), it streams the saved macro string back out of its USB port into the Nintendo Switch console using precise timing pacing.

### Implementation Blueprint (`code.py` for CircuitPython Target)
```python
import time
import board
import digitalio
import usb_hid
from adafruit_hid.keyboard import Keyboard
from adafruit_hid.keycode import Keycode
from adafruit_hid.keyboard_layout_us import KeyboardLayoutUS

# Initialize Native USB HID Emulation
kbd = Keyboard(usb_hid.devices)
layout = KeyboardLayoutUS(kbd)

# Physical Trigger Button Setup
trigger_btn = digitalio.DigitalInOut(board.GP15)
trigger_btn.direction = digitalio.Direction.INPUT
trigger_btn.pull = digitalio.Pull.UP

# Timing & Pacing Profiles
CHAR_DELAY = 0.045  # 45ms pause between characters (Defeats Switch buffer saturation)
LINE_DELAY = 0.150  # 150ms pause after Enter key (Defeats IME Language switching bug)

def execute_paced_macro(file_path):
    """Reads internal storage and pipes keystrokes to the console."""
    try:
        with open(file_path, "r") as f:
            for line in f:
                clean_line = line.strip()
                if not clean_line:
                    continue
                    
                # Type characters sequentially
                for char in clean_line:
                    layout.write(char)
                    time.sleep(CHAR_DELAY)
                
                # Execute line break transition
                time.sleep(LINE_DELAY)
                kbd.send(Keycode.ENTER)
                time.sleep(LINE_DELAY)
    except Exception as e:
        print("Execution tracking error:", e)

while True:
    if not trigger_btn.value:  # Onboard button pressed
        print("Triggering FFXIV Macro Sequence Loop...")
        execute_paced_macro("/macro_0.txt")
        time.sleep(1.0)  # Debounce safety delay
    time.sleep(0.01)
```

---

## 5. Software Data Transformation Rules (Web / Companion App Side)

The orchestration app (the web application or local helper utility) owns 100% of the optimization logic before the text ever leaves your PC or phone.

1. **The 14-Line Parsing Architecture:**
   * Split incoming multi-line macro configurations every 14 rows.
2. **Dynamic Hotbar Chain Injection:**
   * Automatically detect chunk indexes and insert the matching navigation string as line 15.
   * `Chunk 0` ➔ Appends `/hotbar change 2\n`
   * `Chunk 1` ➔ Appends `/hotbar change 3\n`
   * `Final Chunk` ➔ Appends `/hotbar change 1\n`
3. **Array Packaging:**
   * Stream separate discrete strings to independent internal files (`macro_0.txt`, `macro_1.txt`, etc.) so the hardware engine can execute individual components sequentially on separate button presses or macro shifts.

---

## 6. Prompt Engineering Guide for Gemini Agent Integration

When initialization is complete inside VS Code, hand these development prompts directly to your Gemini agent to build the codebase:

### Prompt 1: Building the String Parsing Utility
> *"We are building a Python-based utility script to preprocess crafting macro text from Raphael-XIV for a micro-appliance. Write a text tokenizer that imports an external raw string file. Split the text blocks at every 14 lines. If there are lines remaining, dynamically add line 15 to the block containing `/hotbar change X` where X increments sequentially. Save each resultant 15-line block to a string array so it is ready for hardware file distribution."*

### Prompt 2: Writing the WebUSB Delivery Module
> *"Write a clean JavaScript integration module that can run natively inside a Google Chrome web browser. The script must request pairing access to a generic microcontroller target using the WebUSB API. Once connected, write a routine that loops through our split macro text string arrays and pushes them down to the USB device interface as sequentially indexed data blocks."*

### Prompt 3: Handling Complex FFXIV Keyboard Mapping
> *"Review our CircuitPython `code.py` layout template. FFXIV macros depend heavily on quotation marks (`"`) and forward slashes (`/`). Modify the string loop to ensure that when a line is parsed, layout modifiers are held down explicitly with a safe padding delay before and after tapping the target symbol keycode. This will prevent the Nintendo Switch console from accidentally triggering its internal Japanese IME language toggle bug."*