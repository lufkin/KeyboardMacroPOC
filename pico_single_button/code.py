import board
import digitalio
import time
import usb_hid

from adafruit_hid.keyboard import Keyboard
from adafruit_hid.keyboard_layout_us import KeyboardLayoutUS
from adafruit_hid.keycode import Keycode


BUTTON_PIN = board.GP15
DEBOUNCE_SECONDS = 0.03
MACRO_TEXT = '/echo Pico macro test'
MACRO_LINE_COUNT = 2
KEY_DELAY_SECONDS = 0.10
LINE_DELAY_SECONDS = 1.00


keyboard = Keyboard(usb_hid.devices)
keyboard_layout = KeyboardLayoutUS(keyboard)

button = digitalio.DigitalInOut(BUTTON_PIN)
button.switch_to_input(pull=digitalio.Pull.UP)


def send_macro():
    for _ in range(MACRO_LINE_COUNT):
        for character in MACRO_TEXT:
            keyboard_layout.write(character)
            time.sleep(KEY_DELAY_SECONDS)
        keyboard.send(Keycode.ENTER)
        time.sleep(LINE_DELAY_SECONDS)


was_pressed = not button.value

while True:
    is_pressed = not button.value

    if is_pressed and not was_pressed:
        time.sleep(DEBOUNCE_SECONDS)
        if not button.value:
            send_macro()
            was_pressed = True

    if not is_pressed:
        was_pressed = False

    time.sleep(0.01)