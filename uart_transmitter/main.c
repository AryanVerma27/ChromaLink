/*
 * ChromaLink — UART Communication Between Two MSP432 Boards : TRANSMITTER
 *
 * Board : MSP-EXP432P401R (Code Composer Studio + SimpleLink MSP432 SDK)
 *
 *   PC terminal  <--USB/eUSCI_A0 (P1.2 RX, P1.3 TX)-->  [Transmitter]
 *   [Transmitter] --eUSCI_A2 TX (P3.3)--> RX (P3.2) [Receiver]
 *
 * A keystroke typed in the PC terminal arrives on eUSCI_A0 (RX interrupt),
 * is echoed back to the terminal and forwarded to the receiver over eUSCI_A2.
 *
 * UART: 8N1, 9600 baud, SMCLK = 3 MHz (default DCO)
 *   N = 3 000 000 / 9600 = 312.5  -> OS16 = 1, BRW = 19, BRF = 8, BRS = 0x55
 */

#include "msp.h"
#include <stdint.h>
#include <stdbool.h>

static void uart_init_9600(EUSCI_A_Type *uart);
void sendString(char *str);
void sendChar(char s);
void echoChar(char s);

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;      // stop watchdog timer

    // eUSCI_A0 pins (backchannel UART to PC): P1.2 RX, P1.3 TX
    P1->SEL0 |=  (BIT2 | BIT3);
    P1->SEL1 &= ~(BIT2 | BIT3);

    // eUSCI_A2 pins (link to receiver board): P3.2 RX, P3.3 TX
    P3->SEL0 |=  (BIT2 | BIT3);
    P3->SEL1 &= ~(BIT2 | BIT3);

    uart_init_9600(EUSCI_A0);
    uart_init_9600(EUSCI_A2);

    EUSCI_A0->IE |= EUSCI_A_IE_RXIE;                 // interrupt on keystroke from PC
    NVIC->ISER[0] = 1 << (EUSCIA0_IRQn & 31);
    __enable_irq();

    sendString("Enter r for red, g for green, b for blue!\r\n");

    while (1)
    {
        // everything is interrupt driven
    }
}

// 8N1, 9600 baud from 3 MHz SMCLK with 16x oversampling
static void uart_init_9600(EUSCI_A_Type *uart)
{
    uart->CTLW0  = EUSCI_A_CTLW0_SWRST;              // hold in reset while configuring
    uart->CTLW0 |= EUSCI_A_CTLW0_SSEL__SMCLK;        // clock = SMCLK
    uart->BRW    = 19;                                // coarse prescaler
    uart->MCTLW  = (0x55 << EUSCI_A_MCTLW_BRS_OFS) |  // second-stage modulation
                   (8    << EUSCI_A_MCTLW_BRF_OFS) |  // first-stage modulation
                   EUSCI_A_MCTLW_OS16;                // oversampling
    uart->CTLW0 &= ~EUSCI_A_CTLW0_SWRST;             // release for operation
}

void EUSCIA0_IRQHandler(void)
{
    if (EUSCI_A0->IFG & EUSCI_A_IFG_RXIFG)
    {
        char c = EUSCI_A0->RXBUF;    // reading RXBUF clears RXIFG
        echoChar(c);                 // show the key in the PC terminal
        sendChar(c);                 // forward to receiver board
    }
}

// String to PC terminal (eUSCI_A0)
void sendString(char *str)
{
    int i = 0;
    while (str[i] != '\0')
    {
        while (!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));   // wait until TXBUF empty
        EUSCI_A0->TXBUF = str[i];
        i++;
    }
}

// Single character to PC terminal (eUSCI_A0)
void echoChar(char s)
{
    while (!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));
    EUSCI_A0->TXBUF = s;
}

// Single character to receiver board (eUSCI_A2)
void sendChar(char s)
{
    while (!(EUSCI_A2->IFG & EUSCI_A_IFG_TXIFG));
    EUSCI_A2->TXBUF = s;
}
