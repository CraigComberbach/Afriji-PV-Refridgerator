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
#define	PERIOD						159
#define	SIZE_OF_ARRAY				10
#define	DEADBAND					2	//Period resolution is 62.5nS; This is the delay between turning on/off one and turning on/off the other
#define NUMBER_OF_uS_IN_ONE_SECOND	1000000
#define VOLTAGE_TARGET_DEADBAND		10	//Voltage with one decimal of accuracy
#define MULTIPLIER_MAXIMUM			100

/*************    Enumeration     ***************/
enum SINE_WAVE_STAGES
{
	SINE_0_TO_90,
	SINE_90_TO_180,
	SINE_180_TO_270,
	SINE_270_TO_360,
};

/***********  Structure Definitions  ************/
struct INVERTER_VARIABLES
{
	int multiplier;
	int divider;
	enum SINE_WAVE_STAGES phaseRange;
	int currentStep;
	int delayCounter_uS;
	int targetDelay_uS;
} Invertahoy[NUMBER_OF_INVERTERS_SUPPORTED];
/***********State Machine Definitions************/
/*************  Global Variables  ***************/
int targetVoltage[NUMBER_OF_INVERTERS_SUPPORTED];	//Peak voltage with one decimal of accuracy
unsigned int inverterOnPeriod[SIZE_OF_ARRAY] =
{
//100kHz @ 10 points (0-90º of a sine wave) Note: This series factors in the Rise/Fall times buffer, Min on time, AND zero crossing
5,		30,		55,		79,		100,
118,	133,	144,	150,	153
};

/*************Interrupt Prototypes***************/
/*************Function  Prototypes***************/
void Positive_Sine(int step, enum INVERTERS_SUPPORTED inverter);
void Negative_Sine(int step, enum INVERTERS_SUPPORTED inverter);
void Peaks(enum INVERTERS_SUPPORTED inverter);
void Calculate_Sine_Wave(int step, enum INVERTERS_SUPPORTED inverter, int *conductingCurrentPeriod, int *circulatingCurrentPeriod);

/************* Device Definitions ***************/	
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

	int loop;
	
	for(loop = 0; loop < NUMBER_OF_INVERTERS_SUPPORTED; ++loop)
	{
		Invertahoy[loop].phaseRange = SINE_0_TO_90;
		Invertahoy[loop].currentStep = 0;
		Invertahoy[loop].multiplier = 100;
		Invertahoy[loop].divider = 100;
		Invertahoy[loop].delayCounter_uS = 0;
		Invertahoy[loop].targetDelay_uS = 416;//60Hz
		targetVoltage[loop] = 566;	//Peak voltage with one decimal of accuracy
	}
	
	//OC1 - LOA HiA
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
	
	//OC2 - HOA HiA
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

	//OC3 - HOB HiA
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

	//OC4 - LOB HiA
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
	
	//OC6 - LOA HiV
	OC6RS				= 0;		//Ensures it is off until needed
	OC6R				= PERIOD+1;	//Ensures it is off until needed
	OC6CON1				= 0;
	OC6CON2				= 0;
	OC6CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC6CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC6CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC6CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC6CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC6CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin
	
	//OC7 - HOA HiV
	OC7RS				= 0;		//Ensures it is off until needed
	OC7R				= PERIOD+1;	//Ensures it is off until needed
	OC7CON1				= 0;
	OC7CON2				= 0;
	OC7CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC7CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC7CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC7CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC7CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC7CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC8 - HOB HiV
	OC8RS				= 0;		//Ensures it is off until needed
	OC8R				= PERIOD+1;	//Ensures it is off until needed
	OC8CON1				= 0;
	OC8CON2				= 0;
	OC8CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC8CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC8CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC8CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC8CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC8CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC9 - LOB HiV
	OC9RS				= 0;		//Ensures it is off until needed
	OC9R				= PERIOD+1;	//Ensures it is off until needed
	OC9CON1				= 0;
	OC9CON2				= 0;
	OC9CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC9CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC9CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC9CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC9CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC9CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

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

