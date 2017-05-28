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
#define	PWM_PERIOD_CYCLES			159	//# of operations between PWM module resets
#define	DEADBAND					2	//Period resolution is 62.5nS; This is the delay between turning on/off one and turning on/off the other
#define NUMBER_OF_uS_IN_ONE_SECOND	1000000
#define VOLTAGE_TARGET_DEADBAND		10	//Voltage with one decimal of accuracy
//#define MULTIPLIER_MAXIMUM			100

/*************    Enumeration     ***************/


/***********	Structure Definitions	***********/
struct ERROR_FLAGS
{
    int overCurrent;
    int maxSlopeExceeded;
    int targetExceededSupply;
} inverterErrorFlags[NUMBER_OF_INVERTERS_SUPPORTED];

struct INVERTER_VARIABLES
{
	int targetOutputVoltage_Vx10;
	int targetOutputVoltageShadow_Vx10;
	int targetOutputFrequency_Hz;
	int targetOutputFrequencyShadow_Hz;
	unsigned long targetOutputPeriod_us;
	int ratedOutputVoltage_Vx10;
	int ratedOutputFrequency_Hz;
	int ratedOutputPeriod_us;
	unsigned long currentTime_us;	//Time is referenced to last zero degrees
	int angle_degx10;
	int maxCurrentTripPickup_mA;
	int maxCurrentTripDelay_us;
	int lastPeakPosCurrent_mA;
	int lastPeakNegCurrent_mA;
	
} InverterConfigData[NUMBER_OF_INVERTERS_SUPPORTED];
 
void Set_Target_Output_Voltage_Vx10(int input, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].targetOutputVoltageShadow_Vx10 = input;
}
 
int Get_Target_Output_Voltage_Vx10(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].targetOutputVoltage_Vx10;
}
 
int Get_Target_Output_Voltage_Shadow_Vx10(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].targetOutputVoltageShadow_Vx10;
}
 
void Set_Target_Output_Frequency_Hz (int input, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].targetOutputFrequencyShadow_Hz = input;
}
 
int Get_Target_Output_Frequency_Hz(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].targetOutputFrequency_Hz;
}
 
int Get_Target_Output_Frequency_Shadow_Hz(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].targetOutputFrequencyShadow_Hz;
}
 
int Get_Target_Output_Period_us (enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].targetOutputPeriod_us;
}
 
void Set_Rated_Output_Voltage_Vx10(int input, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].ratedOutputVoltage_Vx10 = input;
}
 
int Get_Rated_Output_Voltage_Vx10(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].ratedOutputVoltage_Vx10;
}
 
void Set_Rated_Output_Frequency_Hz (int input, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].ratedOutputFrequency_Hz = input;
}
 
int Get_Rated_Output_Frequency_Hz(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].ratedOutputFrequency_Hz;
}
 
int Get_Rated_Output_Period_us (enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].ratedOutputPeriod_us;
}
 
void Set_Max_Current_Trip_Pickup_mA (int input, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].maxCurrentTripPickup_mA = input;
}
 
int Get_Max_Current_Trip_Pickup_mA(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].maxCurrentTripPickup_mA;
}
 
void Set_Max_Current_Trip_Delay_ms (int input, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].maxCurrentTripDelay_us = input;
}
 
int Get_Max_Current_Trip_Delay_ms (enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].maxCurrentTripDelay_us;
}
 
 
/***********State Machine Definitions************/
 /*************	Global Variables	**************/
int pwmPeriod_us;
 
 
void Set_PWM_Period_us(int input)
{
	pwmPeriod_us = input;
}
 
int Get_PWM_Period_us(void)
{
	return pwmPeriod_us;
}

