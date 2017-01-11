
#include "config.h"
#include "usbcdc.h"

#ifdef USE_USBCDC
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/stm32/desig.h>

#define DEV_VID    0x0483 /* ST Microelectronics */
#define DEV_PID    0x5740 /* STM32 */
#define DEV_VER    0x0001 /* 0.1 */

#define EP_INT     0x83
#define EP_OUT     0x82
#define EP_IN      0x01

#define STR_MAN    0x01
#define STR_PROD   0x02
#define STR_SER    0x03

#define NULL ((void*) 0)

static const struct usb_device_descriptor dev = {
  .bLength            = USB_DT_DEVICE_SIZE,
  .bDescriptorType    = USB_DT_DEVICE,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = USB_CLASS_CDC,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = USBCDC_PKT_SIZE_DAT,
  .idVendor           = DEV_VID,
  .idProduct          = DEV_PID,
  .bcdDevice          = DEV_VER,
  .iManufacturer      = STR_MAN,
  .iProduct           = STR_PROD,
  .iSerialNumber      = STR_SER,
  .bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec its
 * optional, but its absence causes a NULL pointer dereference in Linux
 * cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
  .bLength            = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType    = USB_DT_ENDPOINT,
  .bEndpointAddress   = EP_INT,
  .bmAttributes       = USB_ENDPOINT_ATTR_INTERRUPT,
  .wMaxPacketSize     = USBCDC_PKT_SIZE_INT,
  .bInterval          = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
  .bLength            = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType    = USB_DT_ENDPOINT,
  .bEndpointAddress   = EP_IN,
  .bmAttributes       = USB_ENDPOINT_ATTR_BULK,
  .wMaxPacketSize     = USBCDC_PKT_SIZE_DAT,
  .bInterval          = 1,
}, {
  .bLength            = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType    = USB_DT_ENDPOINT,
  .bEndpointAddress   = EP_OUT,
  .bmAttributes       = USB_ENDPOINT_ATTR_BULK,
  .wMaxPacketSize     = USBCDC_PKT_SIZE_DAT,
  .bInterval          = 1,
}};

static const struct {
  struct usb_cdc_header_descriptor header;
  struct usb_cdc_call_management_descriptor call_mgmt;
  struct usb_cdc_acm_descriptor acm;
  struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
  .header = {
    .bFunctionLength    = sizeof(struct usb_cdc_header_descriptor),
    .bDescriptorType    = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
    .bcdCDC = 0x0110,
  },
  .call_mgmt = {
    .bFunctionLength    = sizeof(struct usb_cdc_call_management_descriptor),
    .bDescriptorType    = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
    .bmCapabilities     = 0,
    .bDataInterface     = 1,
  },
  .acm = {
    .bFunctionLength    = sizeof(struct usb_cdc_acm_descriptor),
    .bDescriptorType    = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_ACM,
    .bmCapabilities     = 0,
  },
  .cdc_union = {
    .bFunctionLength    = sizeof(struct usb_cdc_union_descriptor),
    .bDescriptorType    = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_UNION,
    .bControlInterface  = 0,
    .bSubordinateInterface0 = 1,
   },
};

static const struct usb_interface_descriptor comm_iface[] = {{
  .bLength              = USB_DT_INTERFACE_SIZE,
  .bDescriptorType      = USB_DT_INTERFACE,
  .bInterfaceNumber     = 0,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 1,
  .bInterfaceClass      = USB_CLASS_CDC,
  .bInterfaceSubClass   = USB_CDC_SUBCLASS_ACM,
  .bInterfaceProtocol   = USB_CDC_PROTOCOL_AT,
  .iInterface           = 0,

  .endpoint             = comm_endp,

  .extra                = &cdcacm_functional_descriptors,
  .extralen             = sizeof(cdcacm_functional_descriptors),
}};

