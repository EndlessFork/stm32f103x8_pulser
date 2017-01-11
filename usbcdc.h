#ifndef __USBCDC_H__
#define __USBCDC_H__

#include <stdint.h>

#define USBCDC_PKT_SIZE_DAT 64
#define USBCDC_PKT_SIZE_INT 16

//extern volatile uint8_t usb_ready;

extern void setup_usbcdc(void);
extern char usbcdc_can_getchar(void); // return 1 if getchar would not block
extern char usbcdc_can_putchar(void); // return 1 if putchar would not block
extern void usbcdc_putchar(char c); // blocking!
extern char usbcdc_getchar(void);

void idle(void);
#endif
