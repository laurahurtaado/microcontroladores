/**
 * @file    uart.c
 * @brief   Implementacion del driver EUSART (UART) para PIC18F4550 @ 8 MHz.
 *
 * Calculo del baud rate (modo asincrono, BRGH = 1):
 * @code
 *   SPBRG = Fosc/(16*baud) - 1 = 8000000/(16*9600) - 1 = 51.08 -> 51
 *   Baud real = 8000000/(16*52) = 9615 bps  (error ~0.16%)
 * @endcode
 *
 * @author  Alejandro Restrepo Acosta
 * @author  Laura Isabel Hurtado Otalora
 * @date    2026
 */
#include "config.h"
#include "uart.h"

void UART_Init(void)
{
    TRISCbits.TRISC6 = 1;   /* TX: el modulo EUSART controla el pin */
    TRISCbits.TRISC7 = 1;   /* RX */

    SPBRG = 51;             /* 9600 baud @ 8 MHz, BRGH = 1 */
    TXSTAbits.BRGH = 1;     /* alta velocidad */
    TXSTAbits.SYNC = 0;     /* modo asincrono */
    RCSTAbits.SPEN = 1;     /* habilita el puerto serie */
    TXSTAbits.TXEN = 1;     /* habilita transmisor */
    RCSTAbits.CREN = 1;     /* habilita receptor (opcional) */
}

void UART_WriteChar(char c)
{
    while (!TXSTAbits.TRMT);   /* espera a que el registro de TX quede libre */
    TXREG = c;
}

void UART_WriteText(const char *s)
{
    while (*s) UART_WriteChar(*s++);
}

void UART_WriteUint(uint32_t v)
{
    char buf[11];
    int8_t i = 0;

    if (v == 0) { UART_WriteChar('0'); return; }
    while (v) { buf[i++] = (char)('0' + (v % 10)); v /= 10; }
    while (i--) UART_WriteChar(buf[i]);
}

void UART_WriteInt(int32_t v)
{
    if (v < 0) { UART_WriteChar('-'); v = -v; }
    UART_WriteUint((uint32_t)v);
}
