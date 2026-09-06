# Technical Specification: Cross-Platform QMK/VIA Dynamic Macro Injector
**Target Architecture:** Custom Python Utility ↔ QMK Raw HID ↔ Console/PC Text Input Target  
**Author:** Senior Data Engineer / Architect  
**Environment:** Visual Studio Code & Gemini Code Assistant Integration  

---

## 1. Executive Summary & Design Goals
The objective of this project is to build a lightweight, cross-platform Python utility that bypasses restrictive web graphical interfaces (VIA/Keychron Launcher) to dynamically inject lengthy, multi-line macro configurations over a raw USB pipeline. 

### Key Engineering Constraints:
* **Target Application:** Automated string injection into *Final Fantasy XIV (FFXIV)* running on standard console platforms (e.g., Nintendo Switch / PlayStation 5).
* **Game Engine Rules:** In-game text macros are strictly capped at **15 rows maximum**.
* **Device Portability & Durability:** The architecture must scale gracefully across diverse QMK/VIA-compatible mechanical keyboard hardware matrix definitions, relying purely on low-level USB protocols without demanding proprietary manufacturer-specific handshakes.
The device acts as a "smart bridge." It exposes a clean data-syncing API to web applications (Raphael-XIV / Teamcraft) via native browser communication protocols, stores the macro array internally, and connects to the console as a standard USB HID keyboard to execute the automated keystrokes.

The product is a configurable USB keyboard macro-input device. Playback requires an intentional physical button press, supports one active macro at a time, and provides a dedicated stop/cancel control. Users are responsible for complying with applicable game and platform rules.

### Open Hardware Distribution
The project will publish its firmware source, browser uploader, KiCad schematic and PCB files, bill of materials, and printable enclosure files. Optional Ko-fi donations and Etsy listings for assembled hardware provide support for users who prefer a ready-built unit.

### Connection Constraint
The standard USB port on a Pico, Trinkey, or similar microcontroller is a USB **device** port. It can act as either the browser-facing WebUSB device during configuration or the console-facing HID keyboard during playback, but it cannot do both on the same port at the same time.

The MVP workflow is therefore: configure the device from a computer or phone over USB, disconnect it, then connect it to the console for playback. A later wireless model can use Web Bluetooth for configuration while its USB port remains connected to the console; this requires a BLE-capable target such as an ESP32-S3 or Pico W-class board. A two-USB-controller design is possible but out of scope for the MVP.

---


### 2. Detailed System Architecture Topology

```text
                  +─────────────────────────────+

                  | Raphael-XIV Crafting Solver |
                  | (Produces raw crafting text)|
                  +──────────────┬──────────────+
                                 |
                                 | (Copy / Web Scrape)
                                 v
                  +─────────────────────────────+

                  |   Python Execution Engine   |
                  | - Tokenizer & Text Cleaner  |
                  | - 14-Line Parsing & Chunking|
                  | - Hotbar Sequence Navigation|
                  +──────────────┬──────────────+
                                 |
                                 | (Serializes into 64-Byte HID packets)
                                 v
                  +─────────────────────────────+

                  |    Standard USB HID Path    |
                  | (Targeting Device VID/PID)  |
                  +──────────────┬──────────────+
                                 |
                                 | (Direct RAW HID Data Pipeline)
                                 v
+─────────────────────────────────────────────────────────────+

|               Keychron K10 Max QMK Firmware                 |
|                                                             |
|   +─────────────────────────+     +─────────────────────+   |
|   |   raw_hid_receive Hook  |     | Dynamic Macro Space |   |
|   |                         |     |                     |   |
|   | Intercepts Command 0x32 ├────►| Overwrites Storage  |   |
|   | Payload Frames          |     | Registers (M0-M3)   |   |
|   +─────────────────────────+     +─────────────────────+   |
+──────────────────────────────┬──────────────────────────────+
                               |
                               | (Physical Selector Switch)
                               v
                  +─────────────────────────────+

                  |       Nintendo Switch       |
                  | (Types text cleanly over USB|
                  +─────────────────────────────+
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
### 2.1 Supported Microcontrollers (Reference Targets)
* **Raspberry Pi Pico W / Pico 2 W (POC recommendation):** USB HID playback with Wi-Fi configuration from a local browser page while remaining connected to the console.
* **Raspberry Pi Pico / Pico 2:** USB HID playback with wired configuration; disconnect from the computer or phone before connecting to the console.
* **Adafruit NeoKey Trinkey / USB Keys:** Direct-plug form factor (no cables required).
* **ESP32-S3 / Pico 2 W:** Wireless Web Bluetooth (BLE) + Wired USB combination target.

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

### 3.2 Wireless Connection: Web Bluetooth (WebBLE) API
For wireless variations, the browser exposes an explicit GATT communication service. The browser connects over-the-air, updates the configuration records, and allows the device to process the values seamlessly.

### 3.3 Wireless Connection: Local Wi-Fi API
The Pico W POC exposes a local Wi-Fi access point and a small HTTP upload API while its USB port remains connected to the console as a HID keyboard. A phone or computer joins the device network, opens its local configuration page, and uploads macro text in an HTTP `POST` body. Raw macro content must not be sent as a URL query parameter because line breaks and special characters require fragile escaping and URLs have practical length limits.

---

## 5. Python Application Boilerplate

This script serves as your functional framework inside Visual Studio Code. It detects standard QMK HID endpoints and translates text sequences into raw serialization buffers.

### Requirements
```bash
pip install hidapi
```

### `macro_injector.py`
```python
import hid
import sys

