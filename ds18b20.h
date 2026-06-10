/**
 * @file    ds18b20.h
 * @brief   Driver del sensor de temperatura DS18B20 sobre bus 1-Wire.
 *
 * Linea de datos en un solo pin (RD3) con resistencia pull-up EXTERNA de
 * 4.7 kohm a VDD. Ademas de la lectura bloqueante (::DS18B20_ReadTempX10)
 * ofrece una API NO bloqueante (::DS18B20_StartConversion +
 * ::DS18B20_ConversionDone + ::DS18B20_ReadResult) para intercalar la medicion
 * de temperatura con el muestreo del MAX30102 sin perder muestras.
 *
 * @author  Alejandro Restrepo Acosta
 * @author  Laura Isabel Hurtado Otalora
 * @date    2026
 */
#ifndef DS18B20_H
#define DS18B20_H

#include <xc.h>
#include <stdint.h>

#define DS18B20_TRIS   TRISDbits.TRISD3   /**< Direccion del pin de datos.   */
#define DS18B20_OUT    LATDbits.LATD3     /**< Registro de salida del pin.   */
#define DS18B20_IN     PORTDbits.RD3      /**< Lectura del pin de datos.     */

#define DS18B20_CMD_SKIP_ROM    0xCC      /**< Comando Skip ROM.             */
#define DS18B20_CMD_CONVERT_T   0x44      /**< Comando Convert T.            */
#define DS18B20_CMD_READ_SCR    0xBE      /**< Comando Read Scratchpad.      */
#define DS18B20_CMD_WRITE_SCR   0x4E      /**< Comando Write Scratchpad.     */

/**
 * @brief  Genera el pulso de reset y detecta la presencia del sensor.
 * @return 1 si el sensor responde (presente), 0 si esta ausente.
 */
uint8_t DS18B20_Reset(void);

/**
 * @brief  Envia un byte por el bus 1-Wire (LSB primero).
 * @param  b  Byte a transmitir.
 */
void    DS18B20_WriteByte(uint8_t b);

/**
 * @brief  Lee un byte del bus 1-Wire (LSB primero).
 * @return Byte leido.
 */
uint8_t DS18B20_ReadByte(void);

/**
 * @brief  Lectura bloqueante de temperatura (~750 ms).
 * @param[out] temp_x10  Temperatura multiplicada por 10 (ej. 367 = 36.7 C).
 * @return 1 si la lectura es valida (CRC correcto), 0 si hubo error.
 */
uint8_t DS18B20_ReadTempX10(int16_t *temp_x10);

/**
 * @brief  Inicia una conversion de temperatura (no bloqueante).
 * @return 1 si el sensor esta presente, 0 si no.
 */
uint8_t DS18B20_StartConversion(void);

/**
 * @brief  Comprueba si la conversion ya termino.
 * @return 1 cuando la conversion finalizo, 0 mientras convierte.
 */
uint8_t DS18B20_ConversionDone(void);

/**
 * @brief  Lee el resultado de una conversion ya iniciada.
 * @param[out] temp_x10  Temperatura multiplicada por 10.
 * @return 1 si el CRC es correcto, 0 si hubo error.
 */
uint8_t DS18B20_ReadResult(int16_t *temp_x10);

#endif /* DS18B20_H */