void Inverter_Routine(unsigned long time_uS)
{
	int currentInverter;

	//Cycle through the inverters
	for(currentInverter = 0; currentInverter < NUMBER_OF_INVERTERS_SUPPORTED; ++currentInverter)
	{
		//Update the amount of time since the waveform was last updated
		Invertahoy[currentInverter].delayCounter_uS += time_uS;

		//Check to see if we are due to update the waveform
		if(Invertahoy[currentInverter].delayCounter_uS >= Invertahoy[currentInverter].targetDelay_uS)
		{
			//Reset the counter for the next waveform update
			Invertahoy[currentInverter].delayCounter_uS = 0;

			//Update the waveform as required
			switch(Invertahoy[currentInverter].phaseRange)
			{
				case SINE_0_TO_90:
					Positive_Sine(Invertahoy[currentInverter].currentStep, currentInverter);

					//Advance in step or state
					++Invertahoy[currentInverter].currentStep;
					if(Invertahoy[currentInverter].currentStep >= (SIZE_OF_ARRAY))
					{
						Peaks(currentInverter);
						Invertahoy[currentInverter].currentStep = SIZE_OF_ARRAY-1;
						Invertahoy[currentInverter].phaseRange = SINE_90_TO_180;
					}
					break;
				case SINE_90_TO_180:
					Positive_Sine(Invertahoy[currentInverter].currentStep, currentInverter);

					//Advance in step or state
					--Invertahoy[currentInverter].currentStep;
					if(--Invertahoy[currentInverter].currentStep < 0)
					{
						Invertahoy[currentInverter].currentStep = 0;
						Invertahoy[currentInverter].phaseRange = SINE_180_TO_270;
					}
					break;
				case SINE_180_TO_270:
					Negative_Sine(Invertahoy[currentInverter].currentStep, currentInverter);

					//Advance in step or state
					++Invertahoy[currentInverter].currentStep;
					if(Invertahoy[currentInverter].currentStep >= (SIZE_OF_ARRAY))
					{
						Peaks(currentInverter);
						Invertahoy[currentInverter].currentStep = SIZE_OF_ARRAY-1;
						Invertahoy[currentInverter].phaseRange = SINE_270_TO_360;
					}
					break;
				case SINE_270_TO_360:
					Negative_Sine(Invertahoy[currentInverter].currentStep, currentInverter);

					//Advance in step or state
					--Invertahoy[currentInverter].currentStep;
					if(--Invertahoy[currentInverter].currentStep < 0)
					{
						Invertahoy[currentInverter].currentStep = 0;
						Invertahoy[currentInverter].phaseRange = SINE_0_TO_90;
					}
					break;
				default://How did we get here?
					//Cycle current through the top FETs and prepare to start back at zero degrees
					switch(currentInverter)
					{
						#ifdef HiI_INVERTER_ENABLED
						case HIGH_CURRENT:
							//HOA - 100% High
							OC2R				= 0;
							OC2RS				= PERIOD+1;

							//HOB - 100% High
							OC3R				= 0;
							OC3RS				= PERIOD+1;

							//LOB - 100% Low
							OC4RS				= 0;
							OC4R				= PERIOD+1;

							//LOA - 100% Low
							OC1RS				= 0;
							OC1R				= PERIOD+1;

							break;
						#endif
						#ifdef HiV_INVERTER_ENABLED
						case HIGH_VOLTAGE:
							//HOA - 100% High
							OC7R				= 0;
							OC7RS				= PERIOD+1;

							//HOB - 100% High
							OC8R				= 0;
							OC8RS				= PERIOD+1;

							//LOB - 100% Low
							OC9RS				= 0;
							OC9R				= PERIOD+1;

							//LOA - 100% Low
							OC6RS				= 0;
							OC6R				= PERIOD+1;

							break;
						#endif
						default:
							break;
					}
					Invertahoy[currentInverter].currentStep = 0;
					Invertahoy[currentInverter].phaseRange = SINE_0_TO_90;
					break;
			}
		}
	}

    return;
}

