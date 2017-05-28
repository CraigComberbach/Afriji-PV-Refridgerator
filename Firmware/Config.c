/**************************************************************************************************
Authours:				Craig Comberbach
Target Hardware:		PIC
Chip resources used:
Code assumptions:
Purpose:

Version History:
v0.0.0	2013-07-11  Craig Comberbach
	Compiler: C30 v3.31		IDE: MPLABx 1.80	Tool: RealICE	Computer: Intel Xeon CPU 3.07 GHz, 6 GB RAM, Windows 7 64 bit Professional SP1
	First version
**************************************************************************************************/
/*************    Header Files    ***************/
#include "Config.h"
#include "Pins.h"
#include "A2D.h"
#include "Scheduler.h"
#include "Analogs.h"
#include "Inverter.h"
#include "Debug.h"

/*************Semantic  Versioning***************/
/*************Library Dependencies***************/
#ifndef PINS_LIBRARY
	#error "You need to include the Pins library for this code to compile"
#endif

#ifndef A2D_LIBRARY
	#error "You need to include the A2D library for this code to compile"
#endif

#ifndef INVERTER_LIBRARY
	#error "You need to include the Inverter library for this code to compile"
#endif

#ifndef SCHEDULER_LIBRARY
	#error "You need to include the Scheduler library for this code to compile"
#endif

/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
/***********State Machine Definitions*************/
/*************  Global Variables  ***************/
volatile int dummy;
const char compiledOnDate[] = __DATE__;
const char compiledAtTime[] = __TIME__;

/*************Function  Prototypes***************/
void __attribute__((interrupt, auto_psv)) _OscillatorFail(void);
void __attribute__((interrupt, auto_psv)) _AddressError(void);
void __attribute__((interrupt, auto_psv)) _StackError(void);
void __attribute__((interrupt, auto_psv)) _MathError(void);
void __attribute__((interrupt, auto_psv)) _DefaultInterrupt(void);

/************* Device Definitions ***************/
/************* Module Definitions ***************/
/************* Other  Definitions ***************/

