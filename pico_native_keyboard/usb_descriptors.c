/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"

#define USB_VID   0x2E8A     /* Raspberry Pi */
#define USB_PID   0x0105     /* Pico */

/* Device Descriptors */
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

enum
{
  ITF_NUM_HID,
  ITF_NUM_TOTAL
};

uint8_t const desc_hid_report[] =
{
  TUD_HID_REPORT_DESC_KEYBOARD()
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance)
{
  (void) instance;
  return desc_hid_report;
}

uint8_t const desc_configuration[] =
{
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN,
                        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report),
                     0x81, CFG_TUD_HID_EP_BUFSIZE, 10)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index;
  return desc_configuration;
}

static const char* string_desc_arr [] =
{
  (const char[]) { 0x09, 0x04 },
  "Lufkin",
  "Pico USB Keyboard Macro Injector",
  "123456",
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  uint8_t chr_count;

  if ( index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }else
  {
    if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

    const char* str = string_desc_arr[index];

    chr_count = strlen(str);

    for(uint8_t i=0; i<chr_count; i++)
    {
      _desc_str[1+i] = str[i];
    }
  }

  _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

  return _desc_str;
}
