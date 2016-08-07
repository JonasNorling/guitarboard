#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/audio.h>
#include "usb_audio_descriptors.h"
#include "platform.h"
#include "usb_audio.h"

// Includes for isochronous hacks
#include <libopencm3/lib/usb/usb_private.h>
#include <libopencm3/stm32/otg_hs.h>
#define OTG_DIEPCTLX_SODDFRM (1 << 29)
#define OTG_DIEPCTLX_SEVNFRM (1 << 28)

#define EP_ID_AUDIO_FROM_ME 0x83

// Send 48 samples (2 channels * 2 bytes) every 1 ms
#define AUDIO_PACKET_SAMPLES 48
#define AUDIO_PACKET_SIZE (AUDIO_PACKET_SAMPLES*4)
#define MAX_AUDIO_PACKET_SIZE (AUDIO_PACKET_SIZE + 16)

#define TERMINAL_LINE_IN 1

struct UsbAudioFrame
{
    int16_t s[2];
} __attribute__((packed)) __attribute__((aligned(4)));

static struct UsbAudioFrame audioInBuffer[CODEC_SAMPLES_PER_FRAME * 6];
static uint16_t txBufferWritePos;
static uint16_t txBufferReadPos;

static const struct usb_audio_streaming_endpoint_descriptor audio_streaming_endp = {
        .bLength = sizeof(struct usb_audio_streaming_endpoint_descriptor),
        .bDescriptorType = USB_AUDIO_DT_CS_ENDPOINT,
        .bDescriptorSubtype = USB_AUDIO_EP_GENERAL,
        .bmAttributes = 0,
        .bLockDelayUnits = 0,
        .wLockDelay = 0,
};

static const struct usb_endpoint_descriptor audio_endp[] = {{
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = EP_ID_AUDIO_FROM_ME,
        .bmAttributes = USB_ENDPOINT_ATTR_ASYNC | USB_ENDPOINT_ATTR_ISOCHRONOUS,
        .wMaxPacketSize = MAX_AUDIO_PACKET_SIZE,
        .bInterval = 1,

        .extra = &audio_streaming_endp,
        .extralen = sizeof(audio_streaming_endp),
} };

static const struct {
        struct usb_audio_header_descriptor_head header_head;
        struct usb_audio_header_descriptor_body header_body[1];
        struct usb_audio_input_terminal_descriptor linein_input_terminal;
} __attribute__((packed)) audio_control_functional_descriptors = {
        .header_head = {
                .bLength = sizeof(audio_control_functional_descriptors.header_head) +
                           sizeof(audio_control_functional_descriptors.header_body),
                .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
                .bDescriptorSubtype = USB_AUDIO_TYPE_HEADER,
                .bcdADC = 0x0100,
                .wTotalLength = sizeof(audio_control_functional_descriptors),
                .binCollection = 1,
        },
        .header_body = { {
                .baInterfaceNr = 3, // audio_streaming_iface interface number
        } },
        .linein_input_terminal = {
                .bLength = sizeof(struct usb_audio_input_terminal_descriptor),
                .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
                .bDescriptorSubtype = USB_AUDIO_TYPE_INPUT_TERMINAL,
                .bTerminalID = TERMINAL_LINE_IN,
                .wTerminalType = USB_AUDIO_INPUT_TERMINAL_TYPE_MICROPHONE,
                .bAssocTerminal = 0,
                .cluster_descriptor = {
                    .bNrChannels = 2,
                    .wChannelConfig = USB_AUDIO_CHAN_LEFTFRONT | USB_AUDIO_CHAN_RIGHTFRONT,
                    .iChannelNames = 0,
                },
                .iTerminal = 0,
        },
};

static const struct {
    struct usb_audio_streaming_interface_descriptor linein;
    struct usb_audio_type_i_iii_format_descriptor format;
} __attribute__((packed)) audio_streaming_functional_descriptors = {
        .linein = {
                .bLength = sizeof(struct usb_audio_streaming_interface_descriptor),
                .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
                .bDescriptorSubtype = USB_AUDIO_STREAMING_DT_GENERAL,
                .bTerminalLink = TERMINAL_LINE_IN,
                .bDelay = 1,
                .wFormatTag = USB_AUDIO_FORMAT_PCM,
        },
        .format = {
                .bLength = sizeof(struct usb_audio_type_i_iii_format_descriptor),
                .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
                .bDescriptorSubtype = USB_AUDIO_STREAMING_DT_FORMAT_TYPE,
                .bFormatType = USB_AUDIO_FORMAT_TYPE_I,
                .bNrChannels = 2,
                .bSubframeSize = 2,
                .bBitResolution = 16,
                .bSamFreqType = USB_AUDIO_SAMPLING_FREQ_FIXED,
                .tSampFreq = USB_AUDIO_SAMPFREQ(CODEC_SAMPLERATE),
        },
};

const struct usb_interface_descriptor audio_control_iface[] = {{
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 2,
        .bAlternateSetting = 0,
        .bNumEndpoints = 0,
        .bInterfaceClass = USB_CLASS_AUDIO,
        .bInterfaceSubClass = USB_AUDIO_SUBCLASS_CONTROL,
        .bInterfaceProtocol = 0,
        .iInterface = 0,

        .extra = &audio_control_functional_descriptors,
        .extralen = sizeof(audio_control_functional_descriptors)
}};

const struct usb_interface_descriptor audio_streaming_iface[] = {{
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 3,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CLASS_AUDIO,
        .bInterfaceSubClass = USB_AUDIO_SUBCLASS_AUDIOSTREAMING,
        .bInterfaceProtocol = 0,
        .iInterface = 0,

        .endpoint = audio_endp,

        .extra = &audio_streaming_functional_descriptors,
        .extralen = sizeof(audio_streaming_functional_descriptors)
} };

static void audioSendCb(usbd_device *usbd_dev, uint8_t ep)
{
    const uint8_t epno = ep & 0x7f;

    /*
     * Isochronous support: update odd/even frame bits.
     * This functionality isn't in libopencm3 yet, but there are pull
     * requests attempting to get this stuff working.
     */
    if (MMIO32(usbd_dev->driver->base_address + OTG_DSTS) & (1 << 8)) {
        MMIO32(usbd_dev->driver->base_address + OTG_DIEPCTL(epno)) |= OTG_DIEPCTLX_SEVNFRM;
    }
    else {
        MMIO32(usbd_dev->driver->base_address + OTG_DIEPCTL(epno)) |= OTG_DIEPCTLX_SODDFRM;
    }

    usbd_ep_write_packet(usbd_dev, ep, &audioInBuffer[txBufferReadPos],
            AUDIO_PACKET_SIZE);

    txBufferReadPos += AUDIO_PACKET_SAMPLES;
    txBufferReadPos %= sizeof(audioInBuffer) / sizeof(*audioInBuffer);
}

void usbAudioFeed(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    (void)out;
    (void)in;

    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        audioInBuffer[txBufferWritePos].s[0] = in->s[s][0];
        audioInBuffer[txBufferWritePos].s[1] = in->s[s][1];
        txBufferWritePos++;
        txBufferWritePos %= sizeof(audioInBuffer) / sizeof(*audioInBuffer);
    }
}

void usbAudioSetupEps(usbd_device *usbd_dev)
{
    usbd_ep_setup(usbd_dev, EP_ID_AUDIO_FROM_ME, USB_ENDPOINT_ATTR_ISOCHRONOUS, MAX_AUDIO_PACKET_SIZE, audioSendCb);

    audioSendCb(usbd_dev, EP_ID_AUDIO_FROM_ME);
}
