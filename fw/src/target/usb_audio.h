#pragma once

#include "codec.h"
#include <libopencm3/usb/usbd.h>

void usbAudioSetupEps(usbd_device *usbd_dev);
void usbAudioFeed(const AudioBuffer* restrict in, AudioBuffer* restrict out);

extern const struct usb_interface_descriptor audio_control_iface[];
extern const struct usb_interface_descriptor audio_streaming_iface[];
