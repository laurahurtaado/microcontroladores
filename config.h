/**
 * @file    config.h
 * @brief   Configuracion global del proyecto: frecuencia, pines y umbrales.
 *
 * Version SIMPLE (sin boton, sin maquina de estados) del Monitor Portatil de
 * Signos Vitales sobre PIC18F4550 @ 8 MHz (oscilador interno INTOSC).
 *
 * Une los dos modulos ya probados (MAX30102 + DS18B20) y agrega:
 *   - 4 LEDs de estado (Encendido, Preparando, En espera, Funcional)
 *   - LED de alarma + buzzer
 *
 * @par Mapa de pines
 * - I2C   : SDA=RB0, SCL=RB1  (OLED 0x3C + MAX30102 0x57)
 * - UART  : TX=RC6, RX=RC7    (9600 baud)
 * - 1-Wire: DS18B20 = RD3     (pull-up externa 4.7k a VDD)
 * - LEDs  : POWER=RD0, FUNC=RD1, PREP=RD2, WAIT=RD4
 * - Alarma: LED_ALARM=RD5, BUZZER=RD6
 *
 * @author  Alejandro Restrepo Acosta
 * @author  Laura Isabel Hurtado Otalora
 * @date    2026
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <xc.h>

/** @brief Frecuencia del oscilador en Hz (igual que i2c.h y ssd1306_oled.h). */
#define _XTAL_FREQ      8000000

/**
 * @name LEDs de estado
 * Cuatro indicadores de estado del sistema. Se usan los registros LAT para
 * escritura de salida.
 * @{
 */
#define LED_POWER_TRIS  TRISDbits.TRISD0   /**< Direccion del LED "Encendido". */
#define LED_POWER       LATDbits.LATD0     /**< LED "Encendido" (ON siempre).  */
#define LED_FUNC_TRIS   TRISDbits.TRISD1   /**< Direccion del LED "Funcional". */
#define LED_FUNC        LATDbits.LATD1     /**< LED "Funcional" (midiendo).    */
#define LED_PREP_TRIS   TRISDbits.TRISD2   /**< Direccion del LED "Preparando".*/
#define LED_PREP        LATDbits.LATD2     /**< LED "Preparando sistema".      */
#define LED_WAIT_TRIS   TRISDbits.TRISD4   /**< Direccion del LED "En espera". */
#define LED_WAIT        LATDbits.LATD4     /**< LED "En espera" (sin dedo).    */
/** @} */

/**
 * @name Alarma
 * Actuadores de alarma: LED dedicado y buzzer.
 * @{
 */
#define LED_ALARM_TRIS  TRISDbits.TRISD5   /**< Direccion del LED de alarma. */
#define LED_ALARM       LATDbits.LATD5     /**< LED de alarma.               */
#define BUZZER_TRIS     TRISDbits.TRISD6   /**< Direccion del buzzer.        */
#define BUZZER          LATDbits.LATD6     /**< Buzzer (activo: on/off).     */
/** @} */

/**
 * @name Umbrales clinicos
 * La temperatura se expresa multiplicada por 10 (ej. 380 = 38.0 C) para
 * trabajar con enteros.
 * @{
 */
#define BPM_MIN          50     /**< Frecuencia cardiaca minima (lpm).        */
#define BPM_MAX         120     /**< Frecuencia cardiaca maxima (lpm).        */
#define TEMP_MIN_X10    350     /**< Temperatura minima x10 (35.0 C).         */
#define TEMP_MAX_X10    380     /**< Temperatura maxima x10 (38.0 C).         */
/** @} */

/** @brief Umbral de IR por encima del cual se considera contacto con la piel. */
#define FINGER_IR_THRESHOLD   50000UL

#endif /* CONFIG_H */