/*************Interrupt Prototypes***************/
/*************Function  Prototypes***************/
int Converter_Time_To_Angle(int currentInverter, unsigned int currentTime, unsigned int outputPeriod);
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
//		+---|LOAD|----+
//		|             |
//	  +-+-+         +-+-+
//	  |LOA|         |LOB|
//	  +-+-+         +-+-+
//		|             |
//		+---- Vin- ---+

	int loop;
	
	for(loop = 0; loop < NUMBER_OF_INVERTERS_SUPPORTED; ++loop)
	{
		InverterConfigData[loop].timeSinceLastPWMStep_us = 0;
		InverterConfigData[loop].angle_degx10 = 0;
		InverterConfigData[loop].targetOutputVoltage_Vx10 = 1697;
		InverterConfigData[loop].counterToNextPWM_Period_Reset = 0;
		InverterConfigData[loop].targetOutputPeriod_cycles = 0;
		InverterConfigData[loop].ratedOutputPeriod_cycles = 0;
	}
	
	//Set initial starting conditions
	#ifdef HiI_INVERTER_ENABLED
	Set_Output_Hz (60 /*Hz*/, HIGH_CURRENT_INVERTER);
	Set_Rated_Hz (60 /*Hz*/ , HIGH_CURRENT_INVERTER );
	Set_Rated_RMS_Voltage (1200/*120.0 Vrms*/,	HIGH_CURRENT_INVERTER);	//Peak voltage of a (2V/Hz*f + 10 V * e^((-f+20Hz)/10Hz)=120Vrms sine wave
	#endif
	
	#ifdef HiVolt_INVERTER_ENABLED
	Set_Output_Hz (20/*Hz*/,	HIGH_CURRENT_INVERTER);	//Motor min starting speed is 20 Hz
	Set_Rated_Hz (60, HIGH_CURRENT_INVERTER );	//Motor rated speed is 60 Hz / 1800 rpm
	Set_Rated_RMS_Voltage (1200/*120.0 Vrms*/,	HIGH_CURRENT_INVERTER);	//Peak voltage of a (2V/Hz*f + 10 V * e^((-f+20Hz)/10Hz)=120Vrms sine wave
	#endif
	
	//OC1 - LOA Hi Current
	OC1RS				= 0;		//Ensures it is off until needed
	OC1R				= PWM_PERIOD_CYCLES+1;	//Ensures it is off until needed
	OC1CON1				= 0;
	OC1CON2				= 0;
	OC1CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC1CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC1CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC1CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC1CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC1CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin
	
	//OC2 - HOA Hi Current
	OC2RS				= 0;		//Ensures it is off until needed
	OC2R				= PWM_PERIOD_CYCLES+1;	//Ensures it is off until needed
	OC2CON1				= 0;
	OC2CON2				= 0;
	OC2CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC2CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC2CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC2CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC2CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC2CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC3 - HOB Hi Current
	OC3RS				= 0;		//Ensures it is off until needed
	OC3R				= PWM_PERIOD_CYCLES+1;	//Ensures it is off until needed
	OC3CON1				= 0;
	OC3CON2				= 0;
	OC3CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC3CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC3CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC3CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC3CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC3CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC4 - LOB Hi Current
	OC4RS				= 0;		//Ensures it is off until needed
	OC4R				= PWM_PERIOD_CYCLES+1;	//Ensures it is off until needed
	OC4CON1				= 0;
	OC4CON2				= 0;
	OC4CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC4CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC4CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC4CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC4CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC4CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin
	
	//OC5 - Master Timer 
	/* !!! One reference timer to rule them all and in the darkness bind them [together] !!! */
	OC5R = 0;
	OC5RS = PWM_PERIOD_CYCLES;
	OC5CON1				= 0;
	OC5CON2				= 0;
	OC5CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC5CON1bits.OCM		= 0b110;	//110= Edge-Aligned PWM mode on OCx
	OC5CON2bits.SYNCSEL	= 0b11111;	//11111= This OC module
	OC5CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC5CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC5CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin
	
	//OC6 - LOA Hi Voltage
	OC6RS				= 0;		//Ensures it is off until needed
	OC6R				= PWM_PERIOD_CYCLES+1;	//Ensures it is off until needed
	OC6CON1				= 0;
	OC6CON2				= 0;
	OC6CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC6CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC6CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC6CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC6CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC6CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin
	
	//OC7 - HOA Hi Voltage
	OC7RS				= 0;		//Ensures it is off until needed
	OC7R				= PWM_PERIOD_CYCLES+1;	//Ensures it is off until needed
	OC7CON1				= 0;
	OC7CON2				= 0;
	OC7CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC7CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC7CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC7CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC7CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC7CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC8 - HOB Hi Voltage
	OC8RS				= 0;		//Ensures it is off until needed
	OC8R				= PWM_PERIOD_CYCLES+1;	//Ensures it is off until needed
	OC8CON1				= 0;
	OC8CON2				= 0;
	OC8CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC8CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC8CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC8CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC8CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC8CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC9 - LOB Hi Voltage
	OC9RS				= 0;		//Ensures it is off until needed
	OC9R				= PWM_PERIOD_CYCLES+1;	//Ensures it is off until needed
	OC9CON1				= 0;
	OC9CON2				= 0;
	OC9CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC9CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC9CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC9CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC9CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC9CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	return;
}

