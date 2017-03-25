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
#include "Inverter.h"
#include "Pins.h"

/************* Library Definition ***************/
/************* Semantic Versioning***************/
/*************Library Dependencies***************/
/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
#define	PERIOD			159
#define MULTIPLIER		3
#define DIVIDER			40//12-20
#define	SIZE_OF_ARRAY	120

/*************    Enumeration     ***************/
/***********  Structure Definitions  ************/
/***********State Machine Definitions************/
/*************  Global Variables  ***************/
unsigned int inverterLevel[SIZE_OF_ARRAY] =
{
//10kHz @ 120 points
41,		83,		124,	166,	207,	248,	289,	330,	371,	411,
451,	491,	530,	569,	608,	646,	684,	721,	758,	795,
830,	865,	900,	934,	967,	1000,	1032,	1063,	1094,	1124,
1153,	1181,	1209,	1235,	1261,	1286,	1310,	1333,	1355,	1376,
1397,	1416,	1435,	1452,	1468,	1484,	1498,	1512,	1524,	1535,
1546,	1555,	1563,	1570,	1576,	1581,	1585,	1587,	1589,	1590,
1589,	1587,	1585,	1581,	1576,	1570,	1563,	1555,	1546,	1535,
1524,	1512,	1498,	1484,	1468,	1452,	1435,	1416,	1397,	1376,
1355,	1333,	1310,	1286,	1261,	1235,	1209,	1181,	1153,	1124,
1094,	1063,	1032,	1000,	967,	934,	900,	865,	830,	795,
758,	721,	684,	646,	608,	569,	530,	491,	451,	411,
371,	330,	289,	248,	207,	166,	124,	83,		41,		0,
};

/*************Interrupt Prototypes***************/
/*************Function  Prototypes***************/
void Positive_Wave(int currentStep);
void Negative_Wave(int currentStep);

/************* Device Definitions ***************/
#if FOSC_Hz > 65536000000  //65.536 GHz
    #error "FOSC is too fast for this code to remain unmodified"
#endif
	
/************* Module Definitions ***************/
#define CbTRIS	OC7CON2bits.OCTRIS
#define DbTRIS	OC8CON2bits.OCTRIS

void Initialize_Inverter(void)
{
	//OC7 - Output_Inverter_Pos-Bottom
	OC7RS = PERIOD;
	OC7R = 0;
	OC7CON1				= 0;
	OC7CON2				= 0;
	OC7CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC7CON1bits.OCM		= 0b110;	//101= Double Compare Continuous Pulse mode: initialize OCx pin low, toggle OCx state continuously on alternate matches of OCxR and OCxRS
	OC7CON2bits.SYNCSEL	= 0b11111;	//00101 = Output Compare 5
	OC7CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC7CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC7CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC8 - Output_Inverter_Neg-Bottom
	OC8RS = PERIOD;
	OC8R = 0;
	OC8CON1				= 0;
	OC8CON2				= 0;
	OC8CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC8CON1bits.OCM		= 0b110;	//101= Double Compare Continuous Pulse mode: initialize OCx pin low, toggle OCx state continuously on alternate matches of OCxR and OCxRS
	OC8CON2bits.SYNCSEL	= 7;		//00101 = Output Compare 5
	OC8CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC8CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC8CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	return;
}

void Inverter_Routine(unsigned long time_mS)
{
	static int currentStep = 0;

	if(currentStep < SIZE_OF_ARRAY)
		Positive_Wave(currentStep);
	else if(currentStep == (SIZE_OF_ARRAY+1))
	{
		OC7R = 0;
		OC8R = 0;
		Pin_Low(PIN_RG7_SWITCHED_GROUND2);
	}
	else if(currentStep < (2*SIZE_OF_ARRAY+1))
		Negative_Wave(currentStep-(SIZE_OF_ARRAY+1));
	else if(currentStep >= (2*SIZE_OF_ARRAY+2))
	{
		OC7R = 0;
		OC8R = 0;
		Pin_High(PIN_RG7_SWITCHED_GROUND2);
	}
		
	if(currentStep >= (2*SIZE_OF_ARRAY+2))
		currentStep = 0;
	else
		++currentStep;

    return;
}
void Positive_Wave(int currentStep)
{
	//Generate Positive Sine Wave
	OC7R = (inverterLevel[currentStep]*MULTIPLIER)/DIVIDER;	//OC7 - Ab & Cb

	return;
}

void Negative_Wave(int currentStep)
{
	//Generate Negative Sine Wave
	OC8R = (inverterLevel[currentStep]*MULTIPLIER)/DIVIDER;	//OC8 - Db

	return;
}
