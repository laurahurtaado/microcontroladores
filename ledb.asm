;=========================================================
; PIC18F4550
; LED en RB0
; 1 segundo ENCENDIDO
; 2 segundos APAGADO
; Oscilador interno
;=========================================================

#include <xc.inc>

;================ CONFIGURACIÓN ==========================
CONFIG  FOSC = INTOSCIO_EC
CONFIG  WDT = OFF
CONFIG  LVP = OFF
CONFIG  PBADEN = OFF

;================ VECTOR DE RESET ========================
PSECT resetVec, class=CODE, reloc=2
ORG 0x00
GOTO Inicio

;================ CÓDIGO PRINCIPAL =======================
PSECT main_code, class=CODE, reloc=2

Inicio:
    CLRF    TRISB, a
    CLRF    LATB, a

Loop:
    BSF     LATB, 0, a
    CALL    Retardo_1s

    BCF     LATB, 0, a
    CALL    Retardo_2s

    GOTO    Loop

;================ RETARDO BASE (1 SEGUNDO) ===============
Retardo_1s:
    MOVLW   80             ; <<<<<< CAMBIO AQUÍ (antes era 25)
    MOVWF   ContExt, a

LoopExterno:
    MOVLW   250
    MOVWF   ContInt, a

LoopInterno:
    NOP
    NOP
    NOP

    DECFSZ  ContInt, F, a
    GOTO    LoopInterno

    DECFSZ  ContExt, F, a
    GOTO    LoopExterno

    RETURN

;================ RETARDO 2 SEGUNDOS =====================
Retardo_2s:
    MOVLW   2
    MOVWF   Cont2s, a

Loop2s:
    CALL    Retardo_1s
    DECFSZ  Cont2s, F, a
    GOTO    Loop2s

    RETURN

;================ VARIABLES EN RAM =======================
PSECT udata

ContExt: DS 1
ContInt: DS 1
Cont2s:  DS 1

END