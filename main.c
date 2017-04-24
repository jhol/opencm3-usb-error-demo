/**
 * @file      main.c
 * @copyright VCA Technology
 * @~english
 * @brief     Implementation of the USB error test code for the STM32F042
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/dfu.h>
#include <libopencm3/usb/usbd.h>

#define CDCACM_1_COMM_IFACE_NUM 0
#define CDCACM_1_DATA_IFACE_NUM 1
#define DFU_IFACE_NUM 2

#define EP_CDCACM_1_DATA_OUT 0x01
#define EP_CDCACM_1_DATA_IN 0x82
#define EP_CDCACM_1_COMM 0x83

#define CONFIG_DI_TYPE 0x0001

static const struct usb_device_descriptor dev = {
  .bLength = USB_DT_DEVICE_SIZE,
  .bDescriptorType = USB_DT_DEVICE,
  .bcdUSB = 0x0200,
  .bDeviceClass = 0,
  .bDeviceSubClass = 0,
  .bDeviceProtocol = 0,
  .bMaxPacketSize0 = 64,
  .idVendor = 0x0925,
  .idProduct = 0xD100,
  .bcdDevice = 0,
  .iManufacturer = 0,
  .iProduct = 0,
  .iSerialNumber = 0,
  .bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec its
 * optional, but its absence causes a NULL pointer dereference in Linux
 * cdc_acm driver.
 */
static const struct usb_endpoint_descriptor cdcacm_comm_endp[][1] = {{
  {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = EP_CDCACM_1_COMM,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 16,
    .bInterval = 255,
  }
}};

static const struct usb_endpoint_descriptor cdcacm_data_endp[][2] = {{
  {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = EP_CDCACM_1_DATA_OUT,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
  }, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = EP_CDCACM_1_DATA_IN,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
  }
}};

static const struct {
  struct usb_cdc_header_descriptor header;
  struct usb_cdc_call_management_descriptor call_mgmt;
  struct usb_cdc_acm_descriptor acm;
  struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors[] = {{
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
  },
}, {
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
    .bDataInterface = 3,
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
    .bControlInterface = 2,
    .bSubordinateInterface0 = 3,
  },
}};

static const struct usb_interface_descriptor cdcacm_comm_iface[] = {{
  .bLength = USB_DT_INTERFACE_SIZE,
  .bDescriptorType = USB_DT_INTERFACE,
  .bInterfaceNumber = CDCACM_1_COMM_IFACE_NUM,
  .bAlternateSetting = 0,
  .bNumEndpoints = 1,
  .bInterfaceClass = USB_CLASS_CDC,
  .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
  .bInterfaceProtocol = USB_CDC_PROTOCOL_NONE,
  .iInterface = 0,

  .endpoint = cdcacm_comm_endp[0],

  .extra = &cdcacm_functional_descriptors[0],
  .extralen = sizeof(cdcacm_functional_descriptors[0]),
}};

static const struct usb_interface_descriptor cdcacm_data_iface[] = {{
  .bLength = USB_DT_INTERFACE_SIZE,
  .bDescriptorType = USB_DT_INTERFACE,
  .bInterfaceNumber = CDCACM_1_DATA_IFACE_NUM,
  .bAlternateSetting = 0,
  .bNumEndpoints = 2,
  .bInterfaceClass = USB_CLASS_DATA,
  .bInterfaceSubClass = 0,
  .bInterfaceProtocol = 0,
  .iInterface = 0,

  .endpoint = cdcacm_data_endp[0],
}};

static const struct usb_dfu_descriptor dfu_function = {
  .bLength = sizeof(struct usb_dfu_descriptor),
  .bDescriptorType = DFU_FUNCTIONAL,
  .bmAttributes = USB_DFU_CAN_DOWNLOAD | USB_DFU_WILL_DETACH,
  .wDetachTimeout = 255,
  .wTransferSize = 1024,
  .bcdDFUVersion = 0x0000,
};

static const struct usb_interface_descriptor dfu_iface[] = {{
  .bLength = USB_DT_INTERFACE_SIZE,
  .bDescriptorType = USB_DT_INTERFACE,
  .bInterfaceNumber = DFU_IFACE_NUM,
  .bAlternateSetting = 0,
  .bNumEndpoints = 0,
  .bInterfaceClass = 0xFE, /* Device Firmware Upgrade */
  .bInterfaceSubClass = 1,
  .bInterfaceProtocol = 0,

  /* The ST Microelectronics DfuSe application needs this string.
  *    * The format isn't documented... */
  .iInterface = 4,

  .extra = &dfu_function,
  .extralen = sizeof(dfu_function),
}};

static const struct usb_interface ifaces[] = {{
  .num_altsetting = 1,
  .altsetting = &cdcacm_comm_iface[0],
}, {
  .num_altsetting = 1,
  .altsetting = &cdcacm_data_iface[0],
}, {
  .num_altsetting = 1,
  .altsetting = dfu_iface,
}};

static const struct usb_config_descriptor config = {
  .bLength = USB_DT_CONFIGURATION_SIZE,
  .bDescriptorType = USB_DT_CONFIGURATION,
  .wTotalLength = 0,
  .bNumInterfaces = sizeof(ifaces) / sizeof(ifaces[0]),
  .bConfigurationValue = 1,
  .iConfiguration = 0,
  .bmAttributes = 0x80,
  .bMaxPower = 0x32,

  .interface = ifaces,
};

static uint16_t usbd_control_buffer[128];
static bool config_complete = false;

static void clock_setup(void) {
  rcc_clock_setup_in_hsi48_out_48mhz();
  crs_autotrim_usb_enable();
  rcc_set_usbclk_source(RCC_HSI48);

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
}

static int vendor_control_callback(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
    uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
  (void)buf;
  (void)len;
  (void)complete;
  (void)usbd_dev;

  return USBD_REQ_HANDLED;
}

static void usb_reset(void) {
  config_complete = false;
}

static void usb_set_config(usbd_device *usbd_dev, uint16_t wValue) {
  (void)wValue;

  usbd_register_control_callback(usbd_dev, USB_REQ_TYPE_VENDOR, USB_REQ_TYPE_TYPE, vendor_control_callback);

  config_complete = true;
}

usbd_device* usb_setup(void) {
  unsigned int i;
  unsigned char serial = 0;

  gpio_set(GPIOA, GPIO1);
  gpio_clear(GPIOB, GPIO6 | GPIO7);
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6 | GPIO7);

  /* Delay briefly to give bus enough time detect reconnection */
  for (i = 0; i != 4096; i++)
    __asm__("nop");

  /* Initialize USB slave. */
  usbd_device *const usbd_dev = usbd_init(&st_usbfs_v2_usb_driver, &dev, &config, NULL,
    0, (uint8_t*)usbd_control_buffer, sizeof(usbd_control_buffer));
  usbd_register_reset_callback(usbd_dev, usb_reset);
  usbd_register_set_config_callback(usbd_dev, usb_set_config);
  return usbd_dev;
}

int main(void) {
  const int delay = 256;

  unsigned int i;

  clock_setup();
  usbd_device *const usbd_dev = usb_setup();

  while (1) {
    usbd_poll(usbd_dev);

    if (config_complete) {
      for (i = 0; i != delay; i++)
       __asm__("nop");
    }
  }

  return 0;
}
