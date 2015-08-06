/*
 * File:   main.c (Master Code)
 * Author: PaoloDantes & BrandonZhang
 *
 * Created on April 17, 2015, 2:23 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "p30FXXXX.h"



// IO definitions
#define LEDRED  LATEbits.LATE3 // RED
#define LEDYLW  LATEbits.LATE4 // YELLOW
#define LEDGRN  LATEbits.LATE5 // GREEN
#define TRISRED  TRISEbits.TRISE3 // YELLOW
#define TRISYLW  TRISEbits.TRISE4 // YELLOW
#define TRISGRN  TRISEbits.TRISE5 // GREEN
#define UART_TX_LEN 50


// CAN Operation Modes
#define CONFIG_MODE     4
#define LISTEN_ALL      7
#define LISTEN_ONLY     3
#define LOOPBACK        2
#define DISABLE         1
#define NORMAL          0

// CAN bit timing
#define FOSC        20000000 // 20Mhz Crystal
#define FCY         FOSC/4
#define BITRATE     1000000  // 100kbps
#define NTQ         16      // Amount of Tq per bit time
#define BRP_VAL     (((4*FCY)/(2*NTQ*BITRATE))-1) // refer to pg. 693 of Family Reference

// UART baud rate
#define UART_BAUD   115000
#define UART_BRG    (FCY/(16*UART_BAUD))-1 // refer to pg. 506 of family reference


// Define PID controls
#define PID_KP  2.0
#define PID_KD  0.1
#define PID_KI  0
#define PID_TI  0
#define PID_TD  0
#define PID_TS  10
#define PID_N   10

// Define PWM constants
#define PWM_FREQUENCY       16000
#define PWM_PRESCALER       0       // 0=1:1 1=1:4 2=1:16 3=1:64
#define PWM_COUNTS_PERIOD   (FCY/PWM_FREQUENCY)-1

// Define PWM PID controller constants
#define PID_PWM_FREQUENCY       16000.0
#define PID_FOSC            20000000.0
#define PID_FCY             PID_FOSC/4.0
#define PID_PWM_COUNTS_PERIOD   (FCY/PWM_FREQUENCY)-1.0

// pid_t type
typedef struct {
    float Kp, Kd, Ki, T;
    unsigned short N; // derivative filter parameter (3-20)
    float i, ilast; // integral --> NOT USED IN THIS VERSION. FOR FUTURE IMPLEMENTATION
    float y, ylast; // output
    float d, dlast; // derivative term
    float u; // u=uc-y
    float e, elast; // error
} pid_t;
pid_t mypid;


// Misc. variables

//InData0 initialization (InData0[0] = POSCNT(Master), InData0[1] = POSCNT(Slave))
unsigned int InData0[4] = {0, 30000, 0, 0};  
unsigned int targetPos = 30000; //Motor initial Target position

/////////////////////////////////////////////////////////////
/////       CAN BUS FILTER TESTING (DOESNT WORK)       //////
/////////////////////////////////////////////////////////////

//void InitCan(void) {
//    // Initializing CAN Module Control Register
//    C1CTRLbits.REQOP = CONFIG_MODE; // 4 = Configuration mode
//
//    C1CTRLbits.CANCAP = 1; // Enable CAN capture
//    C1CTRLbits.CSIDL = 0; // 0 = Continue CAN module op in idle mode
//    C1CTRLbits.CANCKS = 0; // 1: Fcan=Fcy 0: Fcan=4Fcy
//
//    C1CFG1bits.SJW = 0; // Synchronized jump width is 1xTq
//    C1CFG1bits.BRP = 4; // Baud rate prescaler = 20 (CAN baud rate of 125kHz
//
//    C1CFG2bits.SEG2PHTS = 1; // 1=Freely Programmable 0=Maximum of SEG1PH or 3Tq's whichever is greater
//    C1CFG2bits.PRSEG = 1; // Propagation Segment = 2Tq
//    C1CFG2bits.SEG1PH = 6; // Phase Buffer Segment 1 = 7Tq
//    C1CFG2bits.SEG2PH = 5; // Phase Buffer Segment 2 = 6Tq
//    C1CFG2bits.SAM = 1; // 1=Bus line sampled 3 times 0=Bus line sampled once
//
//    // Initializing CAN interrupt
//    C1INTF = 0; // Reset all CAN interrupts
//    IFS1bits.C1IF = 0; // Reset Interrupt flag status register
//    C1INTE = 0x00FF; // Enable all CAN interrupt sources
////    IPC6bits.C1IP = 6; // CAN 1 Module interrupt is priority 6
//    IEC1bits.C1IE = 1; // Enable CAN1 Interrupt
//
//    /*---------------------------------------------------------
//     *  CONFIGURE RECEIVER REGISTERS, FILTERS AND MASKS
//     *---------------------------------------------------------*/
//
//    // Configure CAN Module Receive Buffer Register
//    C1RXF0SIDbits.EXIDE = 0; //turn of extended bit
//    C1RX0CONbits.DBEN = 0; // Buffer 0 does not overflow in buffer 1
//    C1RX0CONbits.FILHIT = 0; // Buffer 0 = Acceptance filter 0
////    C1RX1CONbits.FILHIT = 0; // Buffer 1 = Acceptance filter 2
//
//    // Initializing Acceptance Mask Register
//    C1RXM0SID = 0x1FFD; // SID = 00000011111
////    C1RXM1SID = 0x0000; // SID = 00000000000
//
//    // Initializing Message Acceptance filter
//    C1RXF0SID = 0x0020; //
//    C1RXF1SID = 0x0000; //
//    C1RXF2SID = 0x0000; //
//    C1RXF3SID = 0x0000; //
//    C1RXF4SID = 0x0000; // SID = 00000000000
//    C1RXF5SID = 0x0000; // SID = 00000000000
//
//    /*---------------------------------------------------------
//     *  CONFIGURE RECEIVER REGISTERS, FILTERS AND MASKS
//     *---------------------------------------------------------*/
//
//    // Configure CAN Module Transmit Buffer Register
//    C1TX0CONbits.TXPRI = 1; // 1 = High message priority
////    C1TX2CONbits.TXPRI = 2; // 2 = High intermediate message priority
//    C1TX0SIDbits.TXIDE = 0;//turn off extended bit
//
//    // Initializing Transmit SID
//     //          12345678901
//    C1TX0SID = 0B10101110001; // SID = 01010101010
////    C1TX1SID = 0X0000; // SID = 10101010101
//    C1TX0DLCbits.DLC = 8; // Data length is 8bytes
////    C1TX1DLCbits.DLC = 8; // Data length is 8bytes
//
//}