static const struct usb_interface_descriptor data_iface[] = {{
  .bLength              = USB_DT_INTERFACE_SIZE,
  .bDescriptorType      = USB_DT_INTERFACE,
  .bInterfaceNumber     = 1,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 2,
  .bInterfaceClass      = USB_CLASS_DATA,
  .bInterfaceSubClass   = 0,
  .bInterfaceProtocol   = 0,
  .iInterface           = 0,

  .endpoint             = data_endp,
}};

static const struct usb_interface ifaces[] = {{
  .num_altsetting       = 1,
  .altsetting           = comm_iface,
}, {
  .num_altsetting       = 1,
  .altsetting           = data_iface,
}};

static const struct usb_config_descriptor config = {
  .bLength              = USB_DT_CONFIGURATION_SIZE,
  .bDescriptorType      = USB_DT_CONFIGURATION,
  .wTotalLength         = 0,
  .bNumInterfaces       = 2,
  .bConfigurationValue  = 1,
  .iConfiguration       = 0,
  .bmAttributes         = 0x80,
  .bMaxPower            = 0x32,

  .interface            = ifaces,
};

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[128];

static int cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
    uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
  (void) usbd_dev;
  (void) buf;
  (void) complete;
  switch (req->bRequest) {
  case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
    /*
     * This Linux cdc_acm driver requires this to be implemented
     * even though it's optional in the CDC spec, and we don't
     * advertise it in the ACM functional descriptor.
     */
    char local_buf[10];
    struct usb_cdc_notification *notif = (void *)local_buf;

    /* We echo signals back to host as notification. */
    notif->bmRequestType = 0xa1;
    notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
    notif->wValue        = 0;
    notif->wIndex        = 0;
    notif->wLength       = 2;
    local_buf[8]         = req->wValue & 3;
    local_buf[9]         = 0;
    return 1;
  }
  case USB_CDC_REQ_SET_LINE_CODING:
    if (*len < sizeof(struct usb_cdc_line_coding))
      return 0;
    return 1;
  }
  return 0;
}


static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue) {
  (void) wValue;
  usbd_ep_setup(usbd_dev, EP_IN , USB_ENDPOINT_ATTR_BULK, 64, NULL);
  usbd_ep_setup(usbd_dev, EP_OUT, USB_ENDPOINT_ATTR_BULK, 64, NULL);
  usbd_ep_setup(usbd_dev, EP_INT, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

  usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        cdcacm_control_request);
}

static usbd_device *usbd_dev; /* Just a pointer, need not to be volatile. */

static char serial[25];

/* Vendor, device, version. */
static const char *usb_strings[] = {
/*MAN */  "EndlessFork",
/*PROD*/  "STM32 CDC IF",
/*SER */  serial,
};

void setup_usbcdc(void) {
  desig_get_unique_id_as_string(serial, sizeof(serial));

  usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
  usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
//  usbd_register_reset_callback(usbd_dev, cdcacm_reset);
// XXX: register callback for reading RX ep....

  /* NOTE: Must be called after USB setup since this enables calling usbd_poll(). */
  /* NVIC_USB_HP_CAN_TX_IRQ ??? */
  nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
  nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);
}


void idle(void);

#include "buffer.h"
// forward declarations
#define USBCDC_TX_BUFFER_SIZE BUFFER_SIZE_256
#define USBCDC_RX_BUFFER_SIZE BUFFER_SIZE_256
BUFFER_H(USBCDC_TX_BUFFER, uint8_t, USBCDC_TX_BUFFER_SIZE)
BUFFER_H(USBCDC_RX_BUFFER, uint8_t, USBCDC_RX_BUFFER_SIZE)

// implementations
BUFFER(USBCDC_TX_BUFFER, uint8_t, USBCDC_TX_BUFFER_SIZE)
BUFFER(USBCDC_RX_BUFFER, uint8_t, USBCDC_RX_BUFFER_SIZE)

extern uint8_t systick_usbcdc_timeout_ctr;

void usbcdc_check_rx_buffer(void);
void usbcdc_check_tx_buffer(void);

