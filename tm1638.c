/*

 support for TM1638 based LED displays/keypads

 (8 7-segment digits, 8 extra leds, 8 keys)

*/

#include "tm1638.h"
#include "led_font.h"

/******************
 * internal stuff *
 ******************/
static void udelay(unsigned int microseconds) {
    int ctr;
    while (microseconds--) {
        for(ctr=72;ctr;--ctr) {
            asm("nop");
        }
    }
}

static void set_clk_low(void) {
    gpio_clear(TM1638_PORT, TM1638_CLK_PIN);
    udelay(1);
}

static void set_clk_high(void) {
    gpio_set(TM1638_PORT, TM1638_CLK_PIN);
    udelay(1);
}

static void set_stb_low(void) {
    gpio_clear(TM1638_PORT, TM1638_STB_PIN);
    udelay(1);
}

static void set_stb_high(void) {
    gpio_set(TM1638_PORT, TM1638_STB_PIN);
    udelay(1);
}

static void set_dio_low(void) {
    gpio_clear(TM1638_PORT, TM1638_DIO_PIN);
    udelay(1);
}

static void set_dio_high(void) {
    gpio_set(TM1638_PORT, TM1638_DIO_PIN);
    udelay(1);
}

static void set_dio_input(void) {
    // rely on external pull-up
    gpio_set_mode(TM1638_PORT,
                  GPIO_MODE_INPUT,
//                  GPIO_CNF_INPUT_FLOAT,
                  GPIO_CNF_INPUT_PULL_UPDOWN,
                  TM1638_DIO_PIN);
    gpio_set(TM1638_PORT, TM1638_DIO_PIN);
    udelay(1);
}

static void set_dio_output(void) {
    gpio_set_mode(TM1638_PORT,
                  GPIO_MODE_OUTPUT_10_MHZ,
                  GPIO_CNF_OUTPUT_OPENDRAIN,
                  TM1638_DIO_PIN);
    udelay(1);
}

static uint8_t get_dio_level(void) {
    udelay(1);
    return gpio_get(TM1638_PORT, TM1638_DIO_PIN) != 0;
}

static void sendbyte(uint8_t b) {
    uint8_t i;
    set_dio_high();
    for(i=0;i<8;i++) {
        set_clk_low();
        if (b & 1) {
            set_dio_high();
        } else {
            set_dio_low();
        }
        b >>= 1;
        set_clk_high();
    }
    set_dio_high();
}

static uint8_t readbyte(void) {
    uint8_t i, result=0;
    set_dio_input();
    for(i=0;i<8;i++) {
        set_clk_low();
        udelay(1);
        result >>= 1;
        if (get_dio_level()) {
            result |= 0x80;
        }
        set_clk_high();
    }
    return result;
}

/*************
 * interface *
 *************/
void tm1638_init(void) {
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_set_mode(TM1638_PORT,
                  GPIO_MODE_OUTPUT_10_MHZ,
//                  GPIO_CNF_OUTPUT_PUSHPULL,
                  GPIO_CNF_OUTPUT_OPENDRAIN,
                  TM1638_CLK_PIN | TM1638_DIO_PIN | TM1638_STB_PIN);
    set_stb_high();
    set_clk_high();
    set_stb_low();
    sendbyte(0x40);
    set_stb_high();
    udelay(1); // wait 1us
    tm1638_set_brightness(8);
    tm1638_clear();
}

void tm1638_set_brightness(uint8_t level) {
    set_stb_low();
    if (level) {
        sendbyte(0x88 | ((level-1) & 0x07));
    } else {
        sendbyte(0x80);
    }
    set_stb_high();
}

void tm1638_set_raw_digit(uint8_t pos, uint8_t pattern);
void tm1638_set_raw_digit(uint8_t pos, uint8_t pattern) {
    set_stb_low();
    sendbyte(0xc0 | ((pos & 7)<<1));
    sendbyte(pattern);
    set_stb_high();
}

void tm1638_set_digit(uint8_t pos, char c) {
    tm1638_set_raw_digit(pos, ((c > 31) && (c < 128)) ? LED_FONT[c-32] : 0);
}

void tm1638_set_led(uint8_t pos, uint8_t color) {
    set_stb_low();
    sendbyte(0xc1 | ((pos & 7)<<1));
    sendbyte(color);
    set_stb_high();
}

void tm1638_put_string(uint8_t pos , char* s) {
    // try to collapse '.' and ',' into the previous char if possible ???
    uint8_t pattern;
    char c;
    while ((pos < 8) && *s) {
        c = *s++;
        pattern = ((c > 31) && (c < 128)) ? LED_FONT[c-32] : 0;
        if ((*s == '.') || (*s == ',')) {
            pattern |= 0x80; // 0x80 activates the dot...
            s++;
        }
        tm1638_set_raw_digit(pos++, pattern);
    }
}

void tm1638_clear(void) {
    uint8_t i;
    set_stb_low();
    sendbyte(0xc0);
    for (i=0;i<16;i++) {
        sendbyte(0);
    }
    set_stb_high();
}

uint32_t tm1638_read_keys(void) {
    uint32_t result = 0;
    uint8_t b;
    set_stb_low();
    sendbyte(0x42);
    // read 4 bytes. each contains 6 used bits. also store to right position in result 
    // even if only 8 keys are connected we still have to read all 4 bytes :(
    b = readbyte();
    if (b & 0x01) result |= 0x000001;
    if (b & 0x02) result |= 0x000100;
    if (b & 0x04) result |= 0x010000;
    if (b & 0x10) result |= 0x000010;
    if (b & 0x20) result |= 0x001000;
    if (b & 0x40) result |= 0x100000;
    b = readbyte();
    if (b & 0x01) result |= 0x000002;
    if (b & 0x02) result |= 0x000200;
    if (b & 0x04) result |= 0x020000;
    if (b & 0x10) result |= 0x000020;
    if (b & 0x20) result |= 0x002000;
    if (b & 0x40) result |= 0x200000;
    b = readbyte();
    if (b & 0x01) result |= 0x000004;
    if (b & 0x02) result |= 0x000400;
    if (b & 0x04) result |= 0x040000;
    if (b & 0x10) result |= 0x000040;
    if (b & 0x20) result |= 0x004000;
    if (b & 0x40) result |= 0x400000;
    b = readbyte();
    if (b & 0x01) result |= 0x000008;
    if (b & 0x02) result |= 0x000800;
    if (b & 0x04) result |= 0x080000;
    if (b & 0x10) result |= 0x000080;
    if (b & 0x20) result |= 0x008000;
    if (b & 0x40) result |= 0x800000;
    set_dio_output();
    set_stb_high();
    set_stb_low();
    sendbyte(0x40);
    set_stb_high();
    return result;
}