//CAN BUS initialization
void InitCan(void) {
    // Initializing CAN Module Control Register
    C1CTRLbits.REQOP = CONFIG_MODE; // 4 = Configuration mode
    C1CTRLbits.CANCAP = 1; // Enable CAN capture
    C1CTRLbits.CSIDL = 0; // 0 = Continue CAN module op in idle mode
    C1CTRLbits.CANCKS = 0; // 1: Fcan=Fcy 0: Fcan=4Fcy
    C1CFG1bits.SJW = 0; // Synchronized jump width is 1xTq
    C1CFG1bits.BRP = 1; // Baud rate prescaler = 20 (CAN baud rate of 100kHz) BRP = 1 means maximum CAN Bus Baud rate (should be 1mhz)
    C1CFG2bits.SEG2PHTS = 1; // 1=Freely Programmable 0=Maximum of SEG1PH or 3Tq's whichever is greater
    C1CFG2bits.PRSEG = 1; // Propagation Segment = 2Tq
    C1CFG2bits.SEG1PH = 6; // Phase Buffer Segment 1 = 7Tq
    C1CFG2bits.SEG2PH = 5; // Phase Buffer Segment 2 = 6Tq
    C1CFG2bits.SAM = 1; // 1=Bus line sampled 3 times 0=Bus line sampled once

    // Initializing CAN interrupt
    C1INTF = 0; // Reset all CAN interrupts
    IFS1bits.C1IF = 0; // Reset Interrupt flag status register
    C1INTE = 0x00FF; // Enable all CAN interrupt sources
    IEC1bits.C1IE = 1; // Enable CAN1 Interrupt

    /*---------------------------------------------------------
     *  CONFIGURE RECEIVER REGISTERS, FILTERS AND MASKS
     *---------------------------------------------------------*/

    // Configure CAN Module Receive Buffer Register
    C1RX0CONbits.DBEN = 0; // Buffer 0 does not overflow in buffer 1

    // Initializing Acceptance Mask Register
    C1RXM0SID = 0x1FFD;

    // Initializing Message Acceptance filter
    C1RXF0SID = 0x0AA8; // 0x0FA

    /*---------------------------------------------------------
     *  CONFIGURE RECEIVER REGISTERS, FILTERS AND MASKS
     *---------------------------------------------------------*/

    // Configure CAN Module Transmit Buffer Register
    C1TX0CONbits.TXPRI = 2; // 2 = High intermediate message priority

    // Initializing Transmit SID
    C1TX0SID = 0X50A8;
    C1TX0DLCbits.DLC = 8; // Data length is 8bytes

    // Data Field 1,Data Field 2, Data Field 3, Data Field 4 // 8 bytes selected by DLC

//    C1TX0B1 = OutData0[0];
//    C1TX0B2 = OutData0[1];
//    C1TX0B3 = OutData0[2];
//    C1TX0B4 = OutData0[3];

    C1CTRLbits.REQOP = NORMAL;
    while (C1CTRLbits.OPMODE != NORMAL); //Wait for CAN1 mode change from Configuration Mode to Loopback mode
}

