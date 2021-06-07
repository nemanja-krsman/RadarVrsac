
// CONFIG
#pragma config FOSC = XT        // Oscillator Selection bits (XT oscillator: Crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RA5/MCLR pin function select (RA5/MCLR pin function is MCLR)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOD Reset disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable bit (RB4/PGM pin has PGM function, low-voltage programming enabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection off)
#pragma config CP = OFF         // Code Protection bits (Program memory code protection off)

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <pic16f628.h>
#include "UART.h"
#include "soft_I2C.h"
#include "MapReg.h"


#define _XTAL_FREQ 4000000
#define TIME_ON    5 // Seconds after last load from radar

unsigned int  ms10_counter = 0;
unsigned int  sec_counter = 0;
unsigned char limit_sec = 0;
unsigned char radar_off = 0;
unsigned char Rx_data=0;


/*========================================================================*\
   Description: Setup all pin direction
\*========================================================================*/

void setup_pins(){
    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB7 = 0;
    TRISBbits.TRISB4 = 0;
    TRISBbits.TRISB5 = 0;
    TRISAbits.TRISA3 = 0;
    
    TRISBbits.TRISB6 = 0; // data
    TRISAbits.TRISA1 = 0; // clk
    TRISAbits.TRISA0 = 0; // le
    
    PORTBbits.RB0 = 0;
    PORTBbits.RB7 = 0;
    PORTBbits.RB4 = 0;
    PORTBbits.RB5 = 0;
    PORTAbits.RA3 = 0;
    
    PORTBbits.RB6 = 0;
    PORTAbits.RA1 = 0;
    PORTAbits.RA0 = 0;
    
    
    SDA_TRIS = 1;                   // SDA1 (RC4) = input
    SCL_TRIS = 0;                   // SCL1 (RC3) = output
    SCL = 1;                          // SCL is high
}

/*========================================================================*\
   Description: Turn ON green "HVALA"
\*========================================================================*/
void turn_on_green(){
    PORTBbits.RB5 = 0;
    PORTBbits.RB4 = 1;
}

/*========================================================================*\
   Description: Turn OFF green "HVALA"
\*========================================================================*/
void turn_off_green(){
    PORTBbits.RB5 = 0;
    PORTBbits.RB4 = 0;
}


/*========================================================================*\
   Description: Turn ON red "USPORI"
\*========================================================================*/
void turn_on_red(){
    PORTBbits.RB5 = 1;
    PORTBbits.RB4 = 0;
}

/*========================================================================*\
   Description: Turn OFF red "USPORI"
\*========================================================================*/
void turn_off_red(){
    PORTBbits.RB5 = 0;
    PORTBbits.RB4 = 0;
}

/*========================================================================*\
   Description: Make data for display time
\*========================================================================*/
unsigned char displayConvert(unsigned char data) {
    unsigned char format = 0;
    if (data == 0) {
        format = 0b11111100;
    } else if (data == 1) {
        format = 0b01100000;
    } else if (data == 2) {
        format = 0b11011010;
    } else if (data == 3) {
        format = 0b11110010;
    } else if (data == 4) {
        format = 0b01100110;
    } else if (data == 5) {
        format = 0b10110110;
    } else if (data == 6) {
        format = 0b10111110;
    } else if (data == 7) {
        format = 0b11100000;
    } else if (data == 8) {
        format = 0b11111110;
    } else if (data == 9) {
        format = 0b11110110;
    }
    else {
        format = 0b00000000;
    }
    return format;
}

/*========================================================================*\
   Description: Function for LED displays  
\*========================================================================*/
void shiftOut(unsigned char * DATA_PORT, unsigned char DATA_PIN, unsigned char * CLK_PORT, unsigned char CLK_PIN, unsigned char DATA_VAL) {

    unsigned char i = 0;
    unsigned char num = 1;
    unsigned char value = 0;

    for (i = 0; i < 8; i++) {
        value = (num & DATA_VAL) >> i; // FIND CORRESPONDIG BIT
        // Put this bit on his position by DATA_PIN 
        if (value) {
            *DATA_PORT = *DATA_PORT | (value << DATA_PIN);
        } else if (!value) {
            *DATA_PORT = *DATA_PORT & ~(1 << DATA_PIN);
        }
        // Clock signal 
        *CLK_PORT = *CLK_PORT | (1 << CLK_PIN);
        __delay_us(10);
        *CLK_PORT = *CLK_PORT & ~(1 << CLK_PIN);
        num = num * 2;
    }
}

