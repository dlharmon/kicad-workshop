#include <algorithm>
#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/efm32/cmu.h>
#include <libopencm3/efm32/common/adc_common.h>
#include <libopencm3/efm32/common/i2c_common.h>
#include <libopencm3/efm32/gpio.h>
#include <libopencm3/efm32/hg/nvic.h>
#include <libopencm3/efm32/memorymap.h>
#include <libopencm3/efm32/timer.h>
#include <libopencm3/efm32/usb.h>
#include <libopencm3/usb/usbd.h>
#include <stdlib.h>
#include "font.h"

// R: PB7 TIM1 CC0
// G: PB8 TIM1 CC1
// B: PB11 TIM1 CC2

// ENCS: PA0 - pullup, act low
// ENCA: PE12 TIM2_CC2
// ENCB: PE13 TIM2_CC3
// SDA: PC0
// SCL: PC1
// USB: PC14, 15

#define SCL_PORT GPIOC // I2C0 AF4
#define SDA_PORT GPIOC
#define SCL_PIN GPIO1
#define SDA_PIN GPIO0

#define INT_PORT GPIOA
#define INT_PIN GPIO0

#define I2C_READ 0x01

#define TIMER_US 24
#define TIMER_MS 24000

class Timer {
private:
        uint32_t begin;
        uint32_t etime;

public:
        Timer() { reset(); }
        void reset() {
                begin = STK_CVR;
                etime = 0;
        }
        uint32_t elapsed() {
                __sync_synchronize();
                uint32_t old_etime = etime;
                etime = (etime & 0xFF000000) | (0x00FFFFFF & (begin - STK_CVR));
                if (etime < old_etime)
                        etime += 0x1000000;
                return etime;
        }
};

void delay(uint32_t cycles) {
        Timer t;
        while (t.elapsed() < cycles)
                ;
}

#define I2C_ADDR_OLED 0x78

static int i2c_transaction(uint8_t addr, const uint8_t *wdata, size_t wcount,
                           uint8_t *rdata, uint32_t rcount) {
        Timer t{};
        uint32_t timeout = 100 * TIMER_MS;
        // wait for the peripheral to be idle
        while ((I2C0_STATE & 0xE0) != 0) {
                if (t.elapsed() > timeout)
                        goto fail;
        }
        I2C0_CMD = I2C_CMD_CLEARTX | I2C_CMD_CLEARPC;
        I2C0_CMD = I2C_CMD_START;
        // I2C address
        while ((I2C0_STATUS & I2C_STATUS_TXBL) == 0) {
                if (t.elapsed() > timeout)
                        goto fail;
        }
        I2C0_TXDATA = I2C_ADDR_OLED;
        // register address
        while ((I2C0_STATUS & I2C_STATUS_TXBL) == 0) {
                if (t.elapsed() > timeout)
                        goto fail;
        }
        I2C0_TXDATA = addr;
        // data
        for (size_t i = 0; i < wcount; i++) {
                while ((I2C0_STATUS & I2C_STATUS_TXBL) == 0) {
                        if (t.elapsed() > timeout)
                                goto fail;
                }
                I2C0_TXDATA = wdata[i];
        }
        // wait for transmission to complete
        while ((I2C0_STATUS & I2C_STATUS_TXC) == 0) {
                if (t.elapsed() > timeout)
                        goto fail;
        }
        if (rcount != 0) {
                I2C0_CMD = I2C_CMD_CLEARTX | I2C_CMD_CLEARPC;
                I2C0_CMD = I2C_CMD_START;
                while ((I2C0_STATUS & I2C_STATUS_TXBL) == 0) {
                        if (t.elapsed() > timeout)
                                goto fail;
                }
                I2C0_TXDATA = I2C_ADDR_OLED | I2C_READ;
        }
        for (size_t i = 0; i < rcount; i++) {
                while ((I2C0_STATUS & I2C_STATUS_RXDATAV) == 0) {
                        if (t.elapsed() > timeout)
                                goto fail;
                }
                rdata[i] = I2C0_RXDATA;
                if ((i + 1) == rcount)
                        I2C0_CMD = I2C_CMD_NACK;
                else
                        I2C0_CMD = I2C_CMD_ACK;
        }
        I2C0_CMD = I2C_CMD_STOP;
        return 0;
fail:
        I2C0_CMD = I2C_CMD_ABORT;
        return -1;
}

static void i2c_write(uint8_t addr, uint8_t data) {
        i2c_transaction(addr, &data, 1, NULL, 0);
}

