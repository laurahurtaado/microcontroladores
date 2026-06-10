/**
 * @file    main.c
 * @brief   Programa principal del Monitor de Signos Vitales (version simple).
 *
 * @mainpage Monitor Portatil de Signos Vitales (PIC18F4550)
 *
 * Version SIMPLE: une los dos modulos ya probados por separado y agrega los
 * indicadores y la alarma, en un unico bucle (sin boton ni maquina de estados).
 *
 * - MAX30102 (frecuencia cardiaca) por I2C.
 * - DS18B20  (temperatura) por 1-Wire, con lectura NO bloqueante.
 * - OLED SSD1306 por I2C para visualizacion.
 * - UART a 9600 baudios (texto + CSV).
 * - 4 LEDs de estado (Encendido, Preparando, En espera, Funcional).
 * - LED de alarma + buzzer cuando un signo sale de rango.
 *
 * @par Funcionamiento
 * Lee continuamente el MAX30102; si hay dedo estima el BPM y mide la
 * temperatura en segundo plano. Cada ~0.5 s actualiza la OLED, transmite por
 * UART y evalua las alarmas. El paso "espera/activo" es automatico por el
 * contacto del dedo (no requiere pulsador).
 *
 * @par Pines (ver config.h)
 * I2C SDA=RB0 SCL=RB1 | UART TX=RC6 | 1-Wire RD3 |
 * LEDs POWER=RD0 FUNC=RD1 PREP=RD2 WAIT=RD4 | Alarma RD5 | Buzzer RD6.
 *
 * @par Integrantes
 * - Alejandro Restrepo Acosta
 * - Laura Isabel Hurtado Otalora
 *
 * @author  Alejandro Restrepo Acosta
 * @author  Laura Isabel Hurtado Otalora
 * @date    2026
 */

/* ---------------- Bits de configuracion ---------------- */
#pragma config PLLDIV = 1, CPUDIV = OSC1_PLL2, USBDIV = 1
#pragma config FOSC = INTOSCIO_EC, FCMEN = OFF, IESO = OFF
#pragma config PWRT = OFF, BOR = OFF, BORV = 3, VREGEN = OFF
#pragma config WDT = OFF, WDTPS = 32768
#pragma config CCP2MX = ON, PBADEN = OFF, LPT1OSC = OFF, MCLRE = OFF
#pragma config STVREN = ON, LVP = OFF, ICPRT = OFF, XINST = OFF
#pragma config CP0 = OFF, CP1 = OFF, CP2 = OFF, CP3 = OFF
#pragma config CPB = OFF, CPD = OFF
#pragma config WRT0 = OFF, WRT1 = OFF, WRT2 = OFF, WRT3 = OFF
#pragma config WRTC = OFF, WRTB = OFF, WRTD = OFF
#pragma config EBTR0 = OFF, EBTR1 = OFF, EBTR2 = OFF, EBTR3 = OFF
#pragma config EBTRB = OFF

#include "config.h"
#include "i2c.h"
#include "ssd1306_oled.h"
#include "uart.h"
#include "max30102.h"
#include "ds18b20.h"
#include <stdint.h>

/**
 * @brief  Convierte un entero sin signo a cadena decimal.
 * @param  v  Valor a convertir.
 * @param[out] s  Buffer destino (minimo 11 bytes).
 */
static void u2str(uint32_t v, char *s)
{
    char tmp[11]; uint8_t i = 0, j = 0;
    if (v == 0) { s[0] = '0'; s[1] = 0; return; }
    while (v) { tmp[i++] = (char)('0' + (v % 10)); v /= 10; }
    while (i) s[j++] = tmp[--i];
    s[j] = 0;
}

/**
 * @brief  Compone "clave" + numero en una cadena (ej. "BPM: 78").
 * @param[out] dst  Buffer destino.
 * @param  key  Texto de la clave.
 * @param  val  Valor numerico.
 */
