#include "uart.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#define SET(reg, pos) (reg |= 1<<(pos))
#define FLP(reg, pos) (reg ^= 1<<(pos))
#define CLR(reg, pos) (reg &= ~(1<<(pos)))
#define GET(reg, pos) (reg &  1<<(pos))

#define VREF 2560
#define F_PWM_TGT 500000 // Not accurate, search for nearest clock divisor

/* Determine PWM clock divisor based on F_PWM_TGT and F_CPU */
#if F_CPU < F_PWM_TGT
#define PWM_CK_DIV_BITS _BV(CS00)
#elif F_CPU/8 < F_PWM_TGT
#define PWM_CK_DIV_BITS _BV(CS01)
#elif F_CPU/64 < F_PWM_TGT
#define PWM_CK_DIV_BITS _BV(CS01) | _BV(CS00)
#elif F_CPU/256 < F_PWM_TGT
#define PWM_CK_DIV_BITS _BV(CS02)
#else
#define PWM_CK_DIV_BITS _BV(CS02) | _BV(CS00)
#endif

static void adc_pot_init()
{
        /* Use 2.56V as ref voltage */
        ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS0);
        /* Read on PB3 and left align result */
        ADMUX = _BV(REFS2) | _BV(ADLAR) | _BV(REFS1) | _BV(MUX1) | _BV(MUX0);
}

static void pwm_init()
{
        /* FAST PWM at CK (on OC0A) */
        TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
        TCCR0B = PWM_CK_DIV_BITS;
        OCR0A = 255;
        SET(DDRB, PB0);
}

int main()
{
        uint8_t adc_res;
        uint16_t vx;
        size_t str_len;
        char str[16];

        /* Set CK div to 2 -> 4MHz CK */
        cli();
        CLKPR = _BV(CLKPCE);
        CLKPR = _BV(CLKPS0);
        sei();

        adc_pot_init();
        pwm_init();
        uart_init();

        /* Clear screen */
        uart_putc(27);
        uart_puts("[2J",3);
        uart_putc(27);
        uart_puts("[H",2);

        for(;;) {
                /* Read potentiometer voltage */
                SET(ADCSRA, ADSC);
                while(0 < GET(ADCSRA, ADSC));
                adc_res = ADCH;

                /* Set pwm duty cycle to match it */
                OCR0B = OCR0A = adc_res;

                /* Display info */
                vx = ((uint32_t) adc_res*VREF)/256;
                str_len = sprintf(str, "%04umV - %04u\r\n", vx, OCR0B);
                uart_puts(str, str_len);

                _delay_us(10);
        }
}