void Inverter_Routine(unsigned long time_uS)
{
    int currentInverter;
    int theta;

    for(currentInverter = 0; currentInverter < NUMBER_OF_INVERTERS_SUPPORTED; ++currentInverter)
    {
        //Update Timer
        InverterConfigData[currentInverter].currentTime_uS += time_uS;

        //Convert Time to Angle
        theta = Converter_Time_To_Angle(currentInverter, InverterConfigData[currentInerter].currentTime_uS, InverterConfigData[currentInerter].targetOutputPeriod_uS);

        //Over-Current Detection
        Over_Current_Watch(currentInverter);

        //Calculate Amplitude Factor
        //Calculate PWM Duty Percent
        //Update PWM Registers
        //The big If
//        if that thing be true
//        {
//            Shadow registers need setting;
//            InverterConfigData[currentInverter].currentTime_uS = 0;
//        }
    }
    
//	int currentInverter;
//
//	//Cycle through the inverters
//	for(currentInverter = 0; currentInverter < NUMBER_OF_INVERTERS_SUPPORTED; ++currentInverter)
//	{
//		//Update the amount of time since the waveform was last updated
//		InverterConfigData[currentInverter].currentStep_us += time_uS;
//
//		//Check to see if we are due to update the waveform
//		if(InverterConfigData[currentInverter].currentStep_us >= InverterConfigData[currentInverter].delayBetweenInverterStep_Cyc)
//		{
//			//Reset the counter for the next waveform update
//			InverterConfigData[currentInverter].counterToNextInverterStep_uS = 0;
//
//			//Update the waveform as required
//			switch(InverterConfigData[currentInverter].phaseRange)
//			{
//				case SINE_0_TO_90:
//					Positive_Sine(InverterConfigData[currentInverter].currentStep, currentInverter);
//
//					//Advance in step or state
//					++InverterConfigData[currentInverter].currentStep;
//					if(InverterConfigData[currentInverter].currentStep >= (SIZE_OF_ARRAY))
//					{
//						Peaks(currentInverter);
//						InverterConfigData[currentInverter].currentStep = SIZE_OF_ARRAY-1;
//						InverterConfigData[currentInverter].phaseRange = SINE_90_TO_180;
//					}
//					break;
//				case SINE_90_TO_180:
//					Positive_Sine(InverterConfigData[currentInverter].currentStep, currentInverter);
//
//					//Advance in step or state
//					--InverterConfigData[currentInverter].currentStep;
//					if(--InverterConfigData[currentInverter].currentStep < 0)
//					{
//						InverterConfigData[currentInverter].currentStep = 0;
//						InverterConfigData[currentInverter].phaseRange = SINE_180_TO_270;
//					}
//					break;
//				case SINE_180_TO_270:
//					Negative_Sine(InverterConfigData[currentInverter].currentStep, currentInverter);
//
//					//Advance in step or state
//					++InverterConfigData[currentInverter].currentStep;
//					if(InverterConfigData[currentInverter].currentStep >= (SIZE_OF_ARRAY))
//					{
//						Peaks(currentInverter);
//						InverterConfigData[currentInverter].currentStep = SIZE_OF_ARRAY-1;
//						InverterConfigData[currentInverter].phaseRange = SINE_270_TO_360;
//					}
//					break;
//				case SINE_270_TO_360:
//					Negative_Sine(InverterConfigData[currentInverter].currentStep, currentInverter);
//
//					//Advance in step or state
//					--InverterConfigData[currentInverter].currentStep;
//					if(--InverterConfigData[currentInverter].currentStep < 0)
//					{
//						InverterConfigData[currentInverter].currentStep = 0;
//						InverterConfigData[currentInverter].phaseRange = SINE_0_TO_90;
//					}
//					break;
//				default://How did we get here?
//					//Cycle current through the top FETs and prepare to start back at zero degrees
//					switch(currentInverter)
//					{
//						#ifdef HiI_INVERTER_ENABLED
//						case HIGH_CURRENT:
//							//HOA - 100% High
//							OC2R				= 0;
//							OC2RS				= PERIOD+1;
//
//							//HOB - 100% High
//							OC3R				= 0;
//							OC3RS				= PERIOD+1;
//
//                            //LOA - 100% Low
//							OC1R				= PERIOD+1;
//                            OC1RS				= 0;
//                            
//							//LOB - 100% Low
//							OC4R				= PERIOD+1;
//                            OC4RS				= 0;
//							
//							break;
//						#endif
//						#ifdef HiVolt_INVERTER_ENABLED
//						case HIGH_VOLTAGE:
//							//HOA - 100% High
//							OC7R				= 0;
//							OC7RS				= PERIOD+1;
//
//							//HOB - 100% High
//							OC8R				= 0;
//							OC8RS				= PERIOD+1;
//
//							//LOB - 100% Low
//							OC9RS				= 0;
//							OC9R				= PERIOD+1;
//
//							//LOA - 100% Low
//							OC6RS				= 0;
//							OC6R				= PERIOD+1;
//
//							break;
//						#endif
//						default:
//							break;
//					}
//					InverterConfigData[currentInverter].currentStep = 0;
//					InverterConfigData[currentInverter].phaseRange = SINE_0_TO_90;
//					break;
//			}
//		}
//	}

    return;
}

