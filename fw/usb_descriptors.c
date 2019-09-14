#include <libopencm3/usb/usbd.h>
#include <stdlib.h>

const struct usb_device_descriptor usb_dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
        .bcdUSB = 0x0110,
        .bDeviceClass = USB_CLASS_VENDOR,
	.bDeviceSubClass = 0,
        .bDeviceProtocol = 0,
        .bMaxPacketSize0 = 64,
        .idVendor = 0x10c4, // Silicon Labs
        .idProduct = 0x8b00, // assigned by SiLabs to Harmon Instruments
        .bcdDevice = 0x0001,
        .iManufacturer = 1,
	.iProduct = 2,
        .iSerialNumber = 3,
        .bNumConfigurations = 1,
};

const struct usb_endpoint_descriptor endp_bulk[] = {
        {
                .bLength = USB_DT_ENDPOINT_SIZE,
                .bDescriptorType = USB_DT_ENDPOINT,
                .bEndpointAddress = 0x01,
                .bmAttributes = USB_ENDPOINT_ATTR_BULK,
                .wMaxPacketSize = 64,
                .bInterval = 0,
        },
        {
                .bLength = USB_DT_ENDPOINT_SIZE,
                .bDescriptorType = USB_DT_ENDPOINT,
                .bEndpointAddress = 0x82,
                .bmAttributes = USB_ENDPOINT_ATTR_BULK,
                .wMaxPacketSize = 64,
                .bInterval = 0,
        }
};

const struct usb_interface_descriptor usb_interface_desc[] = {
        {
                .bLength = USB_DT_INTERFACE_SIZE,
                .bDescriptorType = USB_DT_INTERFACE,
                .bInterfaceNumber = 0,
                .bAlternateSetting = 0,
                .bNumEndpoints = 2,
                .bInterfaceClass = USB_CLASS_VENDOR,
                .bInterfaceSubClass = 0,
                .bInterfaceProtocol = 0,
                .iInterface = 0,
                .endpoint = endp_bulk,
                .extra = NULL,
                .extralen = 0
        }
};

const struct usb_interface usb_interfaces[] = {
        {
	.num_altsetting = 1,
	.altsetting = usb_interface_desc
        }
};

const struct usb_config_descriptor usb_config_descr =
{
        .bLength = USB_DT_CONFIGURATION_SIZE,
        .bDescriptorType = USB_DT_CONFIGURATION,
        .wTotalLength = 0,
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0, // string index
        .bmAttributes = 0x80,
        .bMaxPower = 50, // 2 mA increment
        .interface = usb_interfaces,
};