//QEI Initialization
void InitQEI(void) {
    ADPCFG |= 0x0038; // RB3, RB4, RB5 configured to digital pin
    QEICONbits.QEIM = 0; // Disable QEI module
    QEICONbits.CNTERR = 0; // Clear any count errors
    QEICONbits.QEISIDL = 0; // Continue operation during sleep
    QEICONbits.SWPAB = 0; // QEA and QEB not swapped
    QEICONbits.PCDOUT = 0; // Normal I/O pin operation
    QEICONbits.POSRES = 1; // Index pulse resets position counter
    QEICONbits.TQCS = 0; // Internal clock source (Fcy) = 2Mhz
    DFLTCONbits.CEID = 1; // Count error interrupts disabled
    DFLTCONbits.QEOUT = 1; // Digital filters output enabled for QEn pins
    DFLTCONbits.QECK = 2; // 1:4 clock divide for digital filter for QEn
                            // FILTER_DIV = (MIPS*FILTERED_PULSE)/3
                            //  ==> 5MHz*5usec/3 = 3.33 --> 2
    POSCNT = 30000; // Reset position counter
    MAXCNT = 0xFFFF; // 65,535
    QEICONbits.QEIM = 7; // X4 mode with position counter reset by
    // 6 - index pulse
    // 7 - match (MAXCNT)
    return;
}

//PWM initialization
void InitPwm(void) {
    PTCONbits.PTEN = 0; // Disable PWM timerbase
    PTCONbits.PTCKPS = PWM_PRESCALER; // prescaler
    PTCONbits.PTOPS = 0; // 1:1 postscaler
    PTCONbits.PTMOD = 0; // free running mode
    PWMCON1bits.PMOD1 = 1; // PWM in independent mode
    PWMCON1bits.PMOD2 = 1; // PWM in independent mode
    PWMCON1bits.PEN1L = 1; // PWM1L (PIN 24) output controlled by PWM
    PWMCON1bits.PEN2L = 1; // PWM2L (PIN 26) output controlled by PWM
    PTMR = 0; // PWM counter value start at 0
    PTPER = PWM_COUNTS_PERIOD; // Set PWM period
    return;
}

