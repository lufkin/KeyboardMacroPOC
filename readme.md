# Technical Product Specification: Universal FFXIV Crafting Macro Injector Stick
**Project Type:** Standalone Hardware Emulation Appliance + Browser Integration  
**Architecture:** Web-to-Microcontroller (WebUSB/WebBLE) ➔ USB HID Keyboard Emulation  
**Target Platform:** Nintendo Switch 2 / PlayStation 5 (FFXIV Console Client)  
**Development Target:** Python/CircuitPython Workspace for VS Code & Gemini AI  

---

## 1. Product Concept & Strategy Pivot
To eliminate the high-friction requirement of flashing custom keyboard firmware, the system is re-engineered into a standalone, low-cost hardware appliance (e.g., Raspberry Pi Pico, ESP32-S3, or Adafruit Trinkey). 

The device acts as a "smart bridge." It exposes a clean data-syncing API to web applications (Raphael-XIV / Teamcraft) via native browser communication protocols, stores the macro array internally, and connects to the console as a standard USB HID keyboard to execute the automated keystrokes.

### Connection Constraint
The standard USB port on a Pico, Trinkey, or similar microcontroller is a USB **device** port. It can act as either the browser-facing WebUSB device during configuration or the console-facing HID keyboard during playback, but it cannot do both on the same port at the same time.

The MVP workflow is therefore: configure the device from a computer or phone over USB, disconnect it, then connect it to the console for playback. A later wireless model can use Web Bluetooth for configuration while its USB port remains connected to the console; this requires a BLE-capable target such as an ESP32-S3 or Pico W-class board. A two-USB-controller design is possible but out of scope for the MVP.

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

## 2. Hardware Interface Specification

### 2.1 Supported Microcontrollers (Reference Targets)
* **Raspberry Pi Pico / Pico 2:** Wired WebUSB solution via native USB controller.
* **Adafruit NeoKey Trinkey / USB Keys:** Direct-plug form factor (no cables required).
* **ESP32-S3 / Pico 2 W:** Wireless Web Bluetooth (BLE) + Wired USB combination target.

### 2.2 Device Storage & State Management
The device maintains an internal virtual drive (FAT12/VFS). Macros sent from the web app are saved locally as lightweight files or raw memory structures:
* `macro_0.txt`, `macro_1.txt`, `macro_2.txt` ... 
* A simple state registry determines the total number of chunks and execution slot bindings.

---

## 3. Communication Interface (The Web-to-Hardware API)

Because modern web browsers natively support low-level hardware connections, you do not need to distribute a desktop or mobile application. The browser handles the link directly.

### 3.1 Wired Connection: WebUSB / WebHID API
The web platform requests connection rights to the device's Vendor ID (VID) and Product ID (PID).

```javascript
// Web Browser Integration Snippet (Target: Raphael-XIV / Teamcraft)
async function syncMacrosToStick(rawMacroText) {
    // 1. Request USB Access to the custom Dongle
    const device = await navigator.usb.requestDevice({
        filters: [{ vendorId: 0x239A }] // Example Adafruit/Pico VID baseline
    });
    
    await device.open();
    await device.selectConfiguration(1);
    await device.claimInterface(0);

    // 2. Transmit chunks directly over the control transfer wire
    const encoder = new TextEncoder();
    const data = encoder.encode(rawMacroText);
    
    // Command 0x42 initializes data buffer flashing
    await device.controlTransferOut({
        requestType: 'vendor',
        recipient: 'device',
        request: 0x42,
        value: 0,
        index: 0
    }, data);
    
    console.log("Sync Complete!");
}
```

### 3.2 Wireless Connection: Web Bluetooth (WebBLE) API
For wireless variations, the browser exposes an explicit GATT communication service. The browser connects over-the-air, updates the configuration records, and allows the device to process the values seamlessly.

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