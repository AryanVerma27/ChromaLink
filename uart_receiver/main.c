/*
 * ChromaLink — UART Communication Between Two MSP432 Boards : RECEIVER
 *
 * Board : MSP-EXP432P401R (Code Composer Studio + SimpleLink MSP432 SDK)
 *
 * Receives one character on eUSCI_A2 (P3.2 RX) at 9600 baud, 8N1 and drives LEDs:
 *   'r' -> LED2 RED   (P2.0)
 *   'g' -> LED2 GREEN (P2.1)
 *   'b' -> LED2 BLUE  (P2.2)
 *   any other character -> LED2 off, LED1 (P1.0) blinks twice (error)
 */

#include "msp.h"
#include <stdint.h>

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;      // stop watchdog timer

    // LED1 (error) - P1.0
    P1->DIR  |=  BIT0;
    P1->OUT  &= ~BIT0;
    P1->SEL0 &= ~BIT0;
    P1->SEL1 &= ~BIT0;

    // LED2 (RGB) - P2.0 / P2.1 / P2.2
    P2->DIR  |=  (BIT0 | BIT1 | BIT2);
    P2->OUT  &= ~(BIT0 | BIT1 | BIT2);
    P2->SEL0 &= ~(BIT0 | BIT1 | BIT2);
    P2->SEL1 &= ~(BIT0 | BIT1 | BIT2);

    // eUSCI_A2 pins: P3.2 RX, P3.3 TX
    P3->SEL0 |=  (BIT2 | BIT3);
    P3->SEL1 &= ~(BIT2 | BIT3);

    // eUSCI_A2: 8N1, 9600 baud from 3 MHz SMCLK
    EUSCI_A2->CTLW0  = EUSCI_A_CTLW0_SWRST;
    EUSCI_A2->CTLW0 |= EUSCI_A_CTLW0_SSEL__SMCLK;
    EUSCI_A2->BRW    = 19;
    EUSCI_A2->MCTLW  = (0x55 << EUSCI_A_MCTLW_BRS_OFS) |
                       (8    << EUSCI_A_MCTLW_BRF_OFS) |
                       EUSCI_A_MCTLW_OS16;
    EUSCI_A2->CTLW0 &= ~EUSCI_A_CTLW0_SWRST;
    EUSCI_A2->IE    |= EUSCI_A_IE_RXIE;

    NVIC->ISER[0] = 1 << (EUSCIA2_IRQn & 31);
    __enable_irq();

    while (1)
    {
        // everything is interrupt driven
    }
}

void EUSCIA2_IRQHandler(void)
{
    if (EUSCI_A2->IFG & EUSCI_A_IFG_RXIFG)
    {
        char received_char = EUSCI_A2->RXBUF;   // reading clears RXIFG

        if (received_char == 'r')
        {
            P2->OUT ^= BIT0;                    // toggle red
            P2->OUT &= ~(BIT1 | BIT2);          // green, blue off
        }
        else if (received_char == 'g')
        {
            P2->OUT ^= BIT1;                    // toggle green
            P2->OUT &= ~(BIT0 | BIT2);          // red, blue off
        }
        else if (received_char == 'b')
        {
            P2->OUT ^= BIT2;                    // toggle blue
            P2->OUT &= ~(BIT0 | BIT1);          // red, green off
        }
        else
        {
            // Invalid character: LED2 off, blink LED1 twice (4 toggles)
            volatile uint32_t i, j;
            P2->OUT &= ~(BIT0 | BIT1 | BIT2);
            for (j = 0; j < 4; j++)
            {
                P1->OUT ^= BIT0;
                for (i = 0; i < 30000; i++);    // crude visible delay
            }
            P1->OUT &= ~BIT0;
        }
    }
}
