#ifndef __UART_H
#define __UART_H

#include <stddef.h>

#define UART_PORT PORTB
#define UART_DDR DDRB
#define UART_TX_BIT PB1
#define UART_BAUDRATE 9600
// #define UART_PARITY 0
#define UART_BUFFER_SIZE 64

// DON'T CHANGE ANYTHING BELOW /////////////////////////////////////////////////
#if F_CPU/UART_BAUDRATE < 256

#define UART_TIMER1_PRESCALE_BITS _BV(CS10)
#define UART_TIMER1_CK (F_CPU)

#elif (F_CPU/2)/UART_BAUDRATE < 256

#define UART_TIMER1_PRESCALE_BITS _BV(CS11)
#define UART_TIMER1_CK (F_CPU/2)

#elif (F_CPU/4)/UART_BAUDRATE < 256

#define UART_TIMER1_PRESCALE_BITS _BV(CS11) | _BV(CS10)
#define UART_TIMER1_CK (F_CPU/4)

#elif (F_CPU/8)/UART_BAUDRATE < 256

#define UART_TIMER1_PRESCALE_BITS _BV(CS12)
#define UART_TIMER1_CK (F_CPU/8)

#elif (F_CPU/16)/UART_BAUDRATE < 256

#define UART_TIMER1_PRESCALE_BITS _BV(CS12) | _BV(CS10)
#define UART_TIMER1_CK (F_CPU/16)

#elif (F_CPU/32)/UART_BAUDRATE < 256

#define UART_TIMER1_PRESCALE_BITS _BV(CS12) | _BV(CS11)
#define UART_TIMER1_CK (F_CPU/32)

#else
#error BAUDRATE is too low
#endif
////////////////////////////////////////////////////////////////////////////////


typedef enum
{
        UART_OK,
        UART_BUSY
} uart_error_t;

/**
 * Transmit a single character (blocking)
 */
void uart_putc(const char c);

/**
 * Try to transmit a single character, if UART is Busy, return UART_BUSY
 */
uart_error_t uart_try_queue(const char c);

/**
 * Transmit a series of characters (blocking)
 */
void uart_puts(const char *s, size_t len);

/**
 * Try to transmit a string, if UART is Busy, return UART_BUSY
 */
uart_error_t uart_try_puts(const char *s, size_t len);

/**
 * Initialize UART
 * using Timer1
 */
void uart_init();

#endif // __UART_H
