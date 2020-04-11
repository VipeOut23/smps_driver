#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#define UART_TX_BIT PB3
#include "software_uart/uart.h"

#define SET(reg, pos) (reg |= 1<<(pos))
#define FLP(reg, pos) (reg ^= 1<<(pos))
#define CLR(reg, pos) (reg &= ~(1<<(pos)))
#define GET(reg, pos) (reg &  1<<(pos))


#define V_OUT_MAX 40
#define V_OUT_MIN 12
#define PWM_D_MIN (0xFF*0.0)
#define PWM_D_MAX (0xFF*0.7)

#define F_PWM_TGT 32000 // Not accurate, search for nearest clock divisor

/* Determine PWM clock divisor based on F_PWM_TGT and F_CPU */
#if F_CPU < F_PWM_TGT*0xFF
#define F_PWM (F_CPU/0xFF)
#define PWM_CK_DIV_BITS _BV(CS00)
#elif F_CPU/8 < F_PWM_TGT*0xFF
#define F_PWM ((F_CPU/8)/0xFF)
#define PWM_CK_DIV_BITS _BV(CS01)
#elif F_CPU/64 < F_PWM_TGT*0xFF
#define F_PWM ((F_CPU/64)/0xFF)
#define PWM_CK_DIV_BITS _BV(CS01) | _BV(CS00)
#elif F_CPU/256 < F_PWM_TGT*0xFF
#define F_PWM ((F_CPU/256)/0xFF)
#define PWM_CK_DIV_BITS _BV(CS02)
#else
#define F_PWM ((F_CPU/1024)/0xFF)
#define PWM_CK_DIV_BITS _BV(CS02) | _BV(CS00)
#endif

static void adc_pot_init()
{
        /* Read on PB3 and use Vcc as ref */
        ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS0);
        ADMUX = _BV(MUX1) | _BV(MUX0);
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
        uint16_t adc_res;
        uint8_t vx;
        size_t str_len;
        char str[24];

        /* Set CK div to 1 -> 8MHz CK */
        cli();
        CLKPR = _BV(CLKPCE);
        CLKPR = 0;
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
                adc_res =  ADCL;
                adc_res |= ADCH<<8;

                /* compensate 1/2 voltage div at potentiometer
                 * and keep in PWM_D bounds */
                adc_res = (adc_res>>1);
                if(adc_res > PWM_D_MAX) adc_res = PWM_D_MAX;
                if(adc_res < PWM_D_MIN) adc_res = PWM_D_MIN;

                /* Set pwm duty cycle to match it */
                OCR0B = OCR0A = adc_res;

                /* Display info */
                vx = (uint32_t) (100*adc_res)/0xFF;
                str_len = sprintf(str, "%03d%% @ %ldHz\r\n", vx, F_PWM);
                uart_try_puts(str, str_len);
        }
}
