;=========================================================
; PIC18F4550
; LED en RB0
; 1 segundo ENCENDIDO
; 2 segundos APAGADO
; Oscilador interno
;=========================================================

#include <xc.inc>          ; Librería del ensamblador para PIC18

;================ CONFIGURACIÓN ==========================
CONFIG  FOSC = INTOSCIO_EC ; Oscilador interno
CONFIG  WDT = OFF          ; Watchdog deshabilitado
CONFIG  LVP = OFF          ; Programación en bajo voltaje OFF
CONFIG  PBADEN = OFF       ; PORTB como digital

;================ VECTOR DE RESET ========================
PSECT resetVec, class=CODE, reloc=2
ORG 0x00                   ; Dirección 0 (reset)
GOTO Inicio                ; Salta al inicio del programa

;================ CÓDIGO PRINCIPAL =======================
PSECT main_code, class=CODE, reloc=2

Inicio:
    CLRF    TRISB, a       ; Configura todo PORTB como salida
    CLRF    LATB, a        ; Inicializa PORTB en 0 (LED apagado)

Loop:
    BSF     LATB, 0, a     ; Pone RB0 en 1 ? LED encendido
    CALL    Retardo_1s     ; Espera 1 segundo

    BCF     LATB, 0, a     ; Pone RB0 en 0 ? LED apagado
    CALL    Retardo_2s     ; Espera 2 segundos

    GOTO    Loop           ; Repite el ciclo infinitamente

;================ RETARDO BASE (1 SEGUNDO) ===============
Retardo_1s:
    MOVLW   25             ; Carga 25 en W
    MOVWF   ContExt, a     ; Guarda en contador externo

LoopExterno:
    MOVLW   250            ; Carga 250 en W
    MOVWF   ContInt, a     ; Guarda en contador interno

LoopInterno:
    NOP                    ; No operation (consume tiempo)
    NOP
    NOP

    DECFSZ  ContInt, F, a  ; Decrementa contador interno
    GOTO    LoopInterno    ; Si no es cero, repite

    DECFSZ  ContExt, F, a  ; Decrementa contador externo
    GOTO    LoopExterno    ; Si no es cero, repite

    RETURN                 ; Regresa a donde fue llamada

;================ RETARDO 2 SEGUNDOS =====================
Retardo_2s:
    MOVLW   2              ; Queremos repetir 2 veces el retardo de 1s
    MOVWF   Cont2s, a      ; Guarda en variable

Loop2s:
    CALL    Retardo_1s     ; Llama al retardo de 1 segundo
    DECFSZ  Cont2s, F, a   ; Decrementa contador
    GOTO    Loop2s         ; Si no es cero, repite

    RETURN                 ; Regresa al programa principal

;================ VARIABLES EN RAM =======================
PSECT udata

ContExt: DS 1              ; Contador externo
ContInt: DS 1              ; Contador interno
Cont2s:  DS 1              ; Contador para 2 segundos

END                        ; Fin del programa