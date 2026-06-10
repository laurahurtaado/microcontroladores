/**
 * @file    uart.h
 * @brief   Driver EUSART (UART) para PIC18F4550.
 *
 * TX = RC6, RX = RC7. Configurado a 9600 baudios con Fosc = 8 MHz (8N1).
 *
 * @author  Alejandro Restrepo Acosta
 * @author  Laura Isabel Hurtado Otalora
 * @date    2026
 */
#ifndef UART_H
#define UART_H

#include <xc.h>
#include <stdint.h>

/** @brief Inicializa la EUSART en modo asincrono a 9600 baudios. */
void UART_Init(void);

/**
 * @brief  Transmite un caracter (bloqueante).
 * @param  c  Caracter a enviar.
 */
void UART_WriteChar(char c);

/**
 * @brief  Transmite una cadena terminada en nulo.
 * @param  s  Cadena a enviar.
 */
void UART_WriteText(const char *s);

/**
 * @brief  Transmite un entero sin signo en decimal.
 * @param  v  Valor a enviar.
 */
void UART_WriteUint(uint32_t v);

/**
 * @brief  Transmite un entero con signo en decimal.
 * @param  v  Valor a enviar.
 */
void UART_WriteInt(int32_t v);

#endif /* UART_H */
