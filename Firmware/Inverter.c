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
#define	PERIOD			159
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
unsigned int inverterOnPeriod[] =
{
//10kHz @ 60 points (0-90º of a sine wave) Note: This series factors in the Rise/Fall times buffer
//8,		51,		93,		135,	178,	220,	262,	303,	345,	386,
//427,	468,	508,	548,	587,	626,	665,	703,	741,	778,
//815,	851,	886,	921,	955,	988,	1021,	1053,	1084,	1115,
//1144,	1173,	1201,	1228,	1255,	1280,	1305,	1328,	1351,	1373,
//1394,	1413,	1432,	1450,	1467,	1483,	1497,	1511,	1524,	1535,
//1546,	1555,	1563,	1570,	1577,	1582,	1585,	1588,	1590,	1591, 1591
//10kHz @ 60 points (0-90º of a sine wave) Note: This series factors in the Rise/Fall times buffer AND zero crossing AND 95% max PWM
//0,		39,		79,		119,	158,	198,	237,	276,	315,	354,
//393,	431,	469,	507,	544,	581,	617,	653,	689,	724,
//759,	793,	827,	860,	892,	924,	955,	986,	1016,	1045,
//1074,	1101,	1128,	1155,	1180,	1205,	1228,	1251,	1273,	1295,
//1315,	1334,	1353,	1371,	1387,	1403,	1418,	1431,	1444,	1456,
//1467,	1477,	1485,	1493,	1500,	1506,	1510,	1514,	1516,	1518, 1518
//100kHz @ 60 points (0-90º of a sine wave) Note: This series factors in the Rise/Fall times buffer AND zero crossing AND 90% max PWM
1,		4,		8,		12,		15,		19,		23,		26,		30,		34,
37,		41,		44,		48,		51,		55,		58,		62,		65,		68,
72,		75,		78,		81,		84,		87,		90,		93,		96,		98,
101,	104,	106,	108,	111,	113,	115,	118,	120,	122,
123,	125,	127,	129,	130,	132,	133,	134,	136,	137,
138,	139,	139,	140,	141,	141,	142,	142,	142,	142
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
	OC1RS				= 0;		//Ensures it is off until needed
	OC1R				= PERIOD+1;	//Ensures it is off until needed
	OC1CON1				= 0;
	OC1CON2				= 0;
	OC1CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC1CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC1CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC1CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC1CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC1CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin
	
	//OC2 - HOA
	OC2RS				= 0;		//Ensures it is off until needed
	OC2R				= PERIOD+1;	//Ensures it is off until needed
	OC2CON1				= 0;
	OC2CON2				= 0;
	OC2CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC2CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC2CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC2CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC2CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC2CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC3 - HOB
	OC3RS				= 0;		//Ensures it is off until needed
	OC3R				= PERIOD+1;	//Ensures it is off until needed
	OC3CON1				= 0;
	OC3CON2				= 0;
	OC3CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC3CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC3CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC3CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC3CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC3CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC4 - LOB
	OC4RS				= 0;		//Ensures it is off until needed
	OC4R				= PERIOD+1;	//Ensures it is off until needed
	OC4CON1				= 0;
	OC4CON2				= 0;
	OC4CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC4CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC4CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC4CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
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

//Sign	++++++++++++...------------
//HOA	------------...__/-\_______
//LOA	____________...\_____/-----
//HOB	__/-\_______...------------
//LOB	\_____/-----...____________
void Positive_Sine(int step)
{
	//HOA - 100% High
	OC2R				= 0;
	OC2RS				= PERIOD+1;

	//LOA - 100% Low
	OC1RS				= 0;
	OC1R				= PERIOD+1;

	//LOB - Conducting current
	OC4R				= PERIOD - (inverterOnPeriod[step]*multiplier)/divider;
	OC4RS				= PERIOD;

	//HOB - Circulating current
	OC3R				= DEADBAND;
	OC3RS				= OC4R - DEADBAND;

//*****Above this line is Mikes waveform, below in my custom*****//
//	//HOA - 100% High
//	OC2R				= 0;
//	OC2RS				= PERIOD+1;
//
//	//HOB - 100% Low
//	OC3R				= PERIOD+1;
//	OC3RS				= 0;
//
//	//LOA - 100% Low
//	OC1R				= PERIOD+1;
//	OC1RS				= 0;
//
//	//LOB - Triggered
//	OC4R				= PERIOD - (inverterOnPeriod[step]*multiplier)/divider - DEADBAND;
//	OC4RS				= PERIOD - DEADBAND;

	return;
}

//100% Low
//	OCxR				= PERIOD+1;
//	OCxRS				= 0;
//	
//100% High
//	OCxR				= 0;
//	OCxRS				= PERIOD+1;

void Negative_Sine(int step)
{
	//LOA - Conducting Current
	OC1RS				= PERIOD;
	OC1R				= PERIOD - (inverterOnPeriod[step]*multiplier)/divider;

	//HOA - Circulating Current
	OC2R				= DEADBAND;
	OC2RS				= OC1R - DEADBAND;

	//LOB - 100% Low
	OC4RS				= 0;
	OC4R				= PERIOD+1;

	//HOB - 100% High
	OC3R				= 0;
	OC3RS				= PERIOD+1;

//*****above this line is Mikes waveform, below in my custom*****//
//	//HOA - 100% Low
//	OC2R				= PERIOD+1;
//	OC2RS				= 0;
//	
//	//HOB - 100% High
//	OC3R				= 0;
//	OC3RS				= PERIOD+1;
//
//	//LOA - Triggered
//	OC1R				= PERIOD - (inverterOnPeriod[step]*multiplier)/divider - DEADBAND;
//	OC1RS				= PERIOD - DEADBAND;
//
//	//LOB - 100% Low
//	OC4R				= PERIOD+1;
//	OC4RS				= 0;

	return;
}

void Zero_Crossing(void)
{
//	//HOA - 100% High
//	OC2R				= 0;
//	OC2RS				= PERIOD+1;
//
//	//HOB - 100% High
//	OC3R				= 0;
//	OC3RS				= PERIOD+1;
//
//	//LOB - 100% Low
//	OC4R				= PERIOD+1;
//	OC4RS				= 0;
//
//	//LOA - 100% Low
//	OC1R				= PERIOD+1;
//	OC1RS				= 0;
	return;
}