//Sign	++++++++++++...------------
//HOA	------------...__/-\_______
//LOA	____________...\_____/-----
//HOB	__/-\_______...------------
//LOB	\_____/-----...____________
void Positive_Sine(int step, enum INVERTERS_SUPPORTED inverter)
{
	int circulatingCurrentPeriod;
	int conductingCurrentPeriod;

	Calculate_Sine_Wave(step, inverter, &conductingCurrentPeriod, &circulatingCurrentPeriod);

	switch(inverter)
	{
		#ifdef HiI_INVERTER_ENABLED
		case HIGH_CURRENT:
			//LOA - 100% Low
			OC1RS				= 0;
			OC1R				= PERIOD+1;

			//HOA - 100% High
			OC2R				= 0;
			OC2RS				= PERIOD+1;

			//HOB - Circulating current
			OC3R				= DEADBAND;
			OC3RS				= circulatingCurrentPeriod;

			//LOB - Conducting current
			OC4R				= conductingCurrentPeriod;
			OC4RS				= PERIOD;

			break;
		#endif
		#ifdef HiV_INVERTER_ENABLED
		case HIGH_VOLTAGE:
			//LOA - 100% Low
			OC6RS				= 0;
			OC6R				= PERIOD+1;

			//HOA - 100% High
			OC7R				= 0;
			OC7RS				= PERIOD+1;

			//HOB - Circulating current
			OC8R				= DEADBAND;
			OC8RS				= circulatingCurrentPeriod;

			//LOB - Conducting current
			OC9R				= conductingCurrentPeriod;
			OC9RS				= PERIOD;

			break;
		#endif
		default:
			break;
	}

	return;
}

//100% Low
//	OCxRS				= 0;
//	OCxR				= PERIOD+1;
//	
//100% High
//	OCxR				= 0;
//	OCxRS				= PERIOD+1;

void Negative_Sine(int step, enum INVERTERS_SUPPORTED inverter)
{
	int circulatingCurrentPeriod;
	int conductingCurrentPeriod;
	
	Calculate_Sine_Wave(step, inverter, &conductingCurrentPeriod, &circulatingCurrentPeriod);

	switch(inverter)
	{
		#ifdef HiI_INVERTER_ENABLED
		case HIGH_CURRENT:
			//HOA - Circulating Current
			OC2R				= DEADBAND;
			OC2RS				= circulatingCurrentPeriod;

			//LOA - Conducting Current
			OC1R				= conductingCurrentPeriod;
			OC1RS				= PERIOD;

			//LOB - 100% Low
			OC4RS				= 0;
			OC4R				= PERIOD+1;

			//HOB - 100% High
			OC3R				= 0;
			OC3RS				= PERIOD+1;

			break;
		#endif
		#ifdef HiV_INVERTER_ENABLED
		case HIGH_VOLTAGE:
			//HOA - Circulating Current
			OC7R				= DEADBAND;
			OC7RS				= circulatingCurrentPeriod;

			//LOA - Conducting Current
			OC6R				= conductingCurrentPeriod;
			OC6RS				= PERIOD;

			//LOB - 100% Low
			OC9RS				= 0;
			OC9R				= PERIOD+1;

			//HOB - 100% High
			OC8R				= 0;
			OC8RS				= PERIOD+1;

			break;
		#endif
		default:
			break;
	}

	return;
}

void Calculate_Sine_Wave(int step, enum INVERTERS_SUPPORTED inverter, int *conductingCurrentPeriod, int *circulatingCurrentPeriod)
{
	//Precalculate periods
	*circulatingCurrentPeriod	= PERIOD - (inverterOnPeriod[step]*Invertahoy[inverter].multiplier)/Invertahoy[inverter].divider - DEADBAND;
	*conductingCurrentPeriod	= PERIOD - (inverterOnPeriod[step]*Invertahoy[inverter].multiplier)/Invertahoy[inverter].divider;
	
	//Check to see if the circulating current period is valid and cap it if is not
	if(*circulatingCurrentPeriod < (PERIOD - inverterOnPeriod[SIZE_OF_ARRAY-1] - DEADBAND))
		*circulatingCurrentPeriod = PERIOD - inverterOnPeriod[SIZE_OF_ARRAY-1] - DEADBAND;
	else if(*circulatingCurrentPeriod > (PERIOD - inverterOnPeriod[0]) - DEADBAND)
		*circulatingCurrentPeriod = PERIOD - inverterOnPeriod[0] - DEADBAND;

	//Check to see if the conducting current period is valid and cap it if is not
	if(*conductingCurrentPeriod < (PERIOD - inverterOnPeriod[SIZE_OF_ARRAY-1]))
		*conductingCurrentPeriod = PERIOD - inverterOnPeriod[SIZE_OF_ARRAY-1];
	else if(*conductingCurrentPeriod > (PERIOD - inverterOnPeriod[0]))
		*conductingCurrentPeriod = PERIOD - inverterOnPeriod[0];

	return;
}