int Converter_Time_To_Angle(int currentInverter, unsigned int currentTime, unsigned int outputPeriod)
{
    return (long)(currentTime * 3600) / outputPeriod;
}

void Over_Current_Watch(int currentInverter)
{
    int measuredCurrent;

    //Retrieve the circuit currents
    switch(currentInverter)
    {
        case HIGH_CURRENT_INVERTER:
            measuredCurrent = A2D_Value(A2D_AN9_INPUT_CURRENT);
            break;
        case HIGH_VOLTAGE_INVERTER:
            measuredCurrent = A2D_Value(A2D_AN10_OUTPUT_CURRENT);
            break;
        default:
            //Put in terminal window error logging
            break;
    }

    //Check for an over-current condition
    if(measuredCurrent >= InverterConfigData[currentInverter].maxCurrentTripPickup_mA)
    {
        inverterErrorFlags[currentInverter].overCurrent = 1;
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
		case HIGH_CURRENT_INVERTER:
			//LOA - Low for entire Period
			OC1R				= PERIOD+1;
            OC1RS				= 0;

			//HOA - High for entire Period
			OC2R				= 0;
			OC2RS				= PERIOD+1;

			//HOB - Firing (circulating current)
			OC3R				= DEADBAND;
			OC3RS				= circulatingCurrentPeriod;

			//LOB - Firing (conducting current)
			OC4R				= conductingCurrentPeriod;
			OC4RS				= PERIOD;

			break;
		#endif
		
		#ifdef HiVolt_INVERTER_ENABLED
		case HIGH_VOLTAGE_INVERTER:
			//LOA - 100% Low
			OC6R				= PERIOD+1;
            OC6RS				= 0;

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

void Negative_Sine(int step, enum INVERTERS_SUPPORTED inverter)
{
	int circulatingCurrentPeriod;
	int conductingCurrentPeriod;
	
	Calculate_Sine_Wave(step, inverter, &conductingCurrentPeriod, &circulatingCurrentPeriod);

	switch(inverter)
	{
		#ifdef HiI_INVERTER_ENABLED
		case HIGH_CURRENT_INVERTER:
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
		#ifdef HiVolt_INVERTER_ENABLED
		case HIGH_VOLTAGE_INVERTER:
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
	//Precalculate Duty Cycle Periods
	*circulatingCurrentPeriod	= PERIOD - (inverterOnPeriod[step]*InverterConfigData[inverter].multiplier)/InverterConfigData[inverter].divider;
	*conductingCurrentPeriod	= (inverterOnPeriod[step]*InverterConfigData[inverter].multiplier)/InverterConfigData[inverter].divider - DEADBAND;
	
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

//Get & Set Functions for Global Variables

void Set_Output_Hz(int newFrequency_Hz, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].targetOutputPeriod_cycles = FCY_Hz / newFrequency_Hz;
	return;
}

int Get_Output_Hz(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].targetOutputPeriod_cycles;
}

void Set_Rated_Hz (int newFrequency_Hz, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].ratedOutputPeriod_cycles = FCY_Hz / newFrequency_Hz;
	return;
}

int Get_Rated_Hz(enum INVERTERS_SUPPORTED inverter)
{
	return FCY_Hz / InverterConfigData[inverter].ratedOutputPeriod_cycles;
}

void Set_Rated_RMS_Voltage (int newRatedVoltage_Vx10, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].targetOutputVoltage_Vx10 = newRatedVoltage_Vx10 * 1414 / 1000;
	return;
}

