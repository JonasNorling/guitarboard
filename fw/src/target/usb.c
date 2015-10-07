/*
 * Based on libopencm3-examples,
 * examples/stm32/f4/stm32f4-discovery/usb_cdcacm/cdcacm.c.
 *
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/otg_fs.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/cm3/scb.h>
#include "platform.h"

static usbd_device *usbd_dev;
static bool ready;
static char serialNumber[26];

#define EP_BUFFER_SIZE 64

static const struct usb_device_descriptor dev = {
        .bLength = USB_DT_DEVICE_SIZE,
        .bDescriptorType = USB_DT_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = USB_CLASS_CDC,
        .bDeviceSubClass = 0,
        .bDeviceProtocol = 0,
        .bMaxPacketSize0 = 64,
        .idVendor = 0x0483,
        .idProduct = 0x5740,
        .bcdDevice = 0x0200,
        .iManufacturer = 1,
        .iProduct = 2,
        .iSerialNumber = 3,
        .bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x83,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = 16,
        .bInterval = 255,
} };

static const struct usb_endpoint_descriptor data_endp[] = {{
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x01,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = 64,
        .bInterval = 1,
}, {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x82,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = 64,
        .bInterval = 1,
} };

static const struct {
    struct usb_cdc_header_descriptor header;
    struct usb_cdc_call_management_descriptor call_mgmt;
    struct usb_cdc_acm_descriptor acm;
    struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
        .header = {
                .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
                .bDescriptorType = CS_INTERFACE,
                .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
                .bcdCDC = 0x0110,
        },
        .call_mgmt = {
                .bFunctionLength =
                        sizeof(struct usb_cdc_call_management_descriptor),
                        .bDescriptorType = CS_INTERFACE,
                        .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
                        .bmCapabilities = 0,
                        .bDataInterface = 1,
        },
        .acm = {
                .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
                .bDescriptorType = CS_INTERFACE,
                .bDescriptorSubtype = USB_CDC_TYPE_ACM,
                .bmCapabilities = 0,
        },
        .cdc_union = {
                .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
                .bDescriptorType = CS_INTERFACE,
                .bDescriptorSubtype = USB_CDC_TYPE_UNION,
                .bControlInterface = 0,
                .bSubordinateInterface0 = 1,
        }
};

static const struct usb_interface_descriptor comm_iface[] = {{
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CLASS_CDC,
        .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
        .iInterface = 0,

        .endpoint = comm_endp,

        .extra = &cdcacm_functional_descriptors,
        .extralen = sizeof(cdcacm_functional_descriptors)
} };

static const struct usb_interface_descriptor data_iface[] = {{
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 1,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_CLASS_DATA,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface = 0,

        .endpoint = data_endp,
} };

static const struct usb_interface ifaces[] = {{
        .num_altsetting = 1,
        .altsetting = comm_iface,
}, {
        .num_altsetting = 1,
        .altsetting = data_iface,
} };

static const struct usb_config_descriptor config = {
        .bLength = USB_DT_CONFIGURATION_SIZE,
        .bDescriptorType = USB_DT_CONFIGURATION,
        .wTotalLength = 0,
        .bNumInterfaces = 2,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = 0x80,
        .bMaxPower = 0x32,

        .interface = ifaces,
};

static const char * usb_strings[] = {
        "Elemental Industries",
        "Cortex Guitar Board",
        serialNumber,
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static int cdcacm_control_request(usbd_device *usbd_dev,
        struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
        usbd_control_complete_callback *complete)
{
    (void)complete;
    (void)buf;
    (void)usbd_dev;

    switch (req->bRequest) {
    case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
        /*
         * This Linux cdc_acm driver requires this to be implemented
         * even though it's optional in the CDC spec, and we don't
         * advertise it in the ACM functional descriptor.
         */
        return 1;
    }
    case USB_CDC_REQ_SET_LINE_CODING:
        if (*len < sizeof(struct usb_cdc_line_coding)) {
            return 0;
        }

        return 1;
    }
    return 0;
}

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
    (void)ep;

    char buf[64];
    int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

    if (len) {
        while (usbd_ep_write_packet(usbd_dev, 0x82, buf, len) == 0);
    }
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
    (void)wValue;

    usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, EP_BUFFER_SIZE,
            cdcacm_data_rx_cb);
    usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, EP_BUFFER_SIZE, NULL);
    usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

    usbd_register_control_callback(
            usbd_dev,
            USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
            USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
            cdcacm_control_request);

    ready = true;
}

static void reset_callback(void)
{
    ready = false;
}

static void suspend_callback(void)
{
    ready = false;
}

static void resume_callback(void)
{
    ready = true;
}

void usbInit(void)
{
    rcc_periph_clock_enable(RCC_OTGFS);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

    desig_get_unique_id_as_string(serialNumber, sizeof(serialNumber));

    usbd_dev = usbd_init(&otgfs_usb_driver, &dev, &config,
            usb_strings, 3,
            usbd_control_buffer, sizeof(usbd_control_buffer));

    usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
    usbd_register_reset_callback(usbd_dev, reset_callback);
    usbd_register_suspend_callback(usbd_dev, suspend_callback);
    usbd_register_resume_callback(usbd_dev, resume_callback);

    nvic_enable_irq(NVIC_OTG_FS_IRQ);
    nvic_set_priority(NVIC_OTG_FS_IRQ, 0xff);

    // Turn off VBUS sensing logic, forcing USB to peripheral mode.
    // The VBUS pin (PA9) is not connected on this board.
    OTG_FS_GCCFG |= OTG_GCCFG_NOVBUSSENS;
}

// USB interrupt
void otg_fs_isr(void)
{
    usbd_poll(usbd_dev);
}

ssize_t _write(int fd __attribute__((unused)), const void* buf, size_t count)
{
    // NOTE: Logging over USB is disabled right now. I can't get it to work
    // reliably.
    return count;

    if (!ready) {
        return count;
    }

    if (count > EP_BUFFER_SIZE) {
        count = EP_BUFFER_SIZE;
    }

    // The write operation will return 0 ("busy") if the previous transfer on
    // the EP hasn't finished yet -- so we need to loop until it's not busy.
    // This will also happen when nobody is listening on the host side (i.e.
    // /dev/ttyACM0 is not being read) -- so we need to give up after a while.
    for (int i = 0; i < 10000; i++) {
        nvic_disable_irq(NVIC_OTG_FS_IRQ);
        int res = usbd_ep_write_packet(usbd_dev, 0x82, buf, count);
        usbd_poll(usbd_dev);
        nvic_enable_irq(NVIC_OTG_FS_IRQ);
        if (res != 0) {
            return res;
        }
    }

    return count;
}