void Configure_For_Afriji(void)
{
	/*************       Clock        ***************/
	CLKDIVbits.RCDIV	= 0;
	OSCTUNbits.TUN		= 0b000001; //+0.77%

	/*************        Pins        ***************/
	//PORTB
	Pin_Definition(PIN_RB0_TRANSFORMER_PRIMARY_MINUS,	Rx0, &TRISB, &ODCB, &LATB, &PORTB);	//RB0
	Pin_Definition(PIN_RB1_TRANSFORMER_PRIMARY_PLUS,	Rx1, &TRISB, &ODCB, &LATB, &PORTB);	//RB1
	Pin_Definition(PIN_RB2_VSOLAR_PLUS,					Rx2, &TRISB, &ODCB, &LATB, &PORTB);	//RB2
	Pin_Definition(PIN_RB3_TEMP2,						Rx3, &TRISB, &ODCB, &LATB, &PORTB);	//RB3
//	Pin_Definition(PIN_RB4_PGD,							Rx4, &TRISB, &ODCB, &LATB, &PORTB);	//RB4
//	Pin_Definition(PIN_RB5_PGC,							Rx5, &TRISB, &ODCB, &LATB, &PORTB);	//RB5
	Pin_Definition(PIN_RB6_TEMP1,						Rx6, &TRISB, &ODCB, &LATB, &PORTB);	//RB6
	Pin_Definition(PIN_RB7_TEMP3,						Rx7, &TRISB, &ODCB, &LATB, &PORTB);	//RB7
	Pin_Definition(PIN_RB8_TEMP4,						Rx8, &TRISB, &ODCB, &LATB, &PORTB);	//RB8
	Pin_Definition(PIN_RB9_INPUT_CURRENT,				Rx9, &TRISB, &ODCB, &LATB, &PORTB);	//RB9
	Pin_Definition(PIN_RB10_OUTPUT_CURRENT,				Rx10, &TRISB, &ODCB, &LATB, &PORTB);//RB10
	Pin_Definition(PIN_RB11_TEMP5,						Rx11, &TRISB, &ODCB, &LATB, &PORTB);//RB11
	Pin_Definition(PIN_RB12_VDC_BUS_PLUS,				Rx12, &TRISB, &ODCB, &LATB, &PORTB);//RB12
	Pin_Definition(PIN_RB13_TRANSFORMER_SECONDARY_PLUS,	Rx13, &TRISB, &ODCB, &LATB, &PORTB);//RB13
	Pin_Definition(PIN_RB14_VOUT_PLUS,					Rx14, &TRISB, &ODCB, &LATB, &PORTB);//RB14
	Pin_Definition(PIN_RB15_VOUT_MINUS,					Rx15, &TRISB, &ODCB, &LATB, &PORTB);//RB15
	//PORTC
	Pin_Definition(PIN_RC12,							Rx12, &TRISC, &ODCC, &LATC, &PORTC);//RC12
	Pin_Definition(PIN_RC13,							Rx13, &TRISC, &ODCC, &LATC, &PORTC);//RC13
	Pin_Definition(PIN_RC14,							Rx14, &TRISC, &ODCC, &LATC, &PORTC);//RC14
	Pin_Definition(PIN_RC15,							Rx15, &TRISC, &ODCC, &LATC, &PORTC);//RC15
	//PORTD
	Pin_Definition(PIN_RD0_HOB_HiA,						Rx0, &TRISD, &ODCD, &LATD, &PORTD);	//RD0
	Pin_Definition(PIN_RD1_TERMINAL_TX,					Rx1, &TRISD, &ODCD, &LATD, &PORTD);	//RD1
	Pin_Definition(PIN_RD2,								Rx2, &TRISD, &ODCD, &LATD, &PORTD);	//RD2
	Pin_Definition(PIN_RD3,								Rx3, &TRISD, &ODCD, &LATD, &PORTD);	//RD3
	Pin_Definition(PIN_RD4_LOB_HiA,						Rx4, &TRISD, &ODCD, &LATD, &PORTD);	//RD4
	Pin_Definition(PIN_RD5_LOA_HiA,						Rx5, &TRISD, &ODCD, &LATD, &PORTD);	//RD5
	Pin_Definition(PIN_RD6,								Rx6, &TRISD, &ODCD, &LATD, &PORTD);	//RD6
	Pin_Definition(PIN_RD7,								Rx7, &TRISD, &ODCD, &LATD, &PORTD);	//RD7
	Pin_Definition(PIN_RD8_HOA_HiA,						Rx8, &TRISD, &ODCD, &LATD, &PORTD);	//RD8
	Pin_Definition(PIN_RD9_GREEN_LED,					Rx9, &TRISD, &ODCD, &LATD, &PORTD);	//RD9
	Pin_Definition(PIN_RD10_RED_LED,					Rx10, &TRISD, &ODCD, &LATD, &PORTD);//RD10
	Pin_Definition(PIN_RD11_BLUE_LED,					Rx11, &TRISD, &ODCD, &LATD, &PORTD);//RD11
	//PORTE
	Pin_Definition(PIN_RE0,								Rx0, &TRISE, &ODCE, &LATE, &PORTE);	//RE0
	Pin_Definition(PIN_RE1,								Rx1, &TRISE, &ODCE, &LATE, &PORTE);	//RE1
	Pin_Definition(PIN_RE2,								Rx2, &TRISE, &ODCE, &LATE, &PORTE);	//RE2
	Pin_Definition(PIN_RE3,								Rx3, &TRISE, &ODCE, &LATE, &PORTE);	//RE3
	Pin_Definition(PIN_RE4,								Rx4, &TRISE, &ODCE, &LATE, &PORTE);	//RE4
	Pin_Definition(PIN_RE5_SWITCHED_GROUND5,			Rx5, &TRISE, &ODCE, &LATE, &PORTE);	//RE5
	Pin_Definition(PIN_RE6_SWITCHED_GROUND4,			Rx6, &TRISE, &ODCE, &LATE, &PORTE);	//RE6
	Pin_Definition(PIN_RE7_SWITCHED_GROUND3,			Rx7, &TRISE, &ODCE, &LATE, &PORTE);	//RE7
	//PORTF
	Pin_Definition(PIN_RF0,								Rx0, &TRISF, &ODCF, &LATF, &PORTF);	//RF0
	Pin_Definition(PIN_RF1,								Rx1, &TRISF, &ODCF, &LATF, &PORTF);	//RF1
	Pin_Definition(PIN_RF2_HOB_HiV,						Rx2, &TRISF, &ODCF, &LATF, &PORTF);	//RF2
	Pin_Definition(PIN_RF3_HOA_HiV,						Rx3, &TRISF, &ODCF, &LATF, &PORTF);	//RF3
	Pin_Definition(PIN_RF4_LOA_HiV,						Rx4, &TRISF, &ODCF, &LATF, &PORTF);	//RF4
	Pin_Definition(PIN_RF5_LOB_HiV,						Rx5, &TRISF, &ODCF, &LATF, &PORTF);	//RF5
	Pin_Definition(PIN_RF6,								Rx6, &TRISF, &ODCF, &LATF, &PORTF);	//RF6
	//PORTG
	Pin_Definition(PIN_RG2,								Rx2, &TRISG, &ODCG, &LATG, &PORTG);	//RG2
	Pin_Definition(PIN_RG3,								Rx3, &TRISG, &ODCG, &LATG, &PORTG);	//RG3
	Pin_Definition(PIN_RG6_SWITCHED_GROUND1,			Rx6, &TRISG, &ODCG, &LATG, &PORTG);	//RG6
	Pin_Definition(PIN_RG7_SWITCHED_GROUND2,			Rx7, &TRISG, &ODCG, &LATG, &PORTG);	//RG7
	Pin_Definition(PIN_RG8,								Rx8, &TRISG, &ODCG, &LATG, &PORTG);	//RG8
	Pin_Definition(PIN_RG9,								Rx9, &TRISG, &ODCG, &LATG, &PORTG);	//RG9

	//PORTB
	Pin_Initialize(PIN_RB0_TRANSFORMER_PRIMARY_MINUS,	LOW, PUSH_PULL, INPUT);	//RB0
	Pin_Initialize(PIN_RB1_TRANSFORMER_PRIMARY_PLUS,	LOW, PUSH_PULL, INPUT);	//RB1
	Pin_Initialize(PIN_RB2_VSOLAR_PLUS,					LOW, PUSH_PULL, INPUT);	//RB2
	Pin_Initialize(PIN_RB3_TEMP2,						LOW, PUSH_PULL, INPUT);	//RB3
//	Pin_Initialize(PIN_RB4_PGD,							LOW, PUSH_PULL, INPUT);	//RB4
//	Pin_Initialize(PIN_RB5_PGC,							LOW, PUSH_PULL, INPUT);	//RB5
	Pin_Initialize(PIN_RB6_TEMP1,						LOW, PUSH_PULL, INPUT);	//RB6
	Pin_Initialize(PIN_RB7_TEMP3,						LOW, PUSH_PULL, INPUT);	//RB7
	Pin_Initialize(PIN_RB8_TEMP4,						LOW, PUSH_PULL, INPUT);	//RB8
	Pin_Initialize(PIN_RB9_INPUT_CURRENT,				LOW, PUSH_PULL, INPUT);	//RB9
	Pin_Initialize(PIN_RB10_OUTPUT_CURRENT,				LOW, PUSH_PULL, INPUT);	//RB10
	Pin_Initialize(PIN_RB11_TEMP5,						LOW, PUSH_PULL, INPUT);	//RB11
	Pin_Initialize(PIN_RB12_VDC_BUS_PLUS,				LOW, PUSH_PULL, INPUT);	//RB12
	Pin_Initialize(PIN_RB13_TRANSFORMER_SECONDARY_PLUS,	LOW, PUSH_PULL, INPUT);	//RB13
	Pin_Initialize(PIN_RB14_VOUT_PLUS,					LOW, PUSH_PULL, INPUT);	//RB14
	Pin_Initialize(PIN_RB15_VOUT_MINUS,					LOW, PUSH_PULL, INPUT);	//RB15
	//PORTC
	Pin_Initialize(PIN_RC12,							LOW, PUSH_PULL, INPUT);	//RC12
	Pin_Initialize(PIN_RC13,							LOW, PUSH_PULL, INPUT);	//RC13
	Pin_Initialize(PIN_RC14,							LOW, PUSH_PULL, INPUT);	//RC14
	Pin_Initialize(PIN_RC15,							LOW, PUSH_PULL, INPUT);	//RC15
	//PORTD
	Pin_Initialize(PIN_RD0_HOB_HiA,						LOW, PUSH_PULL, OUTPUT);//RD0
	Pin_Initialize(PIN_RD1_TERMINAL_TX,					LOW, PUSH_PULL, INPUT);	//RD1
	Pin_Initialize(PIN_RD2,								LOW, PUSH_PULL, INPUT);	//RD2
	Pin_Initialize(PIN_RD3,								LOW, PUSH_PULL, INPUT);	//RD3
	Pin_Initialize(PIN_RD4_LOB_HiA,						LOW, PUSH_PULL, OUTPUT);//RD4
	Pin_Initialize(PIN_RD5_LOA_HiA,						LOW, PUSH_PULL, OUTPUT);//RD5
	Pin_Initialize(PIN_RD6,								LOW, PUSH_PULL, INPUT);	//RD6
	Pin_Initialize(PIN_RD7,								LOW, PUSH_PULL, INPUT);	//RD7
	Pin_Initialize(PIN_RD8_HOA_HiA,						LOW, PUSH_PULL, OUTPUT);//RD8
	Pin_Initialize(PIN_RD9_GREEN_LED,					LOW, PUSH_PULL, OUTPUT);//RD9
	Pin_Initialize(PIN_RD10_RED_LED,					LOW, PUSH_PULL, INPUT);	//RD10
	Pin_Initialize(PIN_RD11_BLUE_LED,					LOW, PUSH_PULL, INPUT);	//RD11
	//PORTE
	Pin_Initialize(PIN_RE0,								LOW, PUSH_PULL, INPUT);	//RE0
	Pin_Initialize(PIN_RE1,								LOW, PUSH_PULL, INPUT);	//RE1
	Pin_Initialize(PIN_RE2,								LOW, PUSH_PULL, INPUT);	//RE2
	Pin_Initialize(PIN_RE3,								LOW, PUSH_PULL, INPUT);	//RE3
	Pin_Initialize(PIN_RE4,								LOW, PUSH_PULL, INPUT);	//RE4
	Pin_Initialize(PIN_RE5_SWITCHED_GROUND5,			LOW, OPEN_DRAIN, OUTPUT);//RE5
	Pin_Initialize(PIN_RE6_SWITCHED_GROUND4,			LOW, OPEN_DRAIN, OUTPUT);//RE6
	Pin_Initialize(PIN_RE7_SWITCHED_GROUND3,			LOW, OPEN_DRAIN, OUTPUT);//RE7
	//PORTF
	Pin_Initialize(PIN_RF0,								LOW, PUSH_PULL, INPUT);	//RF0
	Pin_Initialize(PIN_RF1,								LOW, PUSH_PULL, INPUT);	//RF1
	Pin_Initialize(PIN_RF2_HOB_HiV,						LOW, PUSH_PULL, OUTPUT);//RF2
	Pin_Initialize(PIN_RF3_HOA_HiV,						LOW, PUSH_PULL, OUTPUT);//RF3
	Pin_Initialize(PIN_RF4_LOA_HiV,						LOW, PUSH_PULL, OUTPUT);//RF4
	Pin_Initialize(PIN_RF5_LOB_HiV,						LOW, PUSH_PULL, OUTPUT);//RF5
	Pin_Initialize(PIN_RF6,								LOW, PUSH_PULL, INPUT);	//RF6
	//PORTG
	Pin_Initialize(PIN_RG2,								LOW, PUSH_PULL, INPUT);	//RG2
	Pin_Initialize(PIN_RG3,								LOW, PUSH_PULL, INPUT);	//RG3
	Pin_Initialize(PIN_RG6_SWITCHED_GROUND1,			LOW, OPEN_DRAIN, OUTPUT);//RG6
	Pin_Initialize(PIN_RG7_SWITCHED_GROUND2,			LOW, OPEN_DRAIN, OUTPUT);//RG7
	Pin_Initialize(PIN_RG8,								LOW, PUSH_PULL, INPUT);	//RG8
	Pin_Initialize(PIN_RG9,								LOW, PUSH_PULL, INPUT);	//RG9

	/************* PeripheralPinSelect***************/

	//Input Inverter (Hi-I Lo-V)
	#ifdef HiI_INVERTER_ENABLED
	RPOR10bits.RP20R	= 18;	//OC1	LOA
	RPOR1bits.RP2R		= 19;	//OC2	HOA
	RPOR5bits.RP11R		= 20;	//OC3	HOB
	RPOR12bits.RP25R	= 21;	//OC4	LOB
	#endif

	//Output Inverter (Hi-V Lo-A)
	#ifdef HiV_INVERTER_ENABLED
	RPOR5bits.RP10R		= 23;	//OC6	LOA
	RPOR8bits.RP16R		= 24;	//OC7	HOA
	RPOR15bits.RP30R	= 25;	//OC8	HOB
	RPOR8bits.RP17R		= 35;	//OC9	LOB
	#endif

	//LED indicators
	RPOR1bits.RP3R		= 23;	//Red LED (Green on Schematic)
	RPOR6bits.RP12R		= 18;	//Blue LED (Blue on schematic)

	//Feedback UART
	RPOR12bits.RP24R 	= 3;	//UART1 - Terminal Tx 

	__builtin_write_OSCCONL(OSCCON | 0x40);

	/*************  Terminal Window   ***************/
	Debug_Initialize();
	
	/*************        A2D         ***************/
	A2D_Initialize();
	A2D_Channel_Settings(A2D_AN0_TRANSFORMER_PRIMARY_MINUS,		RESOLUTION_10_BIT,	1,	&LoV_Formating_AN0);
	A2D_Channel_Settings(A2D_AN1_TRANSFORMER_PRIMARY_PLUS,		RESOLUTION_10_BIT,	1,	&LoV_Formating_AN1);
	A2D_Channel_Settings(A2D_AN2_SOLAR_PLUS,					RESOLUTION_10_BIT,	1,	&LoV_Formating_AN2);
	A2D_Channel_Settings(A2D_AN3_TEMP2,							RESOLUTION_10_BIT,	1,	&Afriji_Celcius_Formating);
//	A2D_Channel_Settings(A2D_AN4_UNUSED,						RESOLUTION_10_BIT,	1,	&);//Unused
//	A2D_Channel_Settings(A2D_AN5_UNUSED,						RESOLUTION_10_BIT,	1,	&);//Unused
	A2D_Channel_Settings(A2D_AN6_TEMP1,							RESOLUTION_10_BIT,	1,	&Afriji_Celcius_Formating);
	A2D_Channel_Settings(A2D_AN7_TEMP3,							RESOLUTION_10_BIT,	1,	&Afriji_Celcius_Formating);
	A2D_Channel_Settings(A2D_AN8_TEMP4,							RESOLUTION_10_BIT,	1,	&Afriji_Celcius_Formating);
	A2D_Channel_Settings(A2D_AN9_INPUT_CURRENT,					RESOLUTION_10_BIT,	1,	&HiI_Formating);
	A2D_Channel_Settings(A2D_AN10_OUTPUT_CURRENT,				RESOLUTION_10_BIT,	1,	&LoI_Formating);
	A2D_Channel_Settings(A2D_AN11_TEMP5,						RESOLUTION_10_BIT,	1,	&Afriji_Celcius_Formating);
	A2D_Channel_Settings(A2D_AN12_VDC_BUS_PLUS,					RESOLUTION_10_BIT,	1,	&HiV_Formating_AN12);
	A2D_Channel_Settings(A2D_AN13_TRANSFORMER_SECONDARY_PLUS,	RESOLUTION_10_BIT,	1,	&HiV_Formating_AN13);
	A2D_Channel_Settings(A2D_AN14_VOUT_PLUS,					RESOLUTION_10_BIT,	1,	&HiV_Formating_AN14);
	A2D_Channel_Settings(A2D_AN15_VOUT_MINUS,					RESOLUTION_10_BIT,	1,	&HiV_Formating_AN15);

	/*************         I2C        ***************/
	/*************       Timers       ***************/
	T2CONbits.TCS	= 0;	//0= Internal clock (FOSC/2)
	T2CONbits.T32	= 0;	//Timerx and Timery act as two 16-bit timers
	T2CONbits.TCKPS	= 0b01;	//01= 1:8
	T2CONbits.TGATE	= 0;	//0= Gated time accumulation disabled
	T2CONbits.TSIDL	= 0;	//0= Continue module operation in Idle mode
	T2CONbits.TON	= 1;	//1= Starts 32-bit Timerx/y
	
	/************* OutputCompareModule***************/
	/*************      H-Bridge      ***************/
	Initialize_Inverter();

	/*************       Other        ***************/

	return;
}

void Heart_Beat_Task(unsigned long time_mS)
{
	Pin_Toggle(PIN_RD9_GREEN_LED);
	return;
}

//Error Traps
//Oscillator Failed
void __attribute__((interrupt, auto_psv)) _OscillatorFail(void)
{
	INTCON1bits.OSCFAIL = 0;
}

//Address Out of range
void __attribute__((interrupt, auto_psv)) _AddressError(void)
{
	INTCON1bits.ADDRERR = 0;
	dummy = 1;
	while(1)
		asm("clrwdt");
//	Reset();
}

//Stack Overflow
void __attribute__((interrupt, auto_psv)) _StackError(void)
{
	INTCON1bits.STKERR = 0;
	dummy = 2;
	while(1)
		asm("clrwdt");
//	Reset();
}

//Math error, stupid divide by zero!
void __attribute__((interrupt, auto_psv)) _MathError(void)
{
	INTCON1bits.MATHERR = 0;
	dummy = 3;
	while(1)
		asm("clrwdt");
//	Reset();
}

//Something has an unhandled interrupt!
void __attribute__((interrupt, auto_psv)) _DefaultInterrupt(void)
{
	Nop();
	dummy = 4;
	return;
}
