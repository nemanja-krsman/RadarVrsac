#include <pic16f628.h>
#include <stdio.h>
#include <stdlib.h>
#include "UART.h"
#include <xc.h>
#define _XTAL_FREQ 4000000


void UART_Init(int baudRate)
{    
    TRISBbits.TRISB1 = 1; // RX
    TRISBbits.TRISB2 = 0; // TX
    TXSTA=(1<<SBIT_TXEN)| (1<<2);;  // Asynchronous mode, 8-bit data & enable transmitter
    RCSTA=(1<<SBIT_SPEN) | (1<<SBIT_CREN);  // Enable Serial Port and 8-bit continuous receive
    SPBRG = 25; //baudrate je 9615 
    //INTCONbits.GIE = 1;
    //INTCONbits.PEIE = 1;
    PIE1bits.RCIE = 0; //omogucen prekid pri primanju
    PIR1bits.RCIF = 0; //interrupt fleg
}


void UART_TxChar(char ch)
{
    while(PIR1bits.TXIF==0);    // Wait till the transmitter register becomes empty
    PIR1bits.TXIF=0;            // Clear transmitter flag
    TXREG=ch;          // load the char to be transmitted into transmit reg
}


char UART_RxChar()
{
    if(RCSTAbits.OERR){
        RCSTAbits.CREN = 0;
        __delay_ms(5);
        RCSTAbits.CREN = 1;
    }
    while(PIR1bits.RCIF==0);    // Wait till the data is received 
    PIR1bits.RCIF=0;            // Clear receiver flag
    return(RCREG);     // Return the received data to calling function
}

void UART_putst(register const char *str)
{
    while ((*str)!=0)
    {
        UART_TxChar(*str);
        if(*str==13) UART_TxChar(13);
        if(*str==10) UART_TxChar(10);
        
        str++;
    }
}