//Uart initialization
void InitUart() {
    U1MODEbits.UARTEN = 0; // UART is disabled
    U1MODEbits.USIDL = 0; // Continue operation in Idle Mode
    U1MODEbits.ALTIO = 1; // UART communicates using U1ATX and U1ARX (pins 11&12)
    U1MODEbits.WAKE = 1; // Enable wake-up on Start bit detec durign sleep mode
    U1MODEbits.PDSEL = 0; // 8-bit data, no parity
    U1MODEbits.STSEL = 0; // 2 stop bits

    U1STAbits.UTXISEL = 0; // Interrupt when TX buffer has one character empty
    U1STAbits.UTXBRK = 0; // U1TX pin operates normally
    U1STAbits.URXISEL = 0; // Interrupt when word moves from REG to RX buffer
    U1STAbits.ADDEN = 0; // Address detect mode disabled

    //    U1BRG = 11;                 // p.507 of family reference
    //                                // 9600 baud rate for FOSC = 7.37MHz
    //    U1BRG = (unsigned int) UART_BRG;           // p.507 of family reference
    //                                // 115000 baud rate for FOSC = 20MHz
    U1BRG = 7; // p.507 of family reference
    // 38400 baud rate for FOSC = 20MHz

    IFS0bits.U1TXIF = 0; // Clear U1TX interrupt
    IFS0bits.U1RXIF = 0; // Clear U1RX interrupt
    IPC2bits.U1TXIP = 5; // U1TX interrupt 5 priority
    IPC2bits.U1RXIP = 5; // U1RX interrupt 5 priority
    IEC0bits.U1TXIE = 1; // Enable U1TX interrupt
    IEC0bits.U1RXIE = 1; // Enable U1RX interrupt

    U1MODEbits.LPBACK = 0; // Enable loopback mode
    U1MODEbits.UARTEN = 1; // UART is enabled
    U1STAbits.UTXEN = 1; // U1TX pin enabled

}

//Timer 1 initialization (Motor PWM output (Haptic Feedback))
void InitTmr1(void) {
    TMR1 = 0; // Reset timer counter
    T1CONbits.TON = 0; // Turn off timer 1
    T1CONbits.TSIDL = 0; // Continue operation during sleep
    T1CONbits.TGATE = 0; // Gated timer accumulation disabled
    T1CONbits.TCS = 0; // Use Tcy as source clock
    T1CONbits.TCKPS = 0; // Tcy/1 as input clock
    PR1 = 5000;        // (5000 = 500hz)
    IFS0bits.T1IF = 0; // Clear timer 1 interrupt flag
    IEC0bits.T1IE = 1; // Enable timer 1 interrupts
    IPC0bits.T1IP = 7; // Enable timer 1 interrupts
    return;
}


//Timer 2 initialization (CAN bus Transfer)
void InitTmr2(void)
{
   TMR2 = 0;                // Reset timer counter
   T2CONbits.TON = 0;       // Turn off timer 2
   T2CONbits.TSIDL = 0;     // Continue operation during sleep
   T2CONbits.TGATE = 0;     // Gated timer accumulation disabled
   T2CONbits.TCS = 0;       // Use Tcy as source clock
   T2CONbits.TCKPS = 0;     // Tcy/1 as input clock
   PR2 = 10000;              // Interrupt period = 10ms
   IFS0bits.T2IF = 0;       // Clear timer 2 interrupt flag
   IEC0bits.T2IE = 1;       // Enable timer 2 interrupts
   IPC1bits.T2IP = 7;       // Enable timer 2 interrupts
   return;
}