void Set_Target_Delay_uS(int newDelay_uS, enum INVERTERS_SUPPORTED inverter)
{
	Invertahoy[inverter].targetDelay_uS = newDelay_uS;
	return;
}

int Get_Target_Delay_uS(enum INVERTERS_SUPPORTED inverter)
{
	return Invertahoy[inverter].targetDelay_uS;
}

void Set_Frequency_Hz(int newFrequency_Hz, enum INVERTERS_SUPPORTED inverter)
{
	long temp;
	temp = NUMBER_OF_uS_IN_ONE_SECOND;	//Number of uS in one second
	temp /= newFrequency_Hz;			//Divide by frequency to get number of uS per full cycle
	temp /= SIZE_OF_ARRAY * 4;			//Divide by number of distinct steps in a full wave
	Invertahoy[inverter].targetDelay_uS = (int)temp;
	return;
}

int Get_Frequency_Hz(enum INVERTERS_SUPPORTED inverter)
{
	long temp;
	temp = NUMBER_OF_uS_IN_ONE_SECOND;			//Number of uS in one second
	temp /= SIZE_OF_ARRAY * 4;					//Divide by number of distinct steps in a full wave
	temp /= Invertahoy[inverter].targetDelay_uS;//Divide by current delay
	return (int)temp;
}

void Peaks(enum INVERTERS_SUPPORTED inverter)
{
	int currentVoltage;

	switch(inverter)
	{
		#ifdef HiI_INVERTER_ENABLED
		case HIGH_CURRENT:
			//Take a sample; it won't give me a result for THIS calculation, but will be ready by the next one
			Trigger_A2D_Scan();

			//Check if the voltage needs to be adjusted
			currentVoltage = A2D_Value(A2D_AN12_VDC_BUS_PLUS);
			break;
		#endif
		#ifdef HiV_INVERTER_ENABLED
		case HIGH_VOLTAGE:
			//Take a sample; it won't give me a result for THIS calculation, but will be ready by the next one
			Trigger_A2D_Scan();

			currentVoltage = A2D_Value(A2D_AN13_TRANSFORMER_SECONDARY_PLUS);
			break;
		#endif
		default:
			break;
	}

	//Check if voltage control is required
	if(currentVoltage < (targetVoltage[inverter] - VOLTAGE_TARGET_DEADBAND))//Voltage is too low
		Invertahoy[inverter].multiplier++;
	else if(currentVoltage > (targetVoltage[inverter] + VOLTAGE_TARGET_DEADBAND))//Voltage is too high
		Invertahoy[inverter].multiplier--;

	//Apply caps to multiplier as required
	if(Invertahoy[inverter].multiplier > MULTIPLIER_MAXIMUM)
		Invertahoy[inverter].multiplier = MULTIPLIER_MAXIMUM;
	else if(Invertahoy[inverter].multiplier <= 0)
		Invertahoy[inverter].multiplier = 1;
	
	return;
}

void Set_Voltage_Target(int newTarget, enum INVERTERS_SUPPORTED inverter)
{
	if((inverter >= 0) && (inverter < NUMBER_OF_INVERTERS_SUPPORTED))
		targetVoltage[inverter] = newTarget;

	return;
}

int Get_Voltage_Target(enum INVERTERS_SUPPORTED inverter)
{
	return targetVoltage[inverter];
}