static void i2c_write(uint8_t addr, const void *data, int count) {
        i2c_transaction(addr, (uint8_t *)data, count, NULL, 0);
}

static uint32_t read_adc(uint32_t input, uint32_t ref) {
        ADC0_SINGLECTRL =
            (5 << 20) | ((ref & 0x7) << 16) | ((input & 0xF) << 8);
        ADC0_CMD = ADC_CMD_SINGLESTART;
        while ((ADC0_STATUS & ADC_STATUS_SINGLEDV) == 0)
                ;
        return ADC0_SINGLEDATA;
}

static uint32_t correct_temp(uint32_t temp) {
        int cal_v = DI_ADC0_TEMP_0_READ_1V25 >> 4;
        int tc = temp - cal_v;
        // 1/6.3 deg C per ADC code, multiply by
        // 10*65536
        tc *= 104025;
        tc >>= 16;
        tc = DI_CAL_TEMP_0 * 10 - tc;
        // tc is now 10 * temp in deg C
        return tc;
}

static void out_cb(usbd_device *usbd_dev, uint8_t ep) {
        uint32_t buf[16] __attribute__((aligned(4)));
        uint16_t len = usbd_ep_read_packet(usbd_dev, ep, (uint8_t *)buf, 64);
        if (len < 8)
                return;
        if (buf[0] == 0) {
                uint32_t rv[4];
                rv[0] = DI_UNIQUE_0;
                rv[1] = DI_UNIQUE_1;
                rv[2] = DI_MEM_INFO_FLASH | (DI_MEM_INFO_RAM << 16);
                rv[3] = DI_PART_NUMBER | (DI_PART_FAMILY << 16) |
                        (DI_PROD_REV << 24);
                usbd_ep_write_packet(usbd_dev, 0x82, &rv[0], sizeof(rv));
                return;
        }
        if (buf[0] == 3) {
                uint32_t rv = read_adc(buf[1], buf[2]);
                if (buf[1] == 8) {
                        // it's the temp sensor, convert to deg C
                        rv = correct_temp(rv);
                }
                usbd_ep_write_packet(usbd_dev, 0x82, &rv, 4);
                return;
        }
        if (buf[0] == 4) {
                TIMER1_CC0_CCV = buf[1] & 0xFFFF;
                TIMER1_CC1_CCV = buf[1] >> 16;
                TIMER1_CC2_CCV = buf[2] & 0xFFFF;
                return;
        }
        if (buf[0] == 5) {
                uint32_t rv = 0xDEADBEEF;
                usbd_ep_write_packet(usbd_dev, 0x82, &rv, 4);
                return;
        }
        return;
}

static char serial_string[17] = "serial";
static const char manufacturer_string[] = "Denhac";
static const char product_string[] = "Kicad workshop";

static const char *usb_strings[] = {
    manufacturer_string,
    product_string,
    serial_string,
};

extern "C" const struct usb_device_descriptor usb_dev_descr;
extern "C" const struct usb_endpoint_descriptor endp_bulk[2];
extern "C" const struct usb_interface_descriptor usb_interface_desc[1];
extern "C" const struct usb_interface usb_interfaces[1];
extern "C" const struct usb_config_descriptor usb_config_descr;

static uint8_t usbd_control_buffer[128];

static void usb_set_config(usbd_device *usbd_dev, uint16_t wValue) {
        (void)wValue;
        usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, out_cb);
        usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
}

static usbd_device *usbd_dev;

void usb_isr(void) { usbd_poll(usbd_dev); }

static void hex_str(uint32_t x, char *s) {
        for (int i = 0; i < 8; i++) {
                uint32_t hexit = x >> 28;
                if (hexit < 10)
                        s[i] = '0' + hexit;
                else
                        s[i] = 'A' + (hexit - 10);
                x <<= 4;
        }
}

#include <stdio.h>

static void temperature_str(uint32_t x, char *s) {
        s[7] = 0;
        s[6] = 'C';
        s[5] = ' ';
        s[4] = '0' + (x % 10);
        s[3] = '.';
        for (int i = 2; i >= 0; i--) {
                x /= 10;
                s[i] = '0' + (x % 10);
        }
}

int isr_count = 0;

void gpio_even_isr(void) { GPIO_IFC = 0xFFFF; }

