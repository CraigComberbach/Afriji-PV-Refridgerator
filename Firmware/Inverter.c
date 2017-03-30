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
#include "Pins.h"

/************* Library Definition ***************/
/************* Semantic Versioning***************/
/*************Library Dependencies***************/
/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
#define	PERIOD			1599
#define	SIZE_OF_ARRAY	60
#define	DEADBAND		8	//Period resolution is 62.5nS, 8 time divisions allows for the waveform to peak at 12VDC or decay to 0V (which is a very efficient turn on point) before 

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
	SINE_360,
};

/***********  Structure Definitions  ************/
/***********State Machine Definitions************/
/*************  Global Variables  ***************/
int multiplier = 1;
int divider = 1;
int adder = 0;
unsigned int inverterLevel[] =
{
////10kHz @ 60 points (0-90º of a sine wave)
//41,		83,		124,	166,	207,	248,	289,	330,	371,	411,
//451,	491,	530,	569,	608,	646,	684,	721,	758,	795,
//830,	865,	900,	934,	967,	1000,	1032,	1063,	1094,	1124,
//1153,	1181,	1209,	1235,	1261,	1286,	1310,	1333,	1355,	1376,
//1397,	1416,	1435,	1452,	1468,	1484,	1498,	1512,	1524,	1535,
//1546,	1555,	1563,	1570,	1576,	1581,	1585,	1587,	1589,	1590
//10kHz @ 60 points (0-90º of a sine wave) Note: This series factors in the Rise/Fall times buffer
//32,		45,		57,		70,		82,		95,		107,	119,	132,	144,
//156,	168,	180,	192,	203,	215,	226,	237,	249,	260,
//270,	281,	291,	302,	312,	322,	331,	341,	350,	359,
//368,	376,	384,	393,	400,	408,	415,	422,	429,	435,
//441,	447,	453,	458,	463,	468,	472,	476,	480,	483,
//486,	489,	491,	494,	495,	497,	498,	499,	499,	500
//100kHz @ 60 points (0-90º of a sine wave) Note: This series factors in the Rise/Fall times buffer
25,		67,		109,	151,	192,	234,	276,	317,	358,	399,
439,	479,	519,	559,	598,	637,	675,	713,	750,	787,
823,	858,	893,	928,	961,	994,	1027,	1058,	1089,	1120,
1149,	1177,	1205,	1232,	1258,	1283,	1308,	1331,	1354,	1375,
1396,	1415,	1434,	1451,	1468,	1484,	1498,	1512,	1524,	1536,
1546,	1555,	1564,	1571,	1577,	1582,	1586,	1588,	1590,	1591



};

/*************Interrupt Prototypes***************/
/*************Function  Prototypes***************/
void Positive_Sine(int step);
void Negative_Sine(int step);
void Zero_Crossing(void);

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


	//OC1 - LOA
	OC1R = 0;
	OC1RS = PERIOD;
	OC1CON1				= 0;
	OC1CON2				= 0;
	OC1CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC1CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC1CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC1CON2bits.OCINV	= 0;
	OC1CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC1CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin
	
	//OC2 - HOA
	OC2R = 0;
	OC2RS = PERIOD;
	OC2CON1				= 0;
	OC2CON2				= 0;
	OC2CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC2CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC2CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC2CON2bits.OCINV	= 1;
	OC2CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC2CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC3 - HOB
	OC3R = 0;
	OC3RS = PERIOD;
	OC3CON1				= 0;
	OC3CON2				= 0;
	OC3CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC3CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC3CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC3CON2bits.OCINV	= 0;
	OC3CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC3CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC4 - LOB
	OC4R = 0;
	OC4RS = PERIOD;
	OC4CON1				= 0;
	OC4CON2				= 0;
	OC4CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC4CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC4CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC4CON2bits.OCINV	= 1;
	OC4CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC4CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC5 - One Reference to rule them all and in the darkness bind them [together]
	OC5R = 0;
	OC5RS = PERIOD;
	OC5CON1				= 0;
	OC5CON2				= 0;
	OC5CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC5CON1bits.OCM		= 0b110;	//110= Edge-Aligned PWM mode on OCx
	OC5CON2bits.SYNCSEL	= 0b11111;	//11111= This OC module
	OC5CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC5CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC5CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	return;
}

