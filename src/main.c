#include "DSP28x_Project.h"     /// Device Headerfile and Examples Include File
#include <math.h>

///prototypes for interrupt service routines (ISRs) and initialization function
__interrupt void epwm1_isr(void);
__interrupt void epwm2_isr(void);
__interrupt void epwm3_isr(void);
void InitEPwmm(void);

///Define Valid Ranges for parameters
#define PWM_FREQUENCY_MIN 687
#define PWM_FREQUENCY_MAX 10000
#define SIN_FREQUENCY_MIN 0
#define SIN_FREQUENCY_MAX 150
#define MODULATION_DEPTH_MIN 0
#define MODULATION_DEPTH_MAX 100

///User-Defined Parameters
#define PWM_FREQUENCY      5000     ///frequency of pwm DO NOT GO BELOW 687Hz, counter wont work properly 65535 < 90*10^6 / (687*2)
#define SIN_FREQUENCY       60      ///sin frequency 0-150Hz
#define MODULATION_DEPTH    1.0     ///modulation depth between 0 and 1
#define OFFSET              0.0       ///make sure offset is between +-(1-MODULATION_DEPTH)/2
#define ANGLE_1             0.0        ///Phase shift angle in degree
#define ANGLE_2            120.0     ///Phase shift angle in degree
#define ANGLE_3            240.0     ///Phase shift angle in degree


/// PWM period register, no clock divisions
const Uint32 g_epwmTimerTBPRD = (Uint32) (.5 * ((90.0 * 1000000.0) / PWM_FREQUENCY));

void main(void)
{
    /// System initialization
    InitSysCtrl();
    InitEPwm1Gpio();
    InitEPwm2Gpio();
    InitEPwm3Gpio();

    DINT; /// Disable CPU interrupts

    /// Initialize the PIE control registers to their default state
    InitPieCtrl();

    IER = 0x0000; /// Clear all CPU interrupt flags
    IFR = 0x0000; /// Clear all CPU interrupt flags

    /// Initialize the PIE vector table with pointers to the ISR
    InitPieVectTable();

    EALLOW; /// This is needed to write to EALLOW protected registers
    PieVectTable.EPWM1_INT = &epwm1_isr;
    PieVectTable.EPWM2_INT = &epwm2_isr;
    PieVectTable.EPWM3_INT = &epwm3_isr;
    EDIS; /// This is needed to disable write to EALLOW protected registers

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0; /// Disable TBCLK within the ePWM
    EDIS;

    /// Initialize the ePWM modules
    InitEPwmm();

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1; /// Enable TBCLK within the ePWM
    EDIS;

    IER |= M_INT3; /// Enable CPU INT3 which is connected to EPWM1-3 INT

    /// Enable EPWM INTn in the PIE: Group 3 interrupt 1-3
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx2 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx3 = 1;

    EINT;
    /// Enable Global interrupt INTM
    ERTM;
    /// Enable Global realtime interrupt DBGM

    for (;;)
    {
        __asm("NOP"); /// Infinite loop
    }

}

__interrupt void epwm1_isr(void)
{
    ///initialize angle and convert from degrees to radians
    static float angle = ANGLE_1 * M_PI / 180.0;

    // Calculate the angle increment per PWM cycle
    const float angleincrement = 2 * M_PI
            / (float) (PWM_FREQUENCY / SIN_FREQUENCY);

    // If the angle exceeds 2*PI, wrap it around
    if (angle > 2 * M_PI)
        angle = angle - 2 * M_PI;

    // Calculate the duty cycle for the PWM signal
    float duty_cycle = (sinf(angle) * MODULATION_DEPTH + 1) * .5
            - OFFSET;

    // Set the compare value for the PWM signal
    EPwm1Regs.CMPA.half.CMPA = (Uint32) ((duty_cycle)
            * ((float) g_epwmTimerTBPRD));

    // Increment the angle for the next cycle
    angle += angleincrement;

    // Clear the interrupt flag
    EPwm1Regs.ETCLR.bit.INT = 1;

    // Acknowledge the interrupt in the PIE control register
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;

}

//same as epwm1
__interrupt void epwm2_isr(void)
{
    static float angle = ANGLE_2 * M_PI / 180.0;

    const float angleincrement = 2 * M_PI
            / (float) (PWM_FREQUENCY / SIN_FREQUENCY);

    if (angle > 2 * M_PI)
        angle = angle - 2 * M_PI;

    float duty_cycle = (sinf(angle) * MODULATION_DEPTH + 1) * .5
            - OFFSET;

    EPwm2Regs.CMPA.half.CMPA = (Uint32) ((duty_cycle)
            * ((float) g_epwmTimerTBPRD));

    angle += angleincrement;

    EPwm2Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}
