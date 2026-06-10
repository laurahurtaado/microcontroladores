/**
 * @file    max30102.h
 * @brief   Driver del sensor optico MAX30102 (frecuencia cardiaca) por I2C.
 *
 * Direccion I2C de 7 bits 0x57 (comparte el bus con la OLED). Depende del
 * driver I2C maestro (i2c.c / i2c.h). En modo SpO2 cada muestra de la FIFO
 * son 6 bytes: 3 de RED + 3 de IR (datos de 18 bits).
 *
 * @author  Alejandro Restrepo Acosta
 * @author  Laura Isabel Hurtado Otalora
 * @date    2026
 */
#ifndef MAX30102_H
#define MAX30102_H

#include <xc.h>
#include <stdint.h>

#define MAX30102_ADDR_W   0xAE   /**< Direccion I2C de escritura (0x57<<1).   */
#define MAX30102_ADDR_R   0xAF   /**< Direccion I2C de lectura.               */
#define MAX30102_PART_ID  0x15   /**< Valor esperado del registro PART_ID.    */

/**
 * @name Mapa de registros (datasheet MAX30102)
 * @{
 */
#define MAX30102_REG_INTR_STATUS_1  0x00 /**< Estado de interrupciones 1.      */
#define MAX30102_REG_INTR_STATUS_2  0x01 /**< Estado de interrupciones 2.      */
#define MAX30102_REG_INTR_ENABLE_1  0x02 /**< Habilitacion de interrupciones 1.*/
#define MAX30102_REG_INTR_ENABLE_2  0x03 /**< Habilitacion de interrupciones 2.*/
#define MAX30102_REG_FIFO_WR_PTR    0x04 /**< Puntero de escritura de la FIFO. */
#define MAX30102_REG_OVF_COUNTER    0x05 /**< Contador de desbordes.           */
#define MAX30102_REG_FIFO_RD_PTR    0x06 /**< Puntero de lectura de la FIFO.   */
#define MAX30102_REG_FIFO_DATA      0x07 /**< Datos de la FIFO.                */
#define MAX30102_REG_FIFO_CONFIG    0x08 /**< Configuracion de la FIFO.        */
#define MAX30102_REG_MODE_CONFIG    0x09 /**< Configuracion de modo.           */
#define MAX30102_REG_SPO2_CONFIG    0x0A /**< Configuracion de SpO2.           */
#define MAX30102_REG_LED1_PA        0x0C /**< Corriente del LED RED.           */
#define MAX30102_REG_LED2_PA        0x0D /**< Corriente del LED IR.            */
#define MAX30102_REG_TEMP_INT       0x1F /**< Temperatura del die (entero).    */
#define MAX30102_REG_TEMP_FRAC      0x20 /**< Temperatura del die (fraccion).  */
#define MAX30102_REG_TEMP_CONFIG    0x21 /**< Configuracion de temperatura.    */
#define MAX30102_REG_REV_ID         0xFE /**< Revision ID.                     */
#define MAX30102_REG_PART_ID        0xFF /**< Part ID (debe ser 0x15).         */
/** @} */

/** @brief Frecuencia de muestreo efectiva en Hz (SR=100Hz, SMP_AVE=2 -> 50). */
#define MAX30102_FS   50

/**
 * @brief  Escribe un valor en un registro del MAX30102.
 * @param  reg  Direccion del registro.
 * @param  val  Valor a escribir.
 */
void     MAX30102_WriteReg(uint8_t reg, uint8_t val);

/**
 * @brief  Lee un registro del MAX30102.
 * @param  reg  Direccion del registro.
 * @return Valor leido.
 */
uint8_t  MAX30102_ReadReg(uint8_t reg);

/**
 * @brief  Lee el registro PART_ID.
 * @return Identificador de parte (0x15 si el sensor es correcto).
 */
uint8_t  MAX30102_ReadPartID(void);

/**
 * @brief  Inicializa el sensor (reset, FIFO, modo SpO2 y corriente de LEDs).
 * @return 1 si el PART_ID coincide (sensor OK), 0 en caso contrario.
 */
uint8_t  MAX30102_Init(void);

/**
 * @brief  Numero de muestras nuevas disponibles en la FIFO.
 * @return Cantidad de muestras (0..31).
 */
uint8_t  MAX30102_Available(void);

/**
 * @brief  Lee una muestra (par RED/IR de 18 bits) de la FIFO.
 * @param[out] red  Puntero donde se guarda el canal RED.
 * @param[out] ir   Puntero donde se guarda el canal IR.
 */
void     MAX30102_ReadFIFO(uint32_t *red, uint32_t *ir);

/**
 * @brief  Indica si el nivel IR corresponde a contacto con la piel.
 * @param  ir  Valor IR de la ultima muestra.
 * @return 1 si hay dedo, 0 si no.
 */
uint8_t  MAX30102_FingerPresent(uint32_t ir);

/**
 * @brief  Procesa una muestra IR para estimar el ritmo cardiaco.
 *
 * Estimador simple por deteccion de cruces por cero de la componente AC.
 * Debe llamarse una vez por cada muestra IR.
 *
 * @param  ir  Valor IR de la muestra.
 * @return 1 si se detecto un latido en esta muestra, 0 si no.
 */
uint8_t  MAX30102_ProcessBeat(uint32_t ir);

/**
 * @brief  Devuelve el BPM suavizado actual.
 * @return Frecuencia cardiaca estimada (lpm).
 */
uint8_t  MAX30102_GetBPM(void);

/** @brief Reinicia el estado del estimador de BPM (al perder contacto). */
void     MAX30102_ResetBeat(void);

#endif /* MAX30102_H */
