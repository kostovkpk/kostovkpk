/*
 * File:   newmain.c
 * Author: kaloyan
 *
 * Created on December 3, 2022, 10:47 PM
 */
// PIC12F675 Configuration Bit Settings

// 'C' source line config statements

// CONFIG
#pragma config FOSC = INTRCIO   // Oscillator Selection bits (RC oscillator: I/O function on GP4/OSC2/CLKOUT pin, RC on GP5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-Up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // GP3/MCLR pin function select (GP3/MCLR pin function is digital I/O, MCLR internally tied to VDD)
#pragma config BOREN = OFF      // Brown-out Detect Enable bit (BOD disabled)
#pragma config CP = OFF         // Code Protection bit (Program Memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <pic12f675.h>//#include <pic12f675.inc>
#include <stdio.h>
#include <stdlib.h>

#define INPUT                       1
#define OUTPUT                      0
#define ENABLE                      1
#define DISABLE                     0
#define PRESSED                     0
#define DATA_1                      0b1011000000001010011010000 // IN
#define DATA_2                      0b1011000000001010011000100 // OUT
#define RF_PIN                      TRISIObits.TRISIO2
#define RF_STATE                    GPIObits.GP2
#define BUTTON1                     GPIObits.GP4
#define BUTTON2                     GPIObits.GP5
#define _XTAL_FREQ (4000000)

//Register setting PIC12F675
void PIC_INIT(void) {
    OSCCAL = 0b00100000;        // OSC COORECTION REGISTER 0x00 min 0xff max
    GPIO = 0;                   // Clear PORTA condition
    TRISIO = 0xff;              // all ports as input 
    ANSEL = 0;                  // Analog inputs Disabled
    ADCON0 = 0;                 // ADC cleared
    INTCON = 0;                 // Interrupts cleared
    CMCON = 0;                  // Comparator module cleared
    OPTION_REGbits.nGPPU = 0;   // General Pull-up resistors Enable
    WPUbits.WPU4 = 1;           // Pull-up on GP4 is enabled
    WPUbits.WPU5 = 1;           // Pull-up on GP5 is enabled
    RF_PIN = OUTPUT;            // Make RF pin as output
    INTCONbits.GIE = 1;         // 
    INTCONbits.GPIE = 1;        // Interrupt on change Enable
    INTCONbits.GPIF = 0;        // Flag bit common int cleared
    IOCbits.IOC4 = 1;           // Interrupt on change GPIO4 Enble
    IOCbits.IOC5 = 1;           // Interrupt on change GPIO5 Enble
}
// Affected registers after sleep and necessary set-up
void INIT_AFTER_SLEEP(void){
    INTCON = 0;
    GPIO = 0;
    INTCONbits.GIE = 1;
    INTCONbits.GPIE = 1;
}
/* Remote control info
 * Start with 0.3mS high state + 9.3mS low state
 * Each bit state is represent as:
 * 1 - 0.9 hi + 0.3 low
 * 0 - 0.3 hi + 0.9 low
 * summary 1.2mS front to front or 3640Hz
 */
union {
    unsigned long int CHAR;
    struct {
        unsigned char byte0 : 8;
        unsigned char byte1 : 8;
        unsigned char byte2 : 8;
        unsigned char bit25  : 1;
    } CHARbits;
} CHARBYTE;

int TRANSMIT_DATA(void){
    unsigned long int CURRENT_DATA = 0;
    
    if (BUTTON1 == PRESSED) {
        CURRENT_DATA = DATA_1;
        }
    else if (BUTTON2 == PRESSED) {
        CURRENT_DATA = DATA_2;
        }    
    else{
        return 0;
        }
    // Start condition send
    RF_STATE = 0;               // send start signal
    __delay_ms(1);           // wait 1.0 ms
    RF_STATE = 1;               // set state as high
    __delay_us(300);            // wait 0.3 ms
    RF_STATE = 0;
    __delay_us(9300);       // wait 9.3 ms.
    // Continuous sending of DATA when a button is continuously pressed
    while(BUTTON1 == PRESSED || BUTTON2 == PRESSED){
        CHARBYTE.CHAR = CURRENT_DATA;
        for (int x = 0; x < 25; x++) {                
            switch(CHARBYTE.CHARbits.bit25){
                case 0:
                    RF_STATE = 1;
                    __delay_us(300);
                    RF_STATE = 0;
                    __delay_us(900);
                    break;
                case 1:
                    RF_STATE = 1;
                    __delay_us(900);
                    RF_STATE = 0;
                    __delay_us(300);
                    break;           
                }
            CHARBYTE.CHAR <<= 1;    // rotate byte left
        }
        __delay_us(8400);       // wait 8.4ms.
    }
    return 1;
}

void main(void) {
    PIC_INIT();

    while(1){
        SLEEP();
        NOP();
      
        if (INTCONbits.GPIF = 1){
                        
            unsigned char but_count = 0;
            for (unsigned char execute = 0; execute <= 20;){

                if (BUTTON1 == PRESSED || BUTTON2 == PRESSED) // check for button pressed
                {
                    if (++but_count >= 10)    //after 1 seconds, count will reach 50
                    {
                        //code here will be executed when button has been pressed for 0.2 seconds
                        TRANSMIT_DATA();
                        but_count=0;    //start count from zero again
                    }
                } 
                else    //reset count if button not pressed
                {
                    but_count=0;
                    execute++;
                }
                // ADD delay after test    
                __delay_ms(20);    //20ms delay to get 50 loops per second
            }
            INIT_AFTER_SLEEP();
        }      
    }
}