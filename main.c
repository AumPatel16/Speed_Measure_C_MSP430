
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

#include <msp430.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned int CaptureFlag = 0;
volatile unsigned long capDelta = 0;
volatile unsigned long Delta = 0;
volatile unsigned long speed = 0;
volatile unsigned int sens1cap = 0;
volatile unsigned int safety = 0;
volatile unsigned int unitChange = 0;
volatile unsigned int convert = 0;



char strSpeed[10];
volatile unsigned int sens2cap = 0;
//unsigned int timerAcapturePointer = 0;

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

#include "main.h"
#include "hal_LCD.h"
#include "StopWatchMode.h"
#include "TempSensorMode.h"

// Backup Memory variables to track states through LPM3.5
//volatile unsigned char * S1buttonDebounce = &BAKMEM2_L;       // S1 button debounce flag
//volatile unsigned char * S2buttonDebounce = &BAKMEM2_H;       // S2 button debounce flag
volatile unsigned char * stopWatchRunning = &BAKMEM3_L;       // Stopwatch running flag
volatile unsigned char * tempSensorRunning = &BAKMEM3_H;      // Temp Sensor running flag
volatile unsigned char * mode = &BAKMEM4_L;                   // mode flag
volatile unsigned int holdCount = 0;

// TimerA0 UpMode Configuration Parameter
Timer_A_initUpModeParam initUpParam_A0 =
{
		TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
		TIMER_A_CLOCKSOURCE_DIVIDER_1,          // SMCLK/1 = 2MHz
		30000,                                  // 15ms debounce period
		TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
		TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
		TIMER_A_DO_CLEAR,                       // Clear value
		true                                    // Start Timer
};

/*
 * main.c
 */