void Inverter_Routine(unsigned long time_mS)
{
	static enum SINE_WAVE_STAGES stage = SINE_0_TO_90;
	static int currentStep = 0;

	//The following code is an implementation of the TI App Note SLAA602 "800VA Pure Sine Wave Inverter?s Reference Design"
	//Positive going voltage
	//HOA	_/- - -...
	//LOB	- -\_/-...
	//HOB	_ _/-\_...
	//LOA	-\_ _ _...

	//Negative going voltage (Inverse of Positive waveform)
	//HOA	-\_ _ _...
	//LOB	_ _/-\_...
	//HOB	- -\_/-...
	//LOA	_/- - -...

	//It looks like the following three rules will always be true
	//1) Diagonals are the inverse of each other
	//2) Both diagonals are unique
	//3) Negative side is a direct inverse of positive
	
	switch(stage)
	{
		case SINE_0_TO_90:
			Positive_Sine(currentStep);

			//Advance in step or state
			++currentStep;
			if(currentStep >= (SIZE_OF_ARRAY-1))
				stage = SINE_90;
			break;
		case SINE_90:	//No special PWM event, only sampling is required
			Trigger_A2D_Scan();

			stage = SINE_90_TO_180;
			break;
		case SINE_90_TO_180:
			Positive_Sine(currentStep);

			if(--currentStep <= 0)
				stage = SINE_180;
			break;
		case SINE_180:
			Zero_Crossing();

			//Trigger an A2D scan
			Trigger_A2D_Scan();

			//Prep for advancement to the next step
			stage = SINE_180_TO_270;
			break;
		case SINE_180_TO_270:
			Negative_Sine(currentStep);

			if(++currentStep >= (SIZE_OF_ARRAY-1))
				stage = SINE_270;
			break;
		case SINE_270:	//No special PWM event, only sampling is required
			Trigger_A2D_Scan();
			stage = SINE_270_TO_360;
			break;
		case SINE_270_TO_360:
			Negative_Sine(currentStep);

			if(--currentStep <= 0)
				stage = SINE_360;
			break;
		case SINE_360:
			Zero_Crossing();

			//Transition to a positive waveform in a safe environment
			Positive_Sine(currentStep);

			//Trigger an A2D scan
			Trigger_A2D_Scan();

			//Prep for advancement to the next step
			stage = SINE_0_TO_90;
			break;
		default://How did we get here?
			#warning "Code not implemented"
			//I don't know where I am so I am going to allow circulation on the bottom half and reset to the start of the waveform
//			//LOA
//			OC1R = ;
//
//			//HOB
//			OC2R = ;
//
//			//HOA
//			OC3R = ;
//			OC3RS = ;
//
//			//LOB
//			OC4R = ;
//			OC4RS = ;
			currentStep = 0;
			stage = SINE_0_TO_90;
			break;
	}

    return;
}

void Positive_Sine(int step)
{
	//LOA - 100% Low
	OC1CON2bits.OCINV	= 1;
	OC1R				= 0;
	OC1RS				= PERIOD+1;

	//HOA - 100% High
	OC2CON2bits.OCINV	= 0;
	OC2R				= 0;
	OC2RS				= PERIOD+1;
	
	//LOB - Bathtub
	OC4CON2bits.OCINV	= 1;
	OC4R				= PERIOD - inverterLevel[step] - DEADBAND;
	OC4RS				= PERIOD - DEADBAND;

	//HOB - Mesa
	OC3CON2bits.OCINV	= 0;
	OC3R				= OC4R + DEADBAND;
	OC3RS				= OC4RS - DEADBAND;

	return;
}

void Negative_Sine(int step)
{
	//HOB - 100% High
	OC3CON2bits.OCINV	= 0;
	OC3R				= 0;
	OC3RS				= PERIOD+1;
	
	//LOB - 100% Low
	OC4CON2bits.OCINV	= 1;
	OC4R				= 0;
	OC4RS				= PERIOD+1;

	//LOA - Bathtub
	OC1CON2bits.OCINV	= 1;
	OC1R				= PERIOD - inverterLevel[step] - DEADBAND;
	OC1RS				= PERIOD - DEADBAND;

	//HOA - Mesa
	OC2CON2bits.OCINV	= 0;
	OC2R				= OC1R + DEADBAND;
	OC2RS				= OC1RS - DEADBAND;

	return;
}

void Zero_Crossing(void)
{
	//HOB - 100% High
	OC3CON2bits.OCINV	= 0;
	OC3R				= 0;
	OC3RS				= PERIOD+1;

	//HOA - Mesa
	OC2CON2bits.OCINV	= 0;
	OC2R				= 0;
	OC2RS				= PERIOD+1;
	
	//LOB - 100% Low
	OC4CON2bits.OCINV	= 1;
	OC4R				= 0;
	OC4RS				= PERIOD+1;

	//LOA - Bathtub
	OC1CON2bits.OCINV	= 1;
	OC1R				= 0;
	OC1RS				= PERIOD+1;

	return;
}
