/**
 * @file    i2c.h
 * @brief   Driver I2C (MSSP) para PIC18F4550 en modo maestro/esclavo.
 *
 * @note    Libreria original creada por Ing. Abiezer Hernandez O. (25/11/2019,
 *          Electronica y Circuitos). Comentarios Doxygen anadidos para el
 *          Monitor de Signos Vitales. Pines por defecto: SCL=RB1, SDA=RB0.
 */
#include <xc.h>
#define _XTAL_FREQ 8000000          /**< Frecuencia del oscilador (Hz). */

#define TRIS_SCL TRISBbits.TRISB1   /**< Direccion del pin SCL. */
#define TRIS_SDA TRISBbits.TRISB0   /**< Direccion del pin SDA. */
/*#define TRIS_SCL TRISCbits.TRISC3
#define TRIS_SDA TRISCbits.TRISC4*/

#define I2C_100KHZ 0x80             /**< Valor de SSPSTAT para 100 kHz. */
#define I2C_400KHZ 0x00             /**< Valor de SSPSTAT para 400 kHz. */

#define I2C_MASTER_MODE             /**< Compila las funciones de maestro. */
//#define I2C_SLAVE_MODE            /**< Compila las funciones de esclavo. */

#ifdef I2C_MASTER_MODE
/**
 * @brief  Inicializa el MSSP como maestro I2C.
 * @param  sp_i2c  Velocidad: ::I2C_100KHZ o ::I2C_400KHZ.
 */
void I2C_Init_Master(unsigned char sp_i2c);

/** @brief Genera la condicion de Start. */
void I2C_Start(void);
/** @brief Genera la condicion de Stop. */
void I2C_Stop(void);
/** @brief Genera una condicion de Start repetido. */
void I2C_Restart(void);
/** @brief Envia un ACK al esclavo. */
void I2C_Ack(void);
/** @brief Envia un NACK al esclavo. */
void I2C_Nack(void);

/**
 * @brief  Lee un byte del bus.
 * @return Byte recibido.
 */
unsigned char I2C_Read(void);

/**
 * @brief  Escribe un byte en el bus.
 * @param  data  Byte a transmitir.
 * @return Estado del ACK (0 = ACK recibido, 1 = NACK).
 */
short I2C_Write(char data);
#endif

#ifdef I2C_SLAVE_MODE
void I2C_Init_Slave(unsigned char add_slave);   /**< Inicializa como esclavo. */
unsigned char I2C_Read_Slave(void);             /**< Lee un byte como esclavo.*/
void I2C_Write_Slave(char dato_i2c);            /**< Escribe como esclavo.    */
void I2C_Error_Data(void);                      /**< Maneja error de datos.   */
short I2C_Write_Mode(void);                      /**< 1 si el maestro escribe. */
short I2C_Read_Mode(void);                       /**< 1 si el maestro lee.     */
short I2C_Error_Read(void);                      /**< 1 si hubo overflow/WCOL. */
#endif
