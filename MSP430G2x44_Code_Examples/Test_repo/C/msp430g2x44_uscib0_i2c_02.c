/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 * 
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430G2x44 Demo - USCI_B0 I2C Master Interface to PCF8574, Read/Write
//
//  Description: I2C communication with a PCF8574 in read and write mode is
//  demonstrated. PCF8574 port P is configured with P0-P3 input, P4-P7. Read
//  P0-P3 input data is written back to Port P4-P7. This example uses the
//  RX ISR and generates an I2C restart condition while switching from
//  master receiver to master transmitter.
//  ACLK = n/a, MCLK = SMCLK = TACLK = BRCLK = default DCO = ~1.2MHz
//
//                                MSP430G2x44
//                              -----------------
//                  /|\ /|\ /|\|              XIN|-
//                  10k 10k  | |                 |
//       PCF8574     |   |   --|RST          XOUT|-
//       ---------   |   |     |                 |
//  --->|P0    SDA|<-|---+---->|P3.1/UCB0SDA     |
//  --->|P1       |  |         |                 |
//  --->|P2       |  |         |                 |
//  --->|P3    SCL|<-+---------|P3.2/UCB0SCL     |
//  <---|P4       |            |                 |
//  <---|P5       |            |                 |
//  <---|P6       |            |                 |
//  <---|P7       |            |                 |
//   +--|A0,A1,A2 |            |                 |
//   |  |         |            |                 |
//  \|/
//
//  William Goh
//  Texas Instruments Inc.
//  March 2013
//  Built with CCS Version: 5.3.0 and IAR Embedded Workbench Version: 5.51
//******************************************************************************
#include <msp430.h>

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop Watchdog Timer
  P3SEL |= 0x06;                            // Assign I2C pins to USCI_B0
  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
  UCB0CTL0 = UCMST+UCMODE_3+UCSYNC;         // I2C Master, synchronous mode
  UCB0CTL1 = UCSSEL_2+UCSWRST;              // Use SMCLK, keep SW reset
  UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
  UCB0BR1 = 0;
  UCB0I2CSA = 0x20;                         // Set slave address
  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
  IE2 |= UCB0RXIE;                          // Enable RX interrupt
  TACCTL0 = CCIE;                           // TACCR0 interrupt enabled
  TACTL = TASSEL_2 + MC_2;                  // SMCLK, contmode

  while (1)
  {
    __bis_SR_register(CPUOFF + GIE);        // CPU off, interrupts enabled
    UCB0CTL1 &= ~UCTR;                      // I2C RX
    UCB0CTL1 |= UCTXSTT;                    // I2C start condition
    while (UCB0CTL1 & UCTXSTT);             // Loop until I2C STT is sent
    UCB0CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
    __bis_SR_register(CPUOFF + GIE);        // CPU off, interrupts enabled
    while (UCB0CTL1 & UCTXSTT);             // Loop until I2C STT is sent
    UCB0CTL1 |= UCTXSTP;                    // I2C stop condition after 1st TX
  }
}

#pragma vector = TIMERA0_VECTOR
__interrupt void TA0_ISR(void)
{
  __bic_SR_register_on_exit(CPUOFF);        // Exit LPM0
}

// USCI_B0 Data ISR
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
  UCB0TXBUF = (UCB0RXBUF << 4) | 0x0f;      // Move RX data to TX
  __bic_SR_register_on_exit(CPUOFF);        // Exit LPM0
}