//same as epwm1
__interrupt void epwm3_isr(void)
{
    static float angle = ANGLE_3 * M_PI / 180.0;

    const float angleincrement = 2 * M_PI
            / (float) (PWM_FREQUENCY / SIN_FREQUENCY);

    if (angle > 2 * M_PI)
        angle = angle - 2 * M_PI;

    float duty_cycle = (sinf(angle) * MODULATION_DEPTH + 1) * .5
            - OFFSET;

    EPwm3Regs.CMPA.half.CMPA = (Uint32) ((duty_cycle)
            * ((float) g_epwmTimerTBPRD));

    angle += angleincrement;

    EPwm3Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}

void InitEPwmm()
{
    ///setup sync
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;  /// Pass through
    EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;  /// Pass through
    EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;  /// Pass through

    ///allow sync
    EPwm1Regs.TBCTL.bit.PHSEN = TB_ENABLE;
    EPwm2Regs.TBCTL.bit.PHSEN = TB_ENABLE;
    EPwm3Regs.TBCTL.bit.PHSEN = TB_ENABLE;

    ///sets the phase angle for each wave
    EPwm1Regs.TBPHS.half.TBPHS =
            (Uint16) (ANGLE_1 / (360) * (g_epwmTimerTBPRD));
    EPwm2Regs.TBPHS.half.TBPHS =
            (Uint16) (ANGLE_2 / (360) * (g_epwmTimerTBPRD));
    EPwm3Regs.TBPHS.half.TBPHS =
            (Uint16) (ANGLE_3 / (360) * (g_epwmTimerTBPRD));

    EPwm1Regs.TBPRD = g_epwmTimerTBPRD;         /// Set timer period
    EPwm2Regs.TBPRD = g_epwmTimerTBPRD;         /// Set timer period
    EPwm3Regs.TBPRD = g_epwmTimerTBPRD;         /// Set timer period

    EPwm1Regs.TBCTR = 0x0000;                   /// Clear counter
    EPwm2Regs.TBCTR = 0x0000;                   /// Clear counter
    EPwm3Regs.TBCTR = 0x0000;                   /// Clear counter

    EPwm1Regs.CMPA.half.CMPA = 0;               /// starts compare A value
    EPwm2Regs.CMPA.half.CMPA = 0;               /// starts compare A value
    EPwm3Regs.CMPA.half.CMPA = 0;               /// starts compare A value

    EPwm1Regs.TBCTL.bit.CTRMODE = 2;            /// Count up/down
    EPwm2Regs.TBCTL.bit.CTRMODE = 2;            /// Count up/down
    EPwm3Regs.TBCTL.bit.CTRMODE = 2;            /// Count up/down

    EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;  /// Interrupt on when counter = 0
    EPwm2Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;  /// Interrupt on when counter = 0
    EPwm3Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;  /// Interrupt on when counter = 0

    EPwm1Regs.ETSEL.bit.INTEN = 1;              /// Enable INT
    EPwm2Regs.ETSEL.bit.INTEN = 1;              /// Enable INT
    EPwm3Regs.ETSEL.bit.INTEN = 1;              /// Enable INT

    EPwm1Regs.ETPS.bit.INTPRD = 1;              /// Generate INT on 1st event
    EPwm2Regs.ETPS.bit.INTPRD = 1;              /// Generate INT on 1st event
    EPwm3Regs.ETPS.bit.INTPRD = 1;              /// Generate INT on 1st event

    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;    /// Clock ratio to SYSCLKOUT
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;    /// Clock ratio to SYSCLKOUT
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;     /// Clock ratio to SYSCLKOUT

    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;           /// Clock ratio to SYSCLKOUT
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;           /// Clock ratio to SYSCLKOUT
    EPwm3Regs.TBCTL.bit.CLKDIV = TB_DIV1;           /// Clock ratio to SYSCLKOUT

    ///shaddow
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;// Enable shadow mode for Compare A registers of ePWM1
    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;// Enable shadow mode for Compare A registers of ePWM2
    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;// Enable shadow mode for Compare A registers of ePWM3

    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;  /// Load on Zero
    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;  /// Load on Zero
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;  /// Load on Zero

    EPwm1Regs.AQCTLA.bit.CAU = 2;    /// Set PWM1A on event A, up count
    EPwm2Regs.AQCTLA.bit.CAU = 2;    /// Set PWM1A on event A, up count
    EPwm3Regs.AQCTLA.bit.CAU = 2;    /// Set PWM1A on event A, up count

    EPwm1Regs.AQCTLA.bit.CAD = 1;   /// Clear PWM1A on event A, down count
    EPwm2Regs.AQCTLA.bit.CAD = 1;   /// Clear PWM1A on event A, down count
    EPwm3Regs.AQCTLA.bit.CAD = 1;   /// Clear PWM1A on event A, down count

}

