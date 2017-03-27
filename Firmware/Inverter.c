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
#include "A2D.h"

/************* Library Definition ***************/
/************* Semantic Versioning***************/
/*************Library Dependencies***************/
/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
#define	PERIOD			159
#define	SIZE_OF_ARRAY	60

/*************    Enumeration     ***************/
enum SINE_WAVE_STAGES
{
	SINE_0_TO_90,
	SINE_90,
	SINE_90_TO_180,
	SINE_180,
	SINE_180_TO_270,
	SINE_270,
	SINE_270_TO_360,
	SINE_360
};

/***********  Structure Definitions  ************/
/***********State Machine Definitions************/
/*************  Global Variables  ***************/
unsigned int inverterLevel[SIZE_OF_ARRAY] =
{
//10kHz @ 60 points (0-90º of a sine wave)
41,		83,		124,	166,	207,	248,	289,	330,	371,	411,
451,	491,	530,	569,	608,	646,	684,	721,	758,	795,
830,	865,	900,	934,	967,	1000,	1032,	1063,	1094,	1124,
1153,	1181,	1209,	1235,	1261,	1286,	1310,	1333,	1355,	1376,
1397,	1416,	1435,	1452,	1468,	1484,	1498,	1512,	1524,	1535,
1546,	1555,	1563,	1570,	1576,	1581,	1585,	1587,	1589,	1590
};

/*************Interrupt Prototypes***************/
/*************Function  Prototypes***************/
/************* Device Definitions ***************/
#if FOSC_Hz > 65536000000  //65.536 GHz
    #error "FOSC is too fast for this code to remain unmodified"
#endif
	
/************* Module Definitions ***************/

void Initialize_Inverter(void)
{
//	Made using http://asciiflow.com/
//		+---- Vin+ ---+
//		|             |
//	  +-+-+         +-+-+
//	  |HOA|         |HOB|
//	  +-+-+         +-+-+
//		|             |
//		+-------------+
//		|             |
//	  +-+-+         +-+-+
//	  |LOA|         |LOB|
//	  +-+-+         +-+-+
//		|             |
//		+---- Vin- ---+

	#warning "The OCM Modules have not been configured, even in the slightest for using the TI App note"
	//OC1 - HOA
	OC1RS = PERIOD;
	OC1R = 0;
	OC1CON1				= 0;
	OC1CON2				= 0;
	OC1CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC1CON1bits.OCM		= 0b110;	//101= Double Compare Continuous Pulse mode: initialize OCx pin low, toggle OCx state continuously on alternate matches of OCxR and OCxRS
	OC1CON2bits.SYNCSEL	= 0b11111;	//00101 = Output Compare 5
	OC1CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC1CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC1CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC2 - HOB
	OC2RS = PERIOD;
	OC2R = 0;
	OC2CON1				= 0;
	OC2CON2				= 0;
	OC2CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC2CON1bits.OCM		= 0b110;	//101= Double Compare Continuous Pulse mode: initialize OCx pin low, toggle OCx state continuously on alternate matches of OCxR and OCxRS
	OC2CON2bits.SYNCSEL	= 7;		//00101 = Output Compare 5
	OC2CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC2CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC2CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC3 - LOA
	OC3RS = PERIOD;
	OC3R = 0;
	OC3CON1				= 0;
	OC3CON2				= 0;
	OC3CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC3CON1bits.OCM		= 0b110;	//101= Double Compare Continuous Pulse mode: initialize OCx pin low, toggle OCx state continuously on alternate matches of OCxR and OCxRS
	OC3CON2bits.SYNCSEL	= 0b11111;	//00101 = Output Compare 5
	OC3CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC3CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC3CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC4 - LOB
	OC4RS = PERIOD;
	OC4R = 0;
	OC4CON1				= 0;
	OC4CON2				= 0;
	OC4CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC4CON1bits.OCM		= 0b110;	//101= Double Compare Continuous Pulse mode: initialize OCx pin low, toggle OCx state continuously on alternate matches of OCxR and OCxRS
	OC4CON2bits.SYNCSEL	= 7;		//00101 = Output Compare 5
	OC4CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC4CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC4CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	return;
}

void Inverter_Routine(unsigned long time_mS)
{
	static enum SINE_WAVE_STAGES stage = SINE_0_TO_90;
	static int currentStep = 0;
	static int multiplier = 1;
	static int divider = 1;

	//The following code is an implementation of the TI App Note SLAA602 "800VA Pure Sine Wave Inverter?s Reference Design"
	//Positive going voltage
	//HOA	_|- - -...
	//LOB	- -|_|-...
	//HOB	_ _|-|_...
	//LOA	-|_ _ _...

	//Negative going voltage (Inverse of Positive waveform)
	//HOA	-|_ _ _...
	//LOB	_ _|-|_...
	//HOB	- -|_|-...
	//LOA	_|- - -...
	
	switch(stage)
	{
		case SINE_0_TO_90:
			#warning "Code not implemented"
//			(inverterLevel[currentStep]*multiplier)/divider;
//			OC1R = 0;
//			OC2R = 0;
//			OC3R = 0;
//			OC4R = 0;
			if(++currentStep >= SIZE_OF_ARRAY)
				stage = SINE_90;
			break;
		case SINE_90:
			OC1R = 0;
			OC2R = 0;
			OC3R = 0;
			OC4R = 0;
			Trigger_A2D_Scan;
			stage = SINE_90_TO_180;
			break;
		case SINE_90_TO_180:
			#warning "Code not implemented"
			if(--currentStep <= 0)
				stage = SINE_180;
			break;
		case SINE_180:
			OC1R = 0;
			OC2R = 0;
			OC3R = 0;
			OC4R = 0;
			Trigger_A2D_Scan;
			stage = SINE_180_TO_270;
			break;
		case SINE_180_TO_270:
			#warning "Code not implemented"
			if(++currentStep >= SIZE_OF_ARRAY)
				stage = SINE_270;
			break;
		case SINE_270:
			OC1R = 0;
			OC2R = 0;
			OC3R = 0;
			OC4R = 0;
			Trigger_A2D_Scan;
			stage = SINE_270_TO_360;
			break;
		case SINE_270_TO_360:
			#warning "Code not implemented"
			if(--currentStep <= 0)
				stage = SINE_360;
			break;
		case SINE_360:
			OC1R = 0;
			OC2R = 0;
			OC3R = 0;
			OC4R = 0;
			Trigger_A2D_Scan;
			stage = SINE_0_TO_90;
			break;
		default://How did we get here?
			OC1R = 0;
			OC2R = 0;
			OC3R = 0;
			OC4R = 0;
			stage = SINE_0_TO_90;
			break;
	}

    return;
}