static void kv(char *dst, const char *key, uint32_t val)
{
    uint8_t i = 0, j = 0; char num[11];
    while (key[i]) { dst[i] = key[i]; i++; }
    u2str(val, num);
    while (num[j]) dst[i++] = num[j++];
    dst[i] = 0;
}

/**
 * @brief  Formatea una temperatura x10 como texto (ej. "T: 36.7C").
 * @param[out] dst   Buffer destino.
 * @param  tx10  Temperatura multiplicada por 10.
 */
static void temp_str(char *dst, int16_t tx10)
{
    uint8_t i = 0, j = 0; char num[6];
    int16_t t = tx10; uint16_t ip, fp;
    dst[i++] = 'T'; dst[i++] = ':'; dst[i++] = ' ';
    if (t < 0) { dst[i++] = '-'; t = (int16_t)(-t); }
    ip = (uint16_t)(t / 10); fp = (uint16_t)(t % 10);
    u2str(ip, num);
    while (num[j]) dst[i++] = num[j++];
    dst[i++] = '.'; dst[i++] = (char)('0' + fp); dst[i++] = 'C';
    dst[i] = 0;
}

/**
 * @brief  Muestra dos lineas de texto en la OLED y refresca.
 * @param  l1  Primera linea.
 * @param  l2  Segunda linea.
 */
static void oled_2lines(char *l1, char *l2)
{
    OLED_ClearDisplay();
    OLED_SetFont(FONT_1);
    OLED_Write_Text(0, 0,  l1);
    OLED_Write_Text(0, 24, l2);
    OLED_Update();
}

/**
 * @brief  Punto de entrada: inicializa, autodiagnostica y ejecuta el bucle.
 */
