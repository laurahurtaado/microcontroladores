/**
 * @file    max30102.c
 * @brief   Implementacion del driver del sensor MAX30102 (frecuencia cardiaca).
 *
 * Usa el driver I2C maestro (i2c.c). En modo SpO2 cada muestra de la FIFO son
 * 6 bytes: 3 de RED + 3 de IR (datos de 18 bits).
 *
 * @author  Alejandro Restrepo Acosta
 * @author  Laura Isabel Hurtado Otalora
 * @date    2026
 */
#include "config.h"
#include "i2c.h"
#include "max30102.h"

/* -------------------- Acceso a registros -------------------- */
void MAX30102_WriteReg(uint8_t reg, uint8_t val)
{
    I2C_Start();
    I2C_Write(MAX30102_ADDR_W);
    I2C_Write(reg);
    I2C_Write(val);
    I2C_Stop();
}

uint8_t MAX30102_ReadReg(uint8_t reg)
{
    uint8_t v;
    I2C_Start();
    I2C_Write(MAX30102_ADDR_W);
    I2C_Write(reg);
    I2C_Restart();
    I2C_Write(MAX30102_ADDR_R);
    v = I2C_Read();
    I2C_Nack();
    I2C_Stop();
    return v;
}

uint8_t MAX30102_ReadPartID(void)
{
    return MAX30102_ReadReg(MAX30102_REG_PART_ID);
}

/* -------------------- Inicializacion -------------------- */
uint8_t MAX30102_Init(void)
{
    /* Reset por software */
    MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, 0x40);
    __delay_ms(50);

    /* Limpiar punteros de la FIFO */
    MAX30102_WriteReg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    MAX30102_WriteReg(MAX30102_REG_OVF_COUNTER, 0x00);
    MAX30102_WriteReg(MAX30102_REG_FIFO_RD_PTR, 0x00);

    /* FIFO_CONFIG = 0x3F: SMP_AVE=2, rollover ON, almost-full=15 */
    MAX30102_WriteReg(MAX30102_REG_FIFO_CONFIG, 0x3F);

    /* MODE_CONFIG = 0x03 -> modo SpO2 (canales RED + IR activos) */
    MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, 0x03);

    /* SPO2_CONFIG = 0x27: ADC 4096nA, SR 100Hz, PW 411us (18 bits) */
    MAX30102_WriteReg(MAX30102_REG_SPO2_CONFIG, 0x27);

    /* Corriente de los LEDs ~7.2 mA (0x24). Subir si la senal es debil. */
    MAX30102_WriteReg(MAX30102_REG_LED1_PA, 0x24);   /* RED */
    MAX30102_WriteReg(MAX30102_REG_LED2_PA, 0x24);   /* IR  */

    MAX30102_ResetBeat();

    /* Autodiagnostico: el PART_ID debe ser 0x15 */
    return (MAX30102_ReadPartID() == MAX30102_PART_ID) ? 1 : 0;
}

/* -------------------- Lectura de la FIFO -------------------- */
uint8_t MAX30102_Available(void)
{
    uint8_t wr = MAX30102_ReadReg(MAX30102_REG_FIFO_WR_PTR);
    uint8_t rd = MAX30102_ReadReg(MAX30102_REG_FIFO_RD_PTR);
    return (uint8_t)((wr - rd) & 0x1F);
}

void MAX30102_ReadFIFO(uint32_t *red, uint32_t *ir)
{
    uint8_t b[6];
    uint8_t i;

    I2C_Start();
    I2C_Write(MAX30102_ADDR_W);
    I2C_Write(MAX30102_REG_FIFO_DATA);
    I2C_Restart();
    I2C_Write(MAX30102_ADDR_R);
    for (i = 0; i < 6; i++) {
        b[i] = I2C_Read();
        if (i < 5) I2C_Ack();
        else       I2C_Nack();
    }
    I2C_Stop();

    /* 18 bits utiles por canal */
    *red = (((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2]) & 0x3FFFF;
    *ir  = (((uint32_t)b[3] << 16) | ((uint32_t)b[4] << 8) | b[5]) & 0x3FFFF;
}

uint8_t MAX30102_FingerPresent(uint32_t ir)
{
    return (ir > FINGER_IR_THRESHOLD) ? 1 : 0;
}

/* -------------------- Estimador de BPM --------------------
 * Algoritmo sencillo (punto de partida para validar el sensor):
 *   1) Estima la componente DC con un filtro IIR (alpha = 1/16).
 *   2) AC = IR - DC representa la pulsacion.
 *   3) Cada cruce por cero ascendente de la AC es un latido (con refractario).
 *   4) El BPM instantaneo se suaviza con un promedio movil.
 */
static int32_t  s_dc      = 0;   /**< Linea base DC estimada.               */
static int32_t  s_prev_ac = 0;   /**< Valor AC de la muestra anterior.      */
static uint16_t s_since   = 0;   /**< Muestras desde el ultimo latido.      */
static uint8_t  s_bpm     = 0;   /**< BPM suavizado actual.                 */

void MAX30102_ResetBeat(void)
{
    s_dc = 0; s_prev_ac = 0; s_since = 0; s_bpm = 0;
}

uint8_t MAX30102_ProcessBeat(uint32_t ir)
{
    int32_t ac;
    uint8_t beat = 0;

    if (s_dc == 0) s_dc = (int32_t)ir;     /* converge rapido al inicio */

    s_dc += ((int32_t)ir - s_dc) >> 4;     /* DC (IIR, alpha = 1/16) */
    ac    = (int32_t)ir - s_dc;            /* componente pulsatil    */
    s_since++;

    if (s_prev_ac < 0 && ac >= 0) {        /* cruce por cero ascendente */
        if (s_since > (MAX30102_FS / 4)) { /* refractario ~250 ms */
            uint16_t inst = (uint16_t)(60UL * MAX30102_FS / s_since);
            if (inst >= 30 && inst <= 220) {
                if (s_bpm == 0) s_bpm = (uint8_t)inst;
                else            s_bpm = (uint8_t)(((uint16_t)s_bpm * 3 + inst) >> 2);
                beat = 1;
            }
            s_since = 0;
        }
    }
    s_prev_ac = ac;
    return beat;
}

uint8_t MAX30102_GetBPM(void)
{
    return s_bpm;
}