/*========================================================================*\
   Description: Turn on 50 limit (50, 30 or anything else is to off)
\*========================================================================*/
void limit_display(unsigned char limit){
    if(limit==50){
        PORTAbits.RA3 = 1;
        PORTBbits.RB0 = 0;
        PORTBbits.RB7 = 1;
    }
    else if(limit==30){
        PORTAbits.RA3 = 1;
        PORTBbits.RB0 = 1;
        PORTBbits.RB7 = 0;
    }
    else{
        PORTAbits.RA3 = 0;
        PORTBbits.RB0 = 0;
        PORTBbits.RB7 = 0;
    }
}


/*========================================================================*\
   Description: Print digits on Radar display
\*========================================================================*/
void print_speed(unsigned char sp) {
    if(sp == 'X'){
        shiftOut(&PORTB, 6, &PORTA, 1, displayConvert('X'));
        shiftOut(&PORTB, 6, &PORTA, 1, displayConvert('X'));
    }
    else if(sp/10 != 0){
        shiftOut(&PORTB, 6, &PORTA, 1, displayConvert(sp/10));
        shiftOut(&PORTB, 6, &PORTA, 1, displayConvert(sp%10)); 
    }
    else{
        shiftOut(&PORTB, 6, &PORTA, 1, displayConvert('X'));
        shiftOut(&PORTB, 6, &PORTA, 1, displayConvert(sp));
    }

        PORTAbits.RA0 = 1;
        __delay_ms(1);
        PORTAbits.RA0 = 0;
}



/*========================================================================*\
   Description: Interrupt routine for timer
\*========================================================================*/

void interrupt TMR_ISR(){
    if(PIR1bits.TMR2IF){
        TMR2 = 0x83;
        PIR1bits.TMR2IF = 0;
        ms10_counter++;
        if(ms10_counter>=100){
            sec_counter++;
            limit_sec++;
            ms10_counter=0;
        }
        if(sec_counter>=TIME_ON){
            sec_counter = 0;
            limit_sec = 0;
            print_speed('X');
            limit_display('X');
            turn_off_red();
            turn_off_green();
            radar_off = 0;
        }
//        else if(limit_sec>=5){
//            limit_sec = 0;
//            limit_display('X');
//        }
        
    }
}

/*========================================================================*\
   Description: Timer 2 setup function 
\*========================================================================*/

void Timer2_Setup(){ // 10msec interrupt
    T2CONbits.TOUTPS = 0b0011; // Postscale 1:4
    T2CONbits.T2CKPS = 0b01;   // Prescale 1:4
    TMR2 = 0x83;
    PR2 = 0xff;
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    PIE1bits.TMR2IE = 1;
    PIR1bits.TMR2IF = 0;
    T2CONbits.TMR2ON = 1;   //TIMER IS INNITAL OFF
}

/*========================================================================*\
   Description: Timer reset
\*========================================================================*/

void TimerReset(){
    T2CONbits.TMR2ON = 0;
    sec_counter = 0;
    ms10_counter = 0;
    TMR2 = 0x83;
    T2CONbits.TMR2ON = 1;
}


