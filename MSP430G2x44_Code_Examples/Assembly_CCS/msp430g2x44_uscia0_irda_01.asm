; --COPYRIGHT--,BSD_EX
;  Copyright (c) 2012, Texas Instruments Incorporated
;  All rights reserved.
; 
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions
;  are met:
; 
;  *  Redistributions of source code must retain the above copyright
;     notice, this list of conditions and the following disclaimer.
; 
;  *  Redistributions in binary form must reproduce the above copyright
;     notice, this list of conditions and the following disclaimer in the
;     documentation and/or other materials provided with the distribution.
; 
;  *  Neither the name of Texas Instruments Incorporated nor the names of
;     its contributors may be used to endorse or promote products derived
;     from this software without specific prior written permission.
; 
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
;  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
;  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
;  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
;  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
;  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
;  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
;  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
;  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
;  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
; 
; ******************************************************************************
;  
;                        MSP430 CODE EXAMPLE DISCLAIMER
; 
;  MSP430 code examples are self-contained low-level programs that typically
;  demonstrate a single peripheral function or device feature in a highly
;  concise manner. For this the code may rely on the device's power-on default
;  register values and settings such as the clock configuration and care must
;  be taken when combining code from several examples to avoid potential side
;  effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
;  for an API functional library-approach to peripheral configuration.
; 
; --/COPYRIGHT--
;*******************************************************************************
;   MSP430G2x44 Demo - USCI_A0 IrDA External Loopback Test, 8MHz SMCLK
;
;   Description: This example transmits bytes through the USCI module
;   configured for IrDA mode, and receives them using an external loopback
;   connection. The transfered sequence is 00h, 01h, 02h, ..., ffh. The
;   received bytes are also stored in memory starting at address RxData.
;   In the case of an RX error the LED is lighted and program execution stops.
;   An external loopback connection has been used as it allows for the
;   connection of a scope to monitor the communication, which is not possible
;   when using the internal loopback.
;   ACLK = n/a, MCLK = SMCLK = BRCLK = CALxxx_8MHZ = 8MHz
;
;               MSP430G2x44
;             -----------------
;         /|\|              XIN|-
;          | |                 |
;          --|RST          XOUT|-
;            |                 |
;            |     P3.5/UCA0RXD|--+   external
;            |     P3.4/UCA0TXD|--+   loopback connection
;            |                 |
;            |                 |
;            |             P1.0|--->  LED
;            |                 |
;
;  William Goh
;  Texas Instruments Inc.
;  March 2013
;  Built with Code Composer Studio v5.3
;*******************************************************************************
 .cdecls C,LIST,  "msp430.h"
;------------------------------------------------------------------------------             
RxData      .usect ".bss",256              ; Received data buffer-256 bytes
;------------------------------------------------------------------------------                              
;------------------------------------------------------------------------------
            .text                  			; Program reset
;------------------------------------------------------------------------------
RESET       mov.w   #300h,SP                ; Initialize stack pointer
StopWDT     mov.w   #WDTPW+WDTHOLD,&WDTCTL  ; Stop WDT
CheckCal	cmp.b   #0xFF,&CALBC1_8MHZ		; Check calibration constant
            jne     SetupBCS				; if not erased, load.           
Trap        jmp     Trap					; if erased do not load, trap CPU!
SetupBCS    clr.b   &DCOCTL                 ; Select lowest DCOx and MODx settings
            mov.b   &CALBC1_8MHZ,&BCSCTL1   ; Load 8MHz constants
            mov.b   &CALDCO_8MHZ,&DCOCTL    ;
SetupP1     bic.b   #001h,&P1OUT            ; Clear P1.0
            bis.b   #001h,&P1DIR            ; P1.0 output
SetupP3     bis.b   #030h,&P3SEL            ; Use P3.4/P3.5 for USCI_A0
SetupUSCI0  bis.b   #UCSWRST,&UCA0CTL1      ; Set SW Reset
            mov.b   #UCSSEL_2+UCSWRST,&UCA0CTL1
                                            ; Use SMCLK, keep SW reset
            mov.b   #52,&UCA0BR0            ; 8MHz/52=153.8KHz
            mov.b   #0,&UCA0BR1             ;
            mov.b   #UCBRF_1+UCOS16,&UCA0MCTL
                                            ; Set 1st stage modulator to 1
                                            ; 16-times oversampling mode
            mov.b   #UCIRTXPL2+UCIRTXPL0+UCIRTXCLK+UCIREN,&UCA0IRTCTL
                                            ; Pulse length = 6 half clock cyc
                                            ; Enable BITCLK16, IrDA enc/dec
            bic.b   #UCSWRST,&UCA0CTL1      ; Resume operation
                                            ;
            clr.w   R4                      ; Init delay counter
            clr.b   R5                      ; TX data and pointer, 8-bit
                                            ;
Mainloop    inc.w   R4                      ; Small delay
            cmp.w   #1000,R4                ;
            jne     Mainloop                ;
            clr.w   R4                      ; Re-init delay counter
                                            ;
TX          bit.b   #UCA0TXIFG,&IFG2        ; USCI_A0 TX buffer ready?
            jnc     TX                      ; Loop if not
            mov.b   R5,&UCA0TXBUF           ; TX character
            dint                            ;
            bis.b   #UCA0RXIE,&IE2          ; Enable RX int
            bis.w   #CPUOFF+GIE,SR          ; Enter LPM0, interrupts enabled
            mov.b   &UCA0RXBUF,RxData(R5)   ; Store RXed character in RAM
            cmp.b   R5,&UCA0RXBUF           ; RX OK?
            jeq     RX_OK                   ;
                                            ;
RX_ERROR    bis.b   #001h,&P1OUT            ; LED P1.0 on
            jmp     $                       ; Trap PC here
                                            ;
RX_OK       inc.b   R5                      ; Next character to TX
            jmp     Mainloop                ; Again
                                            ;
;-------------------------------------------------------------------------------
USCIAB0RX_ISR; Clear UCA0RXIFG interrupt flag and return active
;-------------------------------------------------------------------------------
            bic.b   #UCA0RXIFG,&IFG2        ; Clear RX int flag
            bic.b   #UCA0RXIE,&IE2          ; Disable RX int
            bic.w   #CPUOFF,0(SP)           ; Return active after receiption
            reti                            ; Return from ISR
            
;------------------------------------------------------------------------------
;           Interrupt Vectors
;------------------------------------------------------------------------------
            .sect   ".reset"                ; MSP430 RESET Vector
            .short  RESET                   ;
            .sect   ".int07"                ; USCI Vector
            .short  USCIAB0RX_ISR           ;        
            .end

