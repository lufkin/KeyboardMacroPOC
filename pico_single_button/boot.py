import board
import digitalio
import storage
import usb_cdc
import usb_hid
import usb_midi

button = digitalio.DigitalInOut(board.GP15)
button.switch_to_input(pull=digitalio.Pull.UP)

if not button.value:
	storage.disable_usb_drive()
	usb_cdc.disable()
	usb_midi.disable()
	usb_hid.enable((usb_hid.Device.KEYBOARD,), boot_device=1)
else:
	usb_hid.enable((usb_hid.Device.KEYBOARD,))