int main(void) {
    // Stop Watchdog timer
    WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);     // Stop WDT

    // Check if a wakeup from LPMx.5
    if (SYSRSTIV == SYSRSTIV_LPM5WU)
    {
        Init_GPIO();
        __enable_interrupt();

        switch(*mode)
        {
            case STOPWATCH_MODE:
                stopWatch();
                break;
            case TEMPSENSOR_MODE:
                tempSensor();
                break;
        }
    }
    else
    {
        // Initializations
        Init_GPIO();
        Init_Clock();
        Init_RTC();
        Init_LCD();

        GPIO_clearInterrupt(GPIO_PORT_P1, GPIO_PIN2);
        GPIO_clearInterrupt(GPIO_PORT_P2, GPIO_PIN6);

       // *S1buttonDebounce = *S2buttonDebounce = *stopWatchRunning = *tempSensorRunning = *mode = 0;

        __enable_interrupt();

        displayScrollText("EMTS");
    }

    int i = 0x01;
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    /*// Configure GPIO
        P1DIR |= BIT0;                                  // Set P1.0 as output
        P1OUT |= BIT0;                                  // P1.0 high*/
        P1REN |= BIT6 | BIT7;                                  // enable internal pull-down resistor
        P1OUT |= BIT6 | BIT7;

        P8SEL0 |= BIT1;                                 // Set as ACLK pin, second function
        P8DIR |= BIT1;

        // Disable the GPIO power-on default high-impedance mode to activate
        // previously configured port settings
        PM5CTL0 &= ~LOCKLPM5;

        // Configure clock
        FRCTL0 = FRCTLPW | NWAITS0;                     // Set number of FRAM waitstates to 0
        __bis_SR_register(SCG0);                        // disable FLL
        CSCTL3 |= SELREF__REFOCLK;                      // Set REFO as FLL reference source
        CSCTL0 = 0;                                     // clear DCO and MOD registers
        CSCTL1 &= ~(DCORSEL_7);                         // Clear DCO frequency select bits first
        CSCTL1 |= DCORSEL_2;                            // Set DCO = 4MHz
        CSCTL2 = FLLD_1 + 60;                           // DCODIV = 2MHz
        __delay_cycles(3);
        __bic_SR_register(SCG0);                        // enable FLL
        while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));      // Poll until FLL is locked

        CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;      // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
                                                        // default DCODIV as MCLK and SMCLK source
        CSCTL5 |= DIVM__1 | DIVS__2;                    // SMCLK = 1MHz, MCLK = 2MHz

        // Timer0_A3 Setup

        //capture 1   P1.7   TA0.1
        //capture 2   P1.6   TA0.2

        P1SEL0 |= BIT6 | BIT7   ;                                 // Set as ACLK pin, second function


        TA0CCTL1 |= CM_1 | CCIS_0 |       CAP;   // P1.7
        TA0CCTL2 |= CM_1 | CCIS_0 | CCIE | CAP;   // P1.6
                                                        // Capture rising edge,
                                                        // Use CCI2A=ACLK,
                                                        // SCS     Synchronous capture,
                                                        // Enable capture mode,
                                                        // Enable capture interrupt

        TA0CTL |= TASSEL__ACLK | MC_2 | TACLR;              // Use ACLK as clock source, clear TA0R





        // Start timer in continuous mode

        __bis_SR_register( GIE);
        __no_operation();

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&


   while(1)
    {
        LCD_E_selectDisplayMemory(LCD_E_BASE, LCD_E_DISPLAYSOURCE_MEMORY);

        switch(*mode)
        {
            case STARTUP_MODE:

            if (CaptureFlag & safety)
                       {
                       CaptureFlag = 0;
                       safety = 0;
                       //Measured ACLK = 32894 HZ, or 30.4 Microseconds per cycle.
                       //use nano seconds for precision
                       sens1cap = TA0CCR1;
                       sens2cap = TA0CCR2;

                       Delta = (sens2cap-sens1cap);
                       capDelta = Delta*30400;//time difference in ns.
                       //distance is 73420000 nm
                       speed = 73420000000/capDelta;
                       if(unitChange == 1){
                       speed /= 25;//add 3 zeros to distance in nm.
                       }
                       clearLCD();
                       showChar((speed) % 10 + '0',pos6);
                       speed /= 10;
                       showChar((speed) % 10 + '0',pos5);
                       speed /= 10;
                       showChar((speed) % 10 + '0',pos4);
                       showChar(' ',pos3);
                       speed /= 10;
                       showChar((speed) % 10 + '0',pos2);
                       //showChar((speed) % 10 + '0',pos1);
                       //TACLR;
                       }else if(convert){
                          convert = 0;
                          speed = 73420000000/capDelta;
                          if(unitChange == 1){
                          speed /= 25;//add 3 zeros to distance in nm.
                          }
                          clearLCD();
                          showChar((speed) % 10 + '0',pos6);
                          speed /= 10;
                          showChar((speed) % 10 + '0',pos5);
                          speed /= 10;
                          showChar((speed) % 10 + '0',pos4);
                          showChar(' ',pos3);
                          speed /= 10;
                          showChar((speed) % 10 + '0',pos2);
                       }


                break;
            case STOPWATCH_MODE:         // Stopwatch Timer mode
                break;
            case TEMPSENSOR_MODE:        // Temperature Sensor mode
                break;
        }
   //     __bis_SR_register(LPM3_bits | GIE);         // enter LPM3
        __no_operation();
    }
}

/*
 * GPIO Initialization
 */
void Init_GPIO()
{
    // Set all GPIO pins to output low to prevent floating input and reduce power consumption
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN1);

    // Configure button S1 (P1.2) interrupt
    GPIO_selectInterruptEdge(GPIO_PORT_P1, GPIO_PIN2, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN2);
    GPIO_clearInterrupt(GPIO_PORT_P1, GPIO_PIN2);
    GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN2);

    // Configure button S2 (P2.6) interrupt
    GPIO_selectInterruptEdge(GPIO_PORT_P2, GPIO_PIN6, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P2, GPIO_PIN6);
    GPIO_clearInterrupt(GPIO_PORT_P2, GPIO_PIN6);
    GPIO_enableInterrupt(GPIO_PORT_P2, GPIO_PIN6);

    // Set P4.1 and P4.2 as Secondary Module Function Input, LFXT.
    GPIO_setAsPeripheralModuleFunctionInputPin(
           GPIO_PORT_P4,
           GPIO_PIN1 + GPIO_PIN2,
           GPIO_PRIMARY_MODULE_FUNCTION
           );

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();
}

