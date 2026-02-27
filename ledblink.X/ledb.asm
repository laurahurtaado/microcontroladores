;========================================================================
; PIC18F4550 - LED Blink con Timer0
; 1 segundo encendido
; 2 segundos apagado
; Oscilador interno 4 MHz
;========================================================================

        #include <xc.inc>        ; Incluye definiciones del PIC

;==============================
; CONFIGURACIÓN
;==============================
        CONFIG  FOSC   = INTOSCIO_EC   ; Oscilador interno
        CONFIG  WDT    = OFF           ; Watchdog desactivado
        CONFIG  LVP    = OFF           ; Programación baja tensión OFF
        CONFIG  PBADEN = OFF           ; PORTB digital
        CONFIG  MCLRE  = OFF           ; MCLR deshabilitado
        CONFIG  XINST  = OFF           ; Instrucciones extendidas OFF
        CONFIG  PWRT   = ON            ; Power-up Timer ON
        CONFIG  DEBUG  = OFF           ; Debug OFF

;==============================
; VARIABLES
;==============================
        PSECT   udata_acs          ; Sección de datos en access bank
contador:      DS 1                ; Variable para contar interrupciones
estado:        DS 1                ; 0 = apagado, 1 = encendido

;==============================
; VECTORES
;==============================
        PSECT  resetVec, class=CODE, reloc=2
        ORG     0x00               ; Dirección de reset
        GOTO    INIT               ; Salta a inicialización

        PSECT  intVec, class=CODE, reloc=2
        ORG     0x08               ; Vector de interrupción
        GOTO    ISR                ; Salta a rutina ISR

;==============================
; INICIO
;==============================
INIT:
        MOVLW   0b01100010         ; Configura 4 MHz interno
        MOVWF   OSCCON, a          ; Carga configuración en OSCCON

        BCF     TRISD, 0, a        ; RD0 como salida
        BSF     LATD,  0, a        ; LED inicia encendido

        MOVLW   0x01
        MOVWF   estado, a          ; Estado inicial = encendido
        CLRF    contador, a        ; Contador inicia en 0

;==============================
; CONFIGURACIÓN TIMER0
;==============================
        MOVLW   0b10000111         ; Timer0 ON, 16 bits, prescaler 1:256
        MOVWF   T0CON, a           ; Guarda configuración

        MOVLW   0xFF               ; Precarga alta
        MOVWF   TMR0H, a
        MOVLW   0xD9               ; Precarga baja (~10 ms)
        MOVWF   TMR0L, a

        BCF     INTCON, 2, a       ; Limpia bandera TMR0IF
        BSF     INTCON, 5, a       ; Habilita interrupción Timer0
        BSF     INTCON, 7, a       ; Habilita interrupciones globales

;==============================
; LOOP PRINCIPAL
;==============================
MAIN_LOOP:
        SLEEP                      ; Micro en bajo consumo
        NOP
        GOTO    MAIN_LOOP          ; Espera interrupciones

;==============================
; ISR - TIMER0
;==============================
ISR:
        BTFSS   INTCON, 2, a       ; Verifica si fue Timer0
        RETFIE                    ; Si no, salir

        BCF     INTCON, 2, a       ; Limpia bandera

        MOVLW   0xFF               ; Recarga Timer0
        MOVWF   TMR0H, a
        MOVLW   0xD9
        MOVWF   TMR0L, a

        INCF    contador, f, a     ; Incrementa contador

        MOVF    estado, W, a       ; Revisa estado actual
        BTFSC   STATUS, 2          ; ¿estado = 0?
        GOTO    LED_APAGADO        ; Si sí, ir a sección apagado

;==============================
; LED ENCENDIDO (1 segundo)
;==============================
LED_ENCENDIDO:
        MOVLW   100                ; 100 interrupciones = 1 segundo
        CPFSEQ  contador, a        ; ¿contador == 100?
        RETFIE                    ; Si no, salir

        CLRF    contador, a        ; Reinicia contador
        BCF     LATD, 0, a         ; Apaga LED
        CLRF    estado, a          ; Cambia estado a apagado
        RETFIE

;==============================
; LED APAGADO (2 segundos)
;==============================
LED_APAGADO:
        MOVLW   200                ; 200 interrupciones = 2 segundos
        CPFSEQ  contador, a        ; ¿contador == 200?
        RETFIE                    ; Si no, salir

        CLRF    contador, a        ; Reinicia contador
        BSF     LATD, 0, a         ; Enciende LED
        MOVLW   0x01
        MOVWF   estado, a          ; Cambia estado a encendido
        RETFIE

        END