//Timer 3 initialization (for initialization phase)
void InitTmr3(void)
{
   TMR3 = 0;                // Reset timer counter
   T3CONbits.TON = 0;       // Turn off timer 2
   T3CONbits.TSIDL = 0;     // Continue operation during sleep
   T3CONbits.TGATE = 0;     // Gated timer accumulation disabled
   T3CONbits.TCS = 0;       // Use Tcy as source clock
   T3CONbits.TCKPS = 0;     // Tcy/1 as input clock
   PR3 = 1000;              // Interrupt period = 10ms
   IFS0bits.T3IF = 0;       // Clear timer 2 interrupt flag
   IEC0bits.T3IE = 1;       // Enable timer 2 interrupts
   IPC1bits.T3IP = 7;       // Enable timer 2 interrupts
   return;
}

//Delay function
void msDelay(unsigned int mseconds) //For counting time in ms
//-----------------------------------------------------------------
{
    int i;
    int j;
    for (i = mseconds; i; i--) {
        for (j = 714; j; j--) {
            // 5667 for XT_PLL8 -> Fcy = 34MHz
            // 714 for 20MHz oscillator -> Fcy=4.3MHz
            // 265 for FRC
        } // 1ms/(250ns/instruction*4instructions/forloop)=1000 for loops
    }
    return;
}

//PID initialization
void InitPid(pid_t *p, float kp, float kd, float ki, float T, unsigned short N, float il, float yl, float dl, float el) {
    p->Kp = kp;
    p->Kd = kd;
    p->Ki = ki;
    p->T = T;
    p->N = N;
    p->ilast = il;
    p->ylast = yl;
    p->dlast = dl;
    p->elast = el;
}

//PID calcuation function as well motor PWM output
void CalcPid(pid_t *mypid)
{
    volatile float pidOutDutyCycle;
    volatile int target = InData0[1];//(int)targetPos;
    volatile int motorPos = (int)POSCNT;
    volatile float error = 0.0;

    //Calculates error through Targeted position and current motor position (Float)
    error = (float)(target - motorPos);

    //PID Output duty cycle based on error (Float)
    pidOutDutyCycle = (mypid->Kp*error);

    //Motor PWM Saturation
    if (pidOutDutyCycle >= 997.0) pidOutDutyCycle = 997.0;
    if (pidOutDutyCycle <= -997.0) pidOutDutyCycle = -997.0;

    //Motor CW Operation
    if (pidOutDutyCycle < 0.0){
        PDC1 = 0;
        PDC2  = (unsigned int)((-1.0)*(pidOutDutyCycle));
    }
    //Motor CCW Operation
    else {
        PDC1 = (unsigned int)(1.0)*(pidOutDutyCycle);
        PDC2  = 0;
    }
    return;
}

//Update PID
void UpdatePid(pid_t *mypid) {
    // Update PD variables
    mypid->ylast = mypid->y;
    mypid->dlast = mypid->d;
    mypid->elast = mypid->e;
}

int main() {
//    char txData[UART_TX_LEN] = {'\0'};
    //    char rxData[UART_TX_LEN] = {'\0'};

    //ISR initializations
    //    InitUart();
    InitQEI();
    InitPwm();
    InitTmr3(); // for pid

    //LED PORT initializations
    TRISRED = 0; // PORTE output
    TRISYLW = 0; // PORTE output
    TRISGRN = 0;

    LEDRED = 0;
    LEDYLW = 0;
    LEDGRN = 0;

    // Enable PWM Module
    PTCONbits.PTEN = 1;

    // Initialize PID
    InitPid(&mypid, PID_KP, PID_KD, PID_KI, PID_TS, PID_N, 0.0, 0.0, 0.0, 0.0);

    //Turn on timer 3 (Motor PWM Calculation for initialization)
    T3CONbits.TON = 1;

    //Initialization phase move motor back for 7000 counts
    unsigned int i = 0;
    for(i = 0; i < 1000; i++){
        InData0[1] = InData0[1] - 7;
        msDelay(1);
        LEDYLW = 1;
    }

    //turn of CalcPid and all pwm to motors
    LEDYLW = 0;
    PDC1 = 0;
    PDC2 = 0;
    T3CONbits.TON = 0;

    //reinitialize QEI values
    POSCNT = 30000; // This prevents under and overflow of the POSCNT register
    InData0[1] = 30000;
    msDelay(1);

    //turn HapticFeedback back on as well as initialize CAN bus
    InitTmr1();
    InitTmr2();
    InitCan();

    // Enable CAN module
    C1CTRLbits.REQOP = NORMAL;
    while (C1CTRLbits.OPMODE != NORMAL);

    //Turn on timer 1 (Motor PWM Calculation (Haptic Feedback))
    T1CONbits.TON = 1;

    //Turn on timer 2 (CAN bus message transmission)
    T2CONbits.TON = 1;

    while (1) {

        // Uart function
        //        sprintf(txData, "KP: %f Kd: %f Ki: %f\r\n", mypid.Kp, mypid.Kd, mypid.Ki);
        //        for (i = 0; i < UART_TX_LEN; i++) {
        //            U1TXREG = txData[i];
        //            while (!(U1STAbits.TRMT));
        //        }

    } //while
} // main