# Target Device Parameters
TARGET_VID = 0x3434  # Keychron Vendor ID baseline
TARGET_PID = 0x12A2  # K10 Max Target PID footprint

def find_qmk_device(vid, pid):
    """Scans local USB infrastructure for valid QMK Raw HID interfaces."""
    device_interfaces = hid.enumerate(vid, pid)
    for interface in device_interfaces:
        # Standard QMK Custom RAW HID interface page signatures
        if interface['usage_page'] == 0xFF60 and interface['usage'] == 0x0061:
            return interface['path']
    return None

def split_macro_text(raw_text, lines_per_chunk=14):
    """Parses raw text payload into strict, context-bound arrays."""
    lines = [line.strip() for line in raw_text.strip().split('\n') if line.strip()]
    chunks = []
    
    for i in range(0, len(lines), lines_per_chunk):
        chunk = lines[i:i + lines_per_chunk]
        chunks.append("\n".join(chunk))
    return chunks

def inject_macro_to_hardware(device_path, macro_slot, macro_string):
    """Packages and pipes data downstream via an active USB interface link."""
    try:
        dev = hid.device()
        dev.open_path(device_path)
        
        # Initialize 64-Byte raw USB frame buffer array
        # Byte 0 is explicitly reserved as the USB HID Report ID (0x00)
        buffer = [0] * 65 
        
        buffer[1] = 0x32          # Command Signature: Custom Macro Write
        buffer[2] = macro_slot    # Selected target destination index (M0, M1, M2...)
        
        # Serialize text characters to byte format
        encoded_payload = macro_string.encode('utf-8')
        
        if len(encoded_payload) > 62:
            raise ValueError(f"Payload block length ({len(encoded_payload)} bytes) exceeds transaction frame limits.")
            
        # Bind string payload arrays to frame buffer
        for idx, byte_val in enumerate(encoded_payload):
            buffer[3 + idx] = byte_val
            
        # Send raw instruction packet downstream to the hardware
        dev.write(buffer)
        dev.close()
        print(f" Successfully pushed packet to hardware profile slot M{macro_slot}.")
        
    except Exception as e:
        print(f"Transaction failure: {str(e)}", file=sys.stderr)

if __name__ == "__main__":
    # Sample Mock Input Source string derived from Raphael-XIV solvers
    sample_solver_output = """
    /ac "Muscle Memory" <wait.3>
    /ac "Observe" <wait.3>
    /ac "Advanced Touch" <wait.3>
    /ac "Trained Perfection" <wait.3>
    /ac "Veneration" <wait.2>
    /ac "Groundwork" <wait.3>
    """
    
    print("Initializing USB HID Scanning routine...")
    target_path = find_qmk_device(TARGET_VID, TARGET_PID)
    
    if not target_path:
        print("Error: Target QMK/VIA keyboard raw interface endpoint not detected.", file=sys.stderr)
        sys.exit(1)
        
    processed_blocks = split_macro_text(sample_solver_output)
    
    for slot_index, text_payload in enumerate(processed_blocks):
        # Program loop iteratively commits payloads across slots M0, M1, M2...
        inject_macro_to_hardware(target_path, slot_index, text_payload)
```

---

## 6. Prompt Engineering Guide for Gemini Agent Integration

When working with your internal Gemini AI agent inside Visual Studio Code to scale this engine, you can use these hyper-focused technical prompts to iterate on the project:

### Prompt 1: Enhancing Data Processing Automation
> *"Review the `split_macro_text` function in our `macro_injector.py` script. Modify its implementation to dynamically inject a 15th string line row to every chunk index array. If it is the final chunk in the loop, append `/hotbar change 1`. For all preceding chunks, evaluate the current loop index counter and append `/hotbar change X` where X is `index + 2` to safely orchestrate automated macro daisy-chain loops."*

### Prompt 2: Scaling USB Architecture Device Profiling
> *"I want to decouple the current hardcoded `TARGET_VID` and `TARGET_PID` parameters in our script to support scalability for other QMK/VIA custom boards. Help me restructure the module initialization logic to read configurations from an external `devices.json` lookup matrix database file. Include fallback generic tracking routines if an unrecognized keyboard model is detected."*

### Prompt 3: Formatting In-Line Input Pacing Controls
> *"Console OS platforms handle high-velocity USB typing inputs poorly, resulting in dropped text characters. Let's write an isolation formatting module in Python that hooks into the serialization pipeline. This helper must intercept text segments and automatically pad strings with hardware-enforced delay signatures (such as QMK's native `{+DELAY 50}...{-DELAY}` macro strings) to space text strings evenly before sending them down the wire."*