/*========================================================================*\
   Description: Read Radar
\*========================================================================*/
unsigned char read_radar(){
    unsigned char speed = 0;
    unsigned char c1 = 0;
    unsigned char c2 = 0;
    unsigned char c3 = 0;
    unsigned char c4 = 0;
    unsigned char c5 = 0;
    
    c1 = UART_RxChar();
    c2 = UART_RxChar();
    c3 = UART_RxChar();
    
    if(c3 == 13){ // two digit
        c4 = UART_RxChar();
        if(c1>=48 && c1<=57 && c2>=48 && c2<=57)
        {
           speed = (c1-48)*10 + (c2-48);
        if(speed>45)
            speed = speed + 5;
        else
            speed = speed + 3; 
        }
        else 
            speed = 0;
    }
    else if(c3 == ' '){ // one digit
        if(c1>=48 && c1<=57)
        speed = c1 - 48;
        else
            speed = 0;
    }
    else if(c3>=48 && c3<=57){ // three digit
        c4 = UART_RxChar();
        c5 = UART_RxChar();
        if(c4 == 13 && c5 == ' ')
        speed = 99;
        else 
            speed = 0;
    }
    else {
        
        //c4 = UART_RxChar();
        //c5 = UART_RxChar();
        RCSTAbits.CREN = 0;
        __delay_ms(5);
        RCSTAbits.CREN = 1;
        speed = 0;
    }
    return speed;
}


/*========================================================================*\
   Description: Read DS1307 data
\*========================================================================*/
unsigned char read_ds1307(unsigned char address){
    unsigned char data = 0;
    
        i2c_start();
        i2c_wr(0xd0); // Writing
        i2c_wr(address); // address
        i2c_stop();
        i2c_start();
        i2c_wr(0xd1); // Reading
       // __delay_ms(100);
        data = i2c_rd(NACK);
        i2c_stop();
        return data;
}

/*========================================================================*\
   Description: Main
\*========================================================================*/
void main(void) {
    setup_pins();
    
    __delay_ms(5000);
    unsigned char brzina = 0;
    unsigned char sat = 0;
    CMCONbits.CM = 0b111; // Switch off comparators
    UART_Init(9600);
    
    
    //write_to_calendar();
    
    /* Block for adding started time 
     
     * Uncomment this for adding new time.
     
    i2c_start();
    i2c_wr(0b11010000);
    i2c_wr(0x01); // minutes
    i2c_wr(0x20);
    i2c_wr(0x19);
    i2c_stop();
    
    sat = read_ds1307(HOURS__ADD);
    minut = read_ds1307(MINUTES__ADD);
    sat = (sat>>4)*10 + (sat & 0x0f);
    minut = (minut>>4)*10 + (minut & 0x0f);
    */
   
    
    sat = read_ds1307(HOURS__ADD);
        //minut = read_ds1307(MINUTES__ADD);
        sat = (sat>>4)*10 + (sat & 0x0f);
        //minut = (minut>>4)*10 + (minut & 0x0f);
        if(sat==14){
            limit_display(50);
        }
        
        
        Timer2_Setup();
    
    while(1){
        
        brzina = read_radar();
        TimerReset();
        if(brzina!=0)
        print_speed(brzina);
        
        sat = read_ds1307(HOURS__ADD);
        //minut = read_ds1307(MINUTES__ADD);
        sat = (sat>>4)*10 + (sat & 0x0f);
        //minut = (minut>>4)*10 + (minut & 0x0f);
        
        
        
        /* ------------------OGRANICENJE 30------------------*/
        if(sat<19 && sat>6){ // Ogranicenje 30
            if(!radar_off){
                limit_sec = 0;
                limit_display(30); // OVDE SU NA JEDNOM RADARU OBRNUTE ZICE PA PISE 50 UEMSTO 30
                radar_off = 1;
            }
            if(brzina>30){
                turn_off_green();
                turn_on_red();
            }
            
            else{
                turn_off_red();
                turn_on_green();
            }
        }
        
        /* ------------------OGRANICENJE 50------------------*/
        else{ // Ogranicenje 50
            if(!radar_off){
                limit_sec = 0;
                limit_display(50); // // OVDE SU NA JEDNOM RADARU OBRNUTE ZICE PA PISE 30 UEMSTO 50
                radar_off = 1;
            }
            if(brzina>50){
                turn_off_green();
                turn_on_red();
            }
            
            else{
                turn_off_red();
                turn_on_green();
            }
        }
        
        TimerReset(); // After 3 seconds from last load radar is off
        
    } // While
    return;
}