// Can Bus interrupt
void __attribute__((interrupt, no_auto_psv)) _C1Interrupt(void) {
    IFS1bits.C1IF = 0; // Clear interrupt flag

    if (C1INTFbits.TX0IF) {
        C1INTFbits.TX0IF = 0;

    }

    // If a message was received sucessfully
    if (C1INTFbits.RX0IF) {
        C1INTFbits.RX0IF = 0;

        //Move the recieve data from Buffers to InData
        InData0[0] = C1RX0B1; //POSCNT (Master)
        InData0[1] = C1RX0B2; //POSCNT (Slave)
        InData0[2] = C1RX0B3; //ADCValue (Slave) (Currently Not in Use)
        InData0[3] = C1RX0B4; //Sends Confirmation receive flag for Slave

        C1RX0CONbits.RXFUL = 0;
    }
}


//void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void) {
//    IFS0bits.U1TXIF = 0; // Clear U1TX interrupt
//}
//
//void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void) {
//    IFS0bits.U1RXIF = 0; // Clear U1RX interrupt
//}
//


//Timer 1 for outputing Haptic Feedback
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{
    IFS0bits.T1IF = 0; // Clear timer 1 interrupt flag
    volatile float error = 0.0;

      //Motor PWM calculations (Provides Haptic Feedback (Absolute Position Control))
      //CalcPid functino glitch only allows one direction movement for motor
      //Calculates Error (float) and then uses error data to produce PWM

      //Motor moves CW
    if (InData0[1] > POSCNT) {
        error = (float) (InData0[1] - POSCNT);
        PDC1 = (unsigned int) ((1.0)*(error));
        PDC2 = 0;
        LEDGRN = 0;
        LEDRED = 1;
      //Motor moves CCW
    } else if (InData0[1] < POSCNT) {
        error = (float) (POSCNT - InData0[1]);
        LEDGRN = 1;
        LEDRED = 0;
        PDC1 = 0;
        PDC2 = (unsigned int) (1.0)*(error);
      //NO PWM output to motor
    } else {
        PDC1 = 0;
        PDC2 = 0;
        LEDRED = 0;
        LEDGRN = 0;
    }
//            CalcPid(&mypid);

}

//Timer for Sending CAN bus data
void __attribute__((interrupt, no_auto_psv)) _T2Interrupt(void)
{
    IFS0bits.T2IF = 0; // Clear timer 1 interrupt flag
    C1TX0B1 = POSCNT; // Send Master position Data
    C1TX0B4 = 1; // PIC ID for UART (1 = Master, 2 = Slave)
    C1TX0CONbits.TXREQ = 1;
    while (C1TX0CONbits.TXREQ != 0);

}

//Timer 3 for initialization phase
void __attribute__((interrupt, no_auto_psv)) _T3Interrupt(void)
{
    IFS0bits.T3IF = 0; // Clear timer 1 interrupt flag
    CalcPid(&mypid);

}