/**
 * @file    ds18b20.c
 * @brief   Implementacion del driver DS18B20 sobre 1-Wire (bit-banging).
 *
 * Tiempos (us) segun la hoja de datos Maxim. A 8 MHz, Tcy = 0.5 us, por lo que
 * __delay_us() tiene resolucion suficiente para el protocolo.
 *
 * Secuencia tipica de medicion:
 *   1) Reset + presencia
 *   2) SKIP ROM (0xCC)
 *   3) CONVERT T (0x44)
 *   4) Esperar fin de conversion (la linea pasa a '1' al terminar)
 *   5) Reset + SKIP ROM + READ SCRATCHPAD (0xBE)
 *   6) Leer 9 bytes y validar CRC
 *   7) Convertir los 2 primeros bytes a grados Celsius
 *
 * @author  Alejandro Restrepo Acosta
 * @author  Laura Isabel Hurtado Otalora
 * @date    2026
 */
#include "config.h"
#include "ds18b20.h"

/** @brief Fuerza la linea 1-Wire a 0 (salida en bajo). */
static void ow_low(void)     { DS18B20_OUT = 0; DS18B20_TRIS = 0; }
/** @brief Libera la linea 1-Wire (entrada; la pull-up la sube a 1). */
static void ow_release(void) { DS18B20_TRIS = 1; }

uint8_t DS18B20_Reset(void)
{
    uint8_t presence;
    ow_low();      __delay_us(480);   /* pulso de reset >= 480 us */
    ow_release();  __delay_us(70);    /* respuesta del sensor 15-60 us */
    presence = (DS18B20_IN == 0) ? 1 : 0;  /* 0 en la linea = presente */
    __delay_us(410);
    return presence;
}

/** @brief Escribe un bit en el bus 1-Wire. @param bit Bit a transmitir. */
static void ow_write_bit(uint8_t bit)
{
    if (bit) { ow_low(); __delay_us(6);  ow_release(); __delay_us(64); }
    else     { ow_low(); __delay_us(60); ow_release(); __delay_us(10); }
}

/** @brief Lee un bit del bus 1-Wire. @return Bit leido. */
static uint8_t ow_read_bit(void)
{
    uint8_t b;
    ow_low(); __delay_us(6);
    ow_release(); __delay_us(9);     /* muestrear dentro de la ventana de 15us */
    b = DS18B20_IN ? 1 : 0;
    __delay_us(55);
    return b;
}

void DS18B20_WriteByte(uint8_t v)
{
    uint8_t i;
    for (i = 0; i < 8; i++) { ow_write_bit(v & 0x01); v >>= 1; }
}

uint8_t DS18B20_ReadByte(void)
{
    uint8_t i, v = 0;
    for (i = 0; i < 8; i++) { v >>= 1; if (ow_read_bit()) v |= 0x80; }
    return v;
}

/**
 * @brief  CRC-8 Dallas/Maxim (polinomio 0x31 reflejado = 0x8C).
 * @param  data  Puntero a los datos.
 * @param  len   Cantidad de bytes.
 * @return CRC calculado.
 */
static uint8_t ow_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0, i, j;
    for (i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (j = 0; j < 8; j++) {
            uint8_t mix = (uint8_t)((crc ^ inbyte) & 0x01);
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

/**
 * @brief  Convierte el scratchpad leido a temperatura x10 (valida el CRC).
 * @param  scr       Scratchpad de 9 bytes.
 * @param[out] temp_x10  Temperatura x10 resultante.
 * @return 1 si el CRC es correcto, 0 si no.
 */
static uint8_t scr_to_temp(const uint8_t *scr, int16_t *temp_x10)
{
    int16_t raw;
    if (ow_crc8(scr, 8) != scr[8]) return 0;
    raw = (int16_t)(((uint16_t)scr[1] << 8) | scr[0]);
    *temp_x10 = (int16_t)(((int32_t)raw * 10) / 16);   /* 1 LSB = 1/16 C */
    return 1;
}

uint8_t DS18B20_ReadTempX10(int16_t *temp_x10)
{
    uint8_t scr[9], i;
    uint16_t guard = 0;

    if (!DS18B20_StartConversion()) return 0;
    while (!DS18B20_ConversionDone()) {
        __delay_ms(10);
        if (++guard > 100) break;             /* timeout ~1 s */
    }
    if (!DS18B20_Reset()) return 0;
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_READ_SCR);
    for (i = 0; i < 9; i++) scr[i] = DS18B20_ReadByte();
    return scr_to_temp(scr, temp_x10);
}

uint8_t DS18B20_StartConversion(void)
{
    if (!DS18B20_Reset()) return 0;
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_CONVERT_T);
    return 1;
}

uint8_t DS18B20_ConversionDone(void)
{
    /* Durante la conversion el sensor mantiene la linea en 0 */
    return ow_read_bit() ? 1 : 0;
}

uint8_t DS18B20_ReadResult(int16_t *temp_x10)
{
    uint8_t scr[9], i;
    if (!DS18B20_Reset()) return 0;
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_READ_SCR);
    for (i = 0; i < 9; i++) scr[i] = DS18B20_ReadByte();
    return scr_to_temp(scr, temp_x10);
}