void idle(void) {
   usbd_poll(usbd_dev);
   usbcdc_check_tx_buffer(); // may send tx_buffer data to USB (emptying buffer)
   usbcdc_check_rx_buffer(); // may copy incoming USB data to buffer, filling it)
}

#define USBCDC_TX_TIMEOUT_MS 10

void usbcdc_check_rx_buffer(void) {
   // if there is enough free space in RX-buffer, try to read a packet
   if (USBCDC_RX_BUFFER_used() < (USBCDC_RX_BUFFER_SIZE - USBCDC_PKT_SIZE_DAT)) {
      uint8_t buffer[USBCDC_PKT_SIZE_DAT];
      uint8_t bufsize, *p;
      bufsize = usbd_ep_read_packet(usbd_dev, EP_IN, buffer, USBCDC_PKT_SIZE_DAT);
      p = &buffer[0];
      // transfer data to RX buffer
      while (bufsize--) {
          USBCDC_RX_BUFFER_put(*p++);
      }
   }
}

/*
 * send the tx bytes before they get stale or too many
 * can only send up to the end of the buffer, the wraps the tail around
 * and can send further date next round
 * not optiml but small and working
 */
void usbcdc_check_tx_buffer(void) {
   uint8_t used_bytes = USBCDC_TX_BUFFER_used();
   uint8_t send_bytes = USBCDC_TX_BUFFER_SIZE - USBCDC_TX_BUFFER_tail;
   if (used_bytes) {
      // if bytes in tx buffer and timeout is up, or used_bytes is bigger than 1 paket
      if ((used_bytes >= USBCDC_PKT_SIZE_DAT) ||
          (!systick_usbcdc_timeout_ctr)) {
         // stop sending at end of buffer. warning: may perform slow!
         if (used_bytes < send_bytes)
            send_bytes = used_bytes;
         send_bytes = usbd_ep_write_packet(usbd_dev, EP_OUT,
                    &USBCDC_TX_BUFFER_data[USBCDC_TX_BUFFER_tail], send_bytes);
         if (send_bytes) {
            // if sending succeeded
            // correct tail
            USBCDC_TX_BUFFER_tail += send_bytes;
            USBCDC_TX_BUFFER_tail &= USBCDC_TX_BUFFER_SIZE -1;
            // restart tx timeout counter
            systick_usbcdc_timeout_ctr = USBCDC_TX_TIMEOUT_MS;
         }
      }
   }
}

/* application layer */
void usbcdc_putchar(char c) {
   USBCDC_TX_BUFFER_put(c); // will block if buffer is full
}

char usbcdc_can_putchar(void) {
   return USBCDC_TX_BUFFER_can_write();
}

char usbcdc_getchar(void) {
   // BLOCKING!
   return USBCDC_RX_BUFFER_get();
}

char usbcdc_can_getchar(void) {
   return USBCDC_RX_BUFFER_can_read();
}


/* Interrupts */

void usb_wakeup_isr(void) {
   idle();
}

void usb_hp_can_tx_isr(void) {
   idle();
}

void usb_lp_can_rx0_isr(void) {
   idle();
}


#include <stdio.h>
#include <unistd.h>
#include <errno.h>

// provide backend for stdout/stderr
int _write(int file, char *ptr, int len);
int _write(int file, char *ptr, int len) {
   int i;

   if (file == STDOUT_FILENO || file == STDERR_FILENO) {
      for (i = 0; i < len; i++) {
         usbcdc_putchar(ptr[i]);
      }
      return i;
   }
   errno = EIO;
   return -1;
}

// provide backend for stdin
int _read(int file, char *ptr, int len);
int _read(int file, char *ptr, int len) {
   int i;

   if (file == STDIN_FILENO) {
      for (i = 0; i < len; i++) {
          if (usbcdc_can_getchar())
             *ptr++ = usbcdc_getchar();
          else
             return i;
      }
   }
   errno = EIO;
   return -1;
}

#endif