class Encoder {
private:
        uint32_t angle = 0;
        int last_change = 0;
        int pins_prev = 3;
        uint32_t get_pins() { return 0x3 & (GPIO_PE_DIN >> 12); }

public:
        bool pushed_raw() { return !(0x1 & (GPIO_PA_DIN >> 0)); }
        void update() {
                int p = get_pins();
                if (p == pins_prev)
                        return;
                // 0 rose
                if ((p & 1) && (!(pins_prev & 1)))
                        angle += (p & 2) ? -1 : 1;
                // 0 fell
                else if ((!(p & 1)) && (pins_prev & 1))
                        angle += (p & 2) ? 1 : -1;
                pins_prev = p;
        }
        uint32_t get_angle() { return angle; }
} encoder;

// this interrupt fires every 2.730666 ms (65536 / 24 MHz)
void timer1_isr(void) {
        TIMER1_IFC = 1; // clear the interrupt
        encoder.update();
        isr_count++;
}

// each byte is a column of 8 pixels
uint8_t fb[1024];

static void OLED_text(const char *const text, int startpos,
                      uint8_t inverse = 0) {
        for (size_t i = 0; text[i] != 0; i++) {
                if (startpos + 6 * i + 6 >= sizeof(fb))
                        break;
                for (int j = 0; j < 5; j++) {
                        fb[startpos + (6 * i) + j] =
                            inverse ^ font[text[i] * 5 + j];
                }
                fb[startpos + (6 * i) + 5] = inverse;
        }
}

// update the OLED display with the frame buffer
static void OLED_update() {
        for (int i = 0; i < 8; i++) {
                i2c_write(0, 0xB0 + i); // set page
                i2c_write(0, 0x02);     // set lower column address
                i2c_write(0, 0x10);     // set higher column address
                // write page
                i2c_transaction(0x40, &fb[128 * i], 128, NULL, 0);
        }
}

static void OLED_init() {
        // loosely based on
        // https://github.com/bitbank2/oled_96/blob/master/oled96.c

        constexpr uint8_t oled64_init[] = {
            0xae,       // display off
            0xd5,       // set display clock div
            0x80,       // clock div value
            0xa8,       // set multiplex
            0x3f,       // multiplex value
            0xd3,       // set display offset
            0x00,       // display offset
            0x40,       // set start line to 0
            0x8d,       // set charge pump
            0x14,       // charge pump internal VCC?
            0xa1,       // segment remap
            0xc8,       // scan direction
            0xda,       // set com pins HW config
            0x12,       // arg for above ?
            0x81,       // set contrast Bank 0
            0xcf,       // arg for above ?
            0xd9,       // set precharge
            0xf1,       // precharge arg?
            0xdb,       // VCOMH deslect level
            0x40,       // VCOMH arg?
            0xa4,       // not forced to all on
            0xa6,       // not inverse
            0x20, 0x00, // memory addressing mode
            0xaf        // display on
        };

        i2c_write(0, oled64_init, sizeof(oled64_init));

        OLED_text(serial_string, 0);
        OLED_text("Denhac Kicad Workshop", 256);
        OLED_text("Temp = ", 384);
}

