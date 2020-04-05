#include "uart.h"

#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>


#define SET(reg, pos) (reg |= 1<<(pos))
#define FLP(reg, pos) (reg ^= 1<<(pos))
#define CLR(reg, pos) (reg &= ~(1<<(pos)))
#define GET(reg, pos) (reg &  1<<(pos))


/* TX queue ring buffer */
static volatile uint8_t txqueue[UART_BUFFER_SIZE];
static volatile size_t txqueue_r;
static volatile size_t txqueue_w;
static volatile size_t txqueue_len;


/* TX shift register */
static volatile uint16_t txframe;


/**
 * Prepare c for uart transmission
 */
static inline void uart_set_txframe(const uint8_t c)
{
        /* Calculate parity if needed */
#ifdef UART_PARITY
        uint8_t parity = UART_PARITY;

        for(uint8_t _c = c; _c; _c>>=1)
                parity ^= _c&1;
#endif

        /* MARK . STOP | (PARITY) | DATA | START */
#ifdef UART_PARITY
        txframe = (0b11<<10) | (parity<<9) | ((uint16_t) c<<1) | 0;
#else
        txframe = (0b11<<9) | ((uint16_t) c<<1) | 0;
#endif
}

/**
 * Timer1A interrupt at BAUDRATE
 */
ISR(TIM1_COMPA_vect)
{
        if(txframe) {
                /* Write current bit */
                if(txframe & 1) SET(UART_PORT, UART_TX_BIT);
                else            CLR(UART_PORT, UART_TX_BIT);

                txframe >>= 1;

                if(! (txframe || txqueue_len)) {
                        /* Nothing to send (in the next ISR),
                         * disable interrupts */
                        CLR(TIMSK, OCIE1A);
                }
        }else if(txqueue_len) {
                /* Prepare next frame */
                uart_set_txframe(txqueue[txqueue_r++]);
                txqueue_r %= UART_BUFFER_SIZE;
                txqueue_len--;

        }
}

/**
 * Queue a character for uart transmission (does not start Timer1 ISR)
 */
static void uart_queue(const char c)
{
        uint8_t sreg;

        /* Wait until queue is empty enough */
        while(txqueue_len == UART_BUFFER_SIZE);

        sreg = SREG;
        cli();

        txqueue[txqueue_w++] = c;
        txqueue_w %= UART_BUFFER_SIZE;
        txqueue_len++;

        SREG = sreg;
}

/**
 * Try to queue a character for uart transmission (does not start Timer1 ISR)
 * @return UART_BUSY when busy
 */
uart_error_t uart_try_queue(const char c)
{
        uint8_t sreg;

        /* Check if queue is full */
        if(txqueue_len == UART_BUFFER_SIZE) return UART_BUSY;

        sreg = SREG;
        cli();

        txqueue[txqueue_w++] = c;
        txqueue_w %= UART_BUFFER_SIZE;
        txqueue_len++;

        SREG = sreg;

        return UART_OK;
}

void uart_putc(const char c)
{
        uart_queue(c);

        /* Enable timer interrupt and clear flag */
        SET(TIMSK, OCIE1A);
        SET(TIFR, OCF1A);
        sei();
}

uart_error_t uart_try_putc(const char c)
{
        uart_error_t ret;

        ret = uart_try_queue(c);
        if(ret) return ret;

        /* Enable timer interrupt  */
        SET(TIMSK, OCIE1A);
        sei();

        return UART_OK;
}

void uart_puts(const char *s, size_t len)
{
        size_t maxlen = UART_BUFFER_SIZE - len;

        /* Wait until queue is empty enough */
        while(txqueue_len > maxlen);

        while(len--) uart_queue(*s++);

        /* Enable timer interrupt and clear flag */
        SET(TIMSK, OCIE1A);
        SET(TIFR, OCF1A);
        sei();
}

uart_error_t uart_try_puts(const char *s, size_t len)
{
        size_t maxlen = UART_BUFFER_SIZE - len;

        /* Check if queue is empty enough */
        if(txqueue_len > maxlen) return UART_BUSY;

        while(len--) uart_queue(*s++);

        /* Enable timer interrupt  */
        SET(TIMSK, OCIE1A);
        sei();

        return UART_OK;
}

void uart_init()
{
        uint8_t sreg = SREG;

        cli();

        /* Set timer1 (CK) to CTC with apropiate divisor */
        TCCR1 = _BV(CTC1) | UART_TIMER1_PRESCALE_BITS;

        /* Set BAUDRATE clock divisor */
        OCR1A = (uint8_t) ((uint32_t) UART_TIMER1_CK/UART_BAUDRATE)-1;
        OCR1C = OCR1A;

        /* Enable and pull TX Pin to HIGH */
        SET(UART_DDR, UART_TX_BIT);
        SET(UART_PORT, UART_TX_BIT);

        txframe = 0;
        txqueue_r = 0;
        txqueue_w = 0;
        txqueue_len = 0;

        SET(TIFR, OCF1A);
        sreg = SREG;
}
