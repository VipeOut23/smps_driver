#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "software_uart/uart.h"

#define SET(reg, pos) (reg |= 1<<(pos))
#define FLP(reg, pos) (reg ^= 1<<(pos))
#define CLR(reg, pos) (reg &= ~(1<<(pos)))
#define GET(reg, pos) (reg &  1<<(pos))


/* HW params */
#define VCC 5000         //mV
#define V_OUT_MAX 40000 // mV
#define V_OUT_MIN 3000  // mV
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

static uint16_t adc_pot_read()
{
        uint16_t adc_res;

        /* Read on PB3 */
        ADMUX = _BV(MUX1) | _BV(MUX0);
        SET(ADCSRA, ADSC);
        while(0 < GET(ADCSRA, ADSC));
        adc_res =  ADCL;
        adc_res |= ADCH<<8;

        return adc_res;
}

static uint16_t adc_feedback_read()
{
        uint16_t adc_res;

        /* Read on PB5 */
        ADMUX = 0;
        SET(ADCSRA, ADSC);
        while(0 < GET(ADCSRA, ADSC));
        adc_res =  ADCL;
        adc_res |= ADCH<<8;

        return adc_res;
}

static void adc_init()
{
        /* ADC CK = F_CPU/32 ; Vcc as ref*/
        ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS0);
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
        uint16_t adc_res; // ADC result
        uint8_t pwm_d;    // Raw pwm duty cycle value
        uint16_t v_tgt;  // Target Voltage in mV
        uint16_t vx;      // messured feedback voltage
        size_t str_len;
        char str[64];

        /* Set CK div to 1 -> 8MHz CK */
        cli();
        CLKPR = _BV(CLKPCE);
        CLKPR = 0;
        sei();

        adc_init();
        pwm_init();
        uart_init();

        /* Clear screen */
        uart_putc(27);
        uart_puts("[2J",3);
        uart_putc(27);
        uart_puts("[H",2);

        for(;;) {
                /* Read potentiometer voltage and map it to an output voltage */
                adc_res = adc_pot_read();
                v_tgt = V_OUT_MIN + 1024*(((V_OUT_MAX - V_OUT_MIN))/adc_res);

                /* Read feedback voltage */
                adc_res = adc_feedback_read();
                vx = 1024*(((uint32_t)VCC*10)/adc_res);

                /* converge to target voltage */
                if(vx < v_tgt) pwm_d++;
                if(vx > v_tgt) pwm_d--;

                /* Keep pwm value in bounds */
                if(pwm_d > PWM_D_MAX) pwm_d = PWM_D_MAX;
                if(pwm_d < PWM_D_MIN) pwm_d = PWM_D_MIN;

                /* Set pwm duty cycle */
                OCR0B = OCR0A = pwm_d;

                /* Display info */
                vx = (uint32_t) (100*adc_res)/0xFF;
                str_len = sprintf(str, "F %05dmV ; T %05dmV ; %03d%% @ %dHz\r\n", vx, v_tgt, 0xFF/pwm_d, F_PWM);
                uart_try_puts(str, str_len);
        }
}