int main(void) {
        // use the 24 MHz USB clock as our main clock
        CMU_CMD = CMU_CMD_USBCCLKSEL_USHFRCO | (5 << 0);

        CMU_HFPERCLKEN0 = CMU_HFPERCLKEN0_I2C0 | CMU_HFPERCLKEN0_GPIO |
                          CMU_HFPERCLKEN0_ADC0 | CMU_HFPERCLKEN0_TIMER1 |
                          CMU_HFPERCLKEN0_TIMER2;

        // configure GPIOs

        // RGB LED
        // R: PB7 TIM1 CC0
        // G: PB8 TIM1 CC1
        // B: PB11 TIM1 CC2
        gpio_mode_setup(GPIOB, GPIO_MODE_PUSH_PULL, GPIO7 | GPIO8 | GPIO11);

        // ENCA: PE12 TIM2_CC2
        // ENCB: PE13 TIM2_CC3
        // ENCS: PA0 - pullup, act low
        gpio_mode_setup(GPIOE, GPIO_MODE_INPUT_PULL_FILTER, GPIO12 | GPIO13);
        GPIO_P_DOUTSET(GPIOE) = GPIO12 | GPIO13;
        gpio_mode_setup(INT_PORT, GPIO_MODE_INPUT_PULL_FILTER, INT_PIN);
        GPIO_P_DOUTSET(INT_PORT) = INT_PIN;

        TIMER1_CTRL = (1 << 6); // run in debug
        TIMER1_ROUTE = (3 << 16) | 7;
        TIMER1_TOP = 0xFFFF;
        // set all 3 outputs to inverted PWM
        TIMER1_CC0_CTRL = TIMER_CC_CTRL_MODE_PWM | TIMER_CC_CTRL_OUTINV;
        TIMER1_CC1_CTRL = TIMER_CC_CTRL_MODE_PWM | TIMER_CC_CTRL_OUTINV;
        TIMER1_CC2_CTRL = TIMER_CC_CTRL_MODE_PWM | TIMER_CC_CTRL_OUTINV;
        TIMER1_CC0_CCV = 0x50;
        TIMER1_CC1_CCV = 0x0;
        TIMER1_CC2_CCV = 0x50;
        TIMER1_IEN = 1; // overflow interrupt enabled

        TIMER1_CMD = 1;

        GPIO_EXTIPSELL = 0;       // PA0 as ext interrupt
        GPIO_EXTIFALL = (1 << 0); // falling
        GPIO_EXTIRISE = (1 << 0); // rising
        GPIO_IEN = (1 << 0);

        // I2C0
        GPIO_P_DOUTSET(SCL_PORT) = SCL_PIN;
        gpio_mode_setup(SCL_PORT, GPIO_MODE_WIRED_AND, SCL_PIN);
        gpio_mode_setup(SDA_PORT, GPIO_MODE_WIRED_AND, SDA_PIN);
        I2C0_CTRL = I2C_CTRL_CLTO(I2C_CTRL_CLTO_1024PPC) | I2C_CTRL_GIBITO |
                    I2C_CTRL_BTO(I2C_CTRL_BTO_160PCC) | I2C_CTRL_CLHR_STANDARD |
                    I2C_CTRL_EN;
        I2C0_ROUTE =
            I2C_ROUTE_LOCATION(4) | I2C_ROUTE_SCLPEN | I2C_ROUTE_SDAPEN;
        I2C0_CLKDIV = 6;
        I2C0_CMD = I2C_CMD_ABORT;

        // configure the ADC
        ADC0_CTRL = ADC_CTRL_TIMEBASE(23) | ADC_CTRL_PRESC(1) |
                    ADC_CTRL_LPFMODE_RCFILT | ADC_CTRL_WARMUPMODE_KEEPADCWARM;

        // init USB
        uint32_t unique0 = DI_UNIQUE_0;
        uint32_t unique1 = DI_UNIQUE_1;

        serial_string[16] = 0;
        hex_str(unique0, serial_string + 0);
        hex_str(unique1, serial_string + 8);

        usbd_dev = usbd_init(&efm32hg_usb_driver, &usb_dev_descr,
                             &usb_config_descr, usb_strings, 3,
                             usbd_control_buffer, sizeof(usbd_control_buffer));

        usbd_register_set_config_callback(usbd_dev, usb_set_config);

        nvic_set_priority(NVIC_USB_IRQ, 0x10);
        nvic_enable_irq(NVIC_USB_IRQ);

        nvic_enable_irq(NVIC_TIMER1_IRQ);
        nvic_set_priority(NVIC_TIMER1_IRQ, 0x10);

        // Configure the system tick
        STK_RVR = 0xFFFFFF;
        STK_CSR = STK_CSR_CLKSOURCE_AHB | STK_CSR_ENABLE;

        nvic_set_priority(NVIC_GPIO_EVEN_IRQ, 0x20);
        nvic_enable_irq(NVIC_GPIO_EVEN_IRQ);

        OLED_init();
        // OLED is super slow to power on
        // Fails at 1500 ms, suceeds at 1600 ms
        // sometimes fails at 1800 ms cold boot
        delay(160 * TIMER_MS);

        OLED_init();

        Timer t_display;

        while (1) {
                Timer t{};
                while (t.elapsed() < TIMER_MS * 250)
                        ;
                GPIO_IFS = (1 << 2); // force an interrupt

                char tempstr[9];
                hex_str(t_display.elapsed(), tempstr);
                tempstr[8] = 0;
                OLED_text(tempstr, 128);

                hex_str(isr_count, tempstr);
                OLED_text(tempstr, 192);

                int adct = correct_temp(read_adc(8, 0));

                temperature_str(adct, tempstr);
                OLED_text(tempstr, 384 + 42);

                hex_str(encoder.get_angle(), tempstr);
                OLED_text("encoder ", 896, encoder.pushed_raw() ? 0xFF : 0x00);
                OLED_text(tempstr, 48 + 896);

                OLED_update();
        }

        return 0;
}