void main(void)
{
    uint32_t red = 0, ir = 0;
    uint8_t  finger = 0;
    uint8_t  max_ok, ds_ok;

    int16_t  temp_x10 = 0;
    uint8_t  temp_valid = 0;
    uint8_t  temp_phase = 0;     /* 0=lanzar, 1=convirtiendo, 2=listo */
    uint16_t temp_tmr = 0;
    uint16_t refresh = 0;
    char     line[24];

    /* --- Configuracion base --- */
    OSCCON = 0x72;               /* INTOSC 8 MHz */
    ADCON1bits.PCFG = 0x0F;      /* todos los pines digitales */

    /* 4 LEDs de estado + LED de alarma + buzzer como salida, apagados */
    LED_POWER_TRIS = 0; LED_FUNC_TRIS = 0; LED_PREP_TRIS = 0; LED_WAIT_TRIS = 0;
    LED_ALARM_TRIS = 0; BUZZER_TRIS = 0;
    LED_POWER = 0; LED_FUNC = 0; LED_PREP = 0; LED_WAIT = 0;
    LED_ALARM = 0; BUZZER = 0;

    LED_POWER = 1;      /* Encendido: ON mientras haya alimentacion */
    LED_PREP  = 1;      /* Preparando sistema: ON durante init/autodiagnostico */

    DS18B20_OUT = 0; DS18B20_TRIS = 1;   /* 1-Wire en reposo (liberado) */

    /* --- Perifericos --- */
    I2C_Init_Master(I2C_100KHZ);
    UART_Init();
    OLED_Init();

    oled_2lines("MONITOR SV", "Iniciando...");
    UART_WriteText("\r\n== Monitor SV (version simple) ==\r\n");
    __delay_ms(600);

    /* --- Autodiagnostico --- */
    max_ok = MAX30102_Init();
    ds_ok  = DS18B20_Reset();

    OLED_ClearDisplay();
    OLED_SetFont(FONT_1);
    OLED_Write_Text(0, 0,  "AUTODIAGNOSTICO");
    OLED_Write_Text(0, 24, max_ok ? "MAX30102: OK" : "MAX30102: FALLA");
    OLED_Write_Text(0, 44, ds_ok  ? "DS18B20 : OK" : "DS18B20 : FALLA");
    OLED_Update();
    UART_WriteText(max_ok ? "MAX30102: OK\r\n" : "MAX30102: FALLA\r\n");
    UART_WriteText(ds_ok  ? "DS18B20 : OK\r\n" : "DS18B20 : FALLA\r\n");
    UART_WriteText("CSV: BPM,TEMPx10,ESTADO\r\n");
    __delay_ms(1500);

    LED_PREP = 0;       /* fin de la preparacion */
    LED_WAIT = 1;       /* arranca en espera (aun sin dedo) */

    /* Lanzar la primera conversion de temperatura */
    if (DS18B20_StartConversion()) { temp_phase = 1; temp_tmr = 0; }

    /* --- Bucle principal --- */
    while (1) {
        uint8_t n = MAX30102_Available();

        while (n--) {
            MAX30102_ReadFIFO(&red, &ir);
            finger = MAX30102_FingerPresent(ir);

            if (finger) {
                MAX30102_ProcessBeat(ir);      /* actualiza el BPM */
                LED_FUNC = 1; LED_WAIT = 0;    /* Funcional: midiendo */
            } else {
                MAX30102_ResetBeat();
                LED_FUNC = 0; LED_WAIT = 1;    /* En espera: sin dedo */
            }

            /* Temperatura no bloqueante: ~800 ms (40 muestras @ 50 Hz) */
            if (temp_phase == 1 && ++temp_tmr >= 40) {
                temp_valid = DS18B20_ReadResult(&temp_x10);
                temp_phase = 2;
            }

            /* Refresco OLED + UART + alarma cada ~0.5 s (25 muestras) */
            if (++refresh >= 25) {
                refresh = 0;

                uint8_t bpm = finger ? MAX30102_GetBPM() : 0;

                /* --- Evaluacion de alarma (LED + buzzer) --- */
                uint8_t alarma = 0;
                if (temp_valid &&
                    (temp_x10 > TEMP_MAX_X10 || temp_x10 < TEMP_MIN_X10)) alarma = 1;
                if (finger && bpm > 0 &&
                    (bpm > BPM_MAX || bpm < BPM_MIN)) alarma = 1;

                if (alarma) { LED_ALARM ^= 1; BUZZER = LED_ALARM; }
                else        { LED_ALARM = 0;  BUZZER = 0; }

                /* --- OLED --- */
                OLED_ClearDisplay();
                OLED_SetFont(FONT_1);
                if (finger) {
                    kv(line, "BPM: ", bpm);
                } else {
                    kv(line, "IR: ", ir);     /* muestra el IR para calibrar */
                }
                OLED_Write_Text(0, 0, line);

                if (temp_valid) temp_str(line, temp_x10);
                else { line[0]='T'; line[1]=':'; line[2]=' ';
                       line[3]='-'; line[4]='-'; line[5]=0; }
                OLED_Write_Text(0, 22, line);

                OLED_Write_Text(0, 44,
                    !finger ? "Coloque el dedo" : (alarma ? "ALERTA" : "NORMAL"));
                OLED_Update();

                /* --- UART: texto + CSV --- */
                UART_WriteText("BPM: "); UART_WriteUint(bpm);
                UART_WriteText(", TEMP: ");
                if (temp_valid) UART_WriteInt(temp_x10); else UART_WriteText("--");
                UART_WriteText(", ESTADO: ");
                UART_WriteText(!finger ? "SIN DEDO" : (alarma ? "ALERTA" : "NORMAL"));
                UART_WriteText("\r\n");

                UART_WriteUint(bpm); UART_WriteChar(',');
                if (temp_valid) UART_WriteInt(temp_x10); else UART_WriteText("--");
                UART_WriteChar(',');
                UART_WriteText(!finger ? "SIN_DEDO" : (alarma ? "ALERTA" : "NORMAL"));
                UART_WriteText("\r\n");

                /* Relanzar la conversion de temperatura */
                if (temp_phase == 2 && DS18B20_StartConversion()) {
                    temp_phase = 1; temp_tmr = 0;
                }
            }
        }
    }
}
