/**************************************************************************************************
Target Hardware:		PIC24FJ256GA106
Code assumptions:		
Purpose:				
Notes:					

Version History:
vnext	Y-M-D	Craig Comberbach
Compiler: XC16 v1.26	IDE: MPLABx 3.30	Tool: ICD3	Computer: Intel Core2 Quad CPU 2.40 GHz, 5 GB RAM, Windows 10 64 bit Home
	First version
**************************************************************************************************/
/*************    Header Files    ***************/
#include "Config.h"
#include "Variable Transformer.h"

/************* Library Definition ***************/
/************* Semantic Versioning***************/
#if VARIABLE_TRANSFORMER_MAJOR != 0
	#error "Variable Transformer has had a change that loses some previously supported functionality"
#elif VARIABLE_TRANSFORMER_MINOR != 1
	#error "Variable Transformer has new features that your code may benefit from"
#elif VARIABLE_TRANSFORMER_PATCH != 0
	#error "Variable Transformer has had a bug fix, you should check to see that you weren't relying on a bug for functionality"
#endif

/*************Library Dependencies***************/
/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
#define	PERIOD	1599
/*************    Enumeration     ***************/
/***********  Structure Definitions  ************/
/***********State Machine Definitions************/
/*************  Global Variables  ***************/
/*************Interrupt Prototypes***************/
/*************Function  Prototypes***************/
void Set_Frequency(unsigned int frequency);

/************* Device Definitions ***************/
/************* Module Definitions ***************/
/************* Other  Definitions ***************/

void Initialize_Variable_Transformer(void)
{
	//Red
	OC1RS = 1599;
	OC1R = 0;
	OC1CON1				= 0;
	OC1CON2				= 0;
	OC1CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC1CON1bits.OCM		= 0b110;	//110 = Edge-Aligned PWM mode on OCx
	OC1CON2bits.SYNCSEL	= 0b11111;	//11111 = This OC module
	OC1CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC1CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC1CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//Green
	OC2RS = 0;
	OC2R = 0;
	OC2CON1				= 0;
	OC2CON2				= 0;
	OC2CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC2CON1bits.OCM		= 0b101;	//
	OC2CON2bits.SYNCSEL	= 0b00001;	//00001 = Output Compare 1
	OC2CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC2CON2bits.OCTRIG	= 0;		//1 = Trigger OCx from source designated by SYNCSELx bits
	OC2CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin
		
	return;
}

void Variable_Transformer_Routine(unsigned long time_mS)
{
	Set_Frequency(60);

    return;
}

void Set_Frequency(unsigned int frequency)
{
    //Assign it to the registers
	OC1RS = PERIOD;
	OC1R = PERIOD / 2;	//Duty Cycle
	OC2RS = PERIOD - 2;
	OC2R = PERIOD - 6;

	return;
}