/*
 * Clock System Initialization
 */
void Init_Clock()
{
    // Intializes the XT1 crystal oscillator
    CS_turnOnXT1(CS_XT1_DRIVE_1);
}

/*
 * Real Time Clock counter Initialization
 */
void Init_RTC()
{
    // Set RTC modulo to 327-1 to trigger interrupt every ~10 ms
    RTC_setModulo(RTC_BASE, 326);
    RTC_enableInterrupt(RTC_BASE, RTC_OVERFLOW_INTERRUPT);
}

/*
 * PORT1 Interrupt Service Routine
 * Handles S1 button press interrupt
 */
#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    P1OUT |= BIT0;    // Turn LED1 On
    safety = 1;

    /*showChar('R',pos1);
    showChar('E',pos1);
    showChar('A',pos1);
    showChar('D',pos1);*/
    clearLCD();
    showChar('R',pos1);
    showChar('E',pos2);
    showChar('A',pos3);
    showChar('D',pos4);
    showChar('Y',pos5);

    P1IFG &= ~0x04; // Clear interrupt flag for P1.2
}



// * PORT2 Interrupt Service Routine
// * Handles S2 button press interrupt

#pragma vector = PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
    P4OUT |= BIT0;    // Turn LED2 On
    if (unitChange == 0){
        unitChange = 1;
        clearLCD();
        showChar('I',pos1);
        showChar('N',pos2);
        showChar('C',pos3);
        showChar('H',pos4);
        showChar('E',pos5);
        showChar('S',pos6);
        __delay_cycles(1000000);
        convert = 1;
    }else{
        unitChange = 0;
        clearLCD();
        showChar('M',pos1);
        showChar('E',pos2);
        showChar('T',pos3);
        showChar('E',pos4);
        showChar('R',pos5);
        showChar('S',pos6);
        __delay_cycles(1000000);
        convert = 1;

    }

    P2IFG &= ~BIT6; // P2.6 IFG cleared
}


/*
 * ADC Interrupt Service Routine
 * Wake up from LPM3 when ADC conversion completes

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC_VECTOR))) ADC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
    {
        case ADCIV_NONE:
            break;
        case ADCIV_ADCOVIFG:
            break;
        case ADCIV_ADCTOVIFG:
            break;
        case ADCIV_ADCHIIFG:
            break;
        case ADCIV_ADCLOIFG:
            break;
        case ADCIV_ADCINIFG:
            break;
        case ADCIV_ADCIFG:
            // Clear interrupt flag
            ADC_clearInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT_FLAG);
            __bic_SR_register_on_exit(LPM3_bits);                // Exit LPM3
            break;
        default:
            break;
    }
}
*/
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
volatile int CaptureCount = 0;

/*
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
    __no_operation();
}
*/


 //Timer0_A3 CC1-2, TA Interrupt Handler
#pragma vector = TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void)
{
    switch(__even_in_range(TA0IV,TA0IV_TAIFG))
    {
        case TA0IV_NONE:
            break;                                  // No interrupt
        case TA0IV_TACCR1:
            CaptureCount++; //CCR1 is low
            //sens1cap = TA0CCR1;
            break;                                  // CCR1 not used
        case TA0IV_TACCR2:
            CaptureCount++; //CCR2 is low
            //sens2cap = TA0CCR2;
            CaptureFlag = 1;
            break;                                  // CCR2 not used
        case TA0IV_3:
            break;                                  // reserved
        case TA0IV_4:
            break;                                  // reserved
        case TA0IV_5:
            break;                                  // reserved
        case TA0IV_6:
            break;                                  // reserved
        case TA0IV_TAIFG:
            break;                                  // overflow
        default:
            break;
    }
    __no_operation();
}


//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
