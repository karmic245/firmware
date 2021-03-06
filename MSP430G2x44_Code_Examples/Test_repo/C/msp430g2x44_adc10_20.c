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
//   MSP430G2x44 Demo - ADC10, DTC Sample A0 2-Blk Cont. Mode, AVcc, HF XTAL
//
//   Description: For non-low power applications HF XTAL can source ACLK and be
//   used also for CPU MCLK, as in this example. DTC always uses MCLK for data
//   transfer. Though not required, ADC10 can also clock from HF XTAL as in this
//   example using MCLK.
//   MCLK = ACLK = ADC10CLK = HF XTAL in this example.
//   DTC used to sample A0 with reference to AVcc. Software writes once to
//   ADC10SC to trigger continous sampling. DTC configured to transfer code
//   into two 16-word blocks at RAM 200h - 240h. Mainloop jumps to itself.
//   MCLK times sample period (8x) and conversion (13x). For demostration
//   puspose, ADC10_ISR(DTC) used to set P1.1 at completion of first block
//   transfer, reset at completion of second block transfer. Toggle rate on P1.1
//   is then calculated as MCLK/(21*16*2)
//   //* HF XTAL NOT INSTALLED ON FET *//
//   //* Min Vcc required varies with MCLK frequency - refer to datasheet *//
//
//                MSP430G2x44
//             -----------------
//         /|\|              XIN|-
//          | |                 | HF XTAL (3 - 16MHz crystal or resonator)
//          --|RST          XOUT|-
//            |                 |
//        >---|P2.0/A0      P1.1|-->MCLK/672
//
//   William Goh
//   Texas Instruments Inc.
//   March 2013
//   Built with CCS Version: 5.3.0 and IAR Embedded Workbench Version: 5.51
//******************************************************************************
#include <msp430.h>

volatile unsigned int i;

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  BCSCTL1 |= XTS;                           // LFXT1 = HF XTAL
  BCSCTL3 |= LFXT1S1;                       // LFXT1S1 = 3-16Mhz
  do
  {
    IFG1 &= ~OFIFG;                         // Clear OSC fault flag
    i = 0xFF;                               // i = Delay
    while (i--);                            // Additional delay to ensure start
  }
  while (OFIFG & IFG1);                     // OSC fault flag set?
  BCSCTL2 |= SELM_3;                        // MCLK = LFXT1

  ADC10CTL1 = ADC10SSEL_2 + CONSEQ_2;
  ADC10CTL0 = ADC10SHT_1 + MSC + ADC10ON + ADC10IE;
  ADC10AE0 |= 0x01;                         // P2.0 ADC option select
  ADC10DTC0 |= ADC10TB + ADC10CT;           // Continous two block transfers
  ADC10DTC1 = 0x010;                        // 16*2 conversions
  P1DIR |= 0x02;                            // P1.1 output
  ADC10SA = 0x0200;                         // Data buffer start
  ADC10CTL0 |= ENC + ADC10SC;               // Start sampling continously
  __enable_interrupt();                     // Enable interrupts
  while(1);                                 // Do nothing (and keep MCLK alive)
}

#pragma vector = ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  if (ADC10DTC0 & ADC10B1)
    P1OUT &= ~0x002;                        // P1.1 = 0
  else
    P1OUT |= 0x002;                         // P1.1 = 1
}