int Get_Rated_RMS_Voltage (enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].targetOutputVoltage_Vx10 * 1000 / 1414;
}

void Peaks(enum INVERTERS_SUPPORTED inverter)
{
	int currentVoltage;
	
	switch(inverter)
	{
		#ifdef HiI_INVERTER_ENABLED
		case HIGH_CURRENT_INVERTER:
			//Take a sample; it won't give me a result for THIS calculation, but will be ready by the next one
			Trigger_A2D_Scan();

			//Check if the voltage needs to be adjusted
			currentVoltage = A2D_Value(A2D_AN12_VDC_BUS_PLUS);
			break;
		#endif
		#ifdef HiVolt_INVERTER_ENABLED
		case HIGH_VOLTAGE_INVERTER:
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
		InverterConfigData[inverter].multiplier++;
	else if(currentVoltage > (targetVoltage[inverter] + VOLTAGE_TARGET_DEADBAND))//Voltage is too high
		InverterConfigData[inverter].multiplier--;

	//Apply caps to multiplier as required
	if(InverterConfigData[inverter].multiplier > MULTIPLIER_MAXIMUM)
		InverterConfigData[inverter].multiplier = MULTIPLIER_MAXIMUM;
	else if(InverterConfigData[inverter].multiplier <= 0)
		InverterConfigData[inverter].multiplier = 1;
	
	return;
}

void Set_Rated_Voltage(int newTarget, enum INVERTERS_SUPPORTED inverter)
{
	if((inverter >= 0) && (inverter < NUMBER_OF_INVERTERS_SUPPORTED))
		targetVoltage[inverter] = newTarget;

	return;
}

int Get_Voltage_Target(enum INVERTERS_SUPPORTED inverter)
{
	return targetVoltage[inverter];
}

void Frequency_Ramp(unsigned long time_mS)
{
	static int frequency = 20;

	#ifdef HiI_INVERTER_ENABLED
	Set_Frequency_Hz(frequency++,		HIGH_CURRENT_INVERTER);
	Set_Voltage_Target(frequency*28,	HIGH_CURRENT_INVERTER);	//Voltage is frequency *2 * 2^0.5, hence 1.41*2 become 28 in interger math
	#endif
	#ifdef HiVolt_INVERTER_ENABLED
	Set_Frequency_Hz(60/*Hz*/,			HIGH_VOLTAGE_INVERTER);
	Set_Voltage_Target(1697/*169.7V*/,	HIGH_VOLTAGE_INVERTER);	//Peak voltage of a 120Vrms sine wave
	#endif

	return;
}

