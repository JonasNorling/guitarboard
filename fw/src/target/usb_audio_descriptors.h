/**
 * Those definitions were copied from https://github.com/svofski/libopencm3,
 * usbaudio_descriptors branch, c2add53.
 * Those changes were submitted as a pull request on libopencm3, but it
 * hasn't been merged yet.
 *
 * Once this functionality is available in libopencm3 we should migrate
 * to that implementation.
 */

struct usb_audio_streaming_interface_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;        // USB_AUDIO_DT_CS_INTERFACE
    uint8_t bDescriptorSubtype;     // USB_AUDIO_STREAMING_DT_GENERAL, etc
    uint8_t bTerminalLink;
    uint8_t bDelay;
    uint16_t wFormatTag;            // USB_AUDIO_FORMAT_PCM, etc
} __attribute__((packed));

struct usb_audio_type_i_iii_format_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;        // USB_AUDIO_DT_CS_INTERFACE
    uint8_t bDescriptorSubtype;     // USB_AUDIO_STREAMING_DT_FORMAT_TYPE
    uint8_t bFormatType;            // USB_AUDIO_FORMAT_TYPE_I, etc
    uint8_t bNrChannels;
    uint8_t bSubframeSize;          // bytes per frame (e.g. 2 for 16-bit)
    uint8_t bBitResolution;         // e.g. 16
    uint8_t bSamFreqType;
    uint8_t tSampFreq[3];           // see USB_AUDIO_SAMPFREQ(X)
} __attribute__((packed));

struct usb_audio_cluster_descriptor {
    uint8_t bNrChannels;
    /* A bit field that indicates which spatial locations are present in the cluster.
           See USB_AUDIO_CHAN for bit definitions */
    uint16_t wChannelConfig;
    uint8_t iChannelNames;
} __attribute__((packed));

struct usb_audio_input_terminal_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bTerminalID;
    uint16_t wTerminalType;
    uint8_t bAssocTerminal;

    struct usb_audio_cluster_descriptor cluster_descriptor;

    uint8_t iTerminal;
} __attribute__((packed));

struct usb_audio_streaming_endpoint_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bmAttributes;
    uint8_t bLockDelayUnits;
    uint16_t wLockDelay;
} __attribute__((packed));

/* Audio Streaming Interface Descriptor Subtypes */
#define USB_AUDIO_STREAMING_DT_GENERAL                 0x01
#define USB_AUDIO_STREAMING_DT_FORMAT_TYPE             0x02

/* Data format tags - wFormatTag */
#define USB_AUDIO_FORMAT_PCM                    0x0001

/* Audio Format Types */
#define USB_AUDIO_FORMAT_TYPE_I                 0x01

/* Table 2-1: Type I Format Descriptor, bSamFreqType */
#define USB_AUDIO_SAMPLING_FREQ_FIXED           1

#define USB_AUDIO_SAMPFREQ(X) {(X)&0xff,((X)>>8)&0xff,((X)>>16)&0xff}

/* Input terminal types */
#define USB_AUDIO_INPUT_TERMINAL_TYPE_MICROPHONE                0x201

/* Audio Channel config (see audio_input_terminal_descriptor.wChannelConfig */
#define USB_AUDIO_CHAN_MONO             (0)
#define USB_AUDIO_CHAN_LEFTFRONT        (1 << 0)
#define USB_AUDIO_CHAN_RIGHTFRONT       (1 << 1)

/* Audio specific Endpoint subtypes */
#define USB_AUDIO_EP_GENERAL        1

