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
/*************	Header Files	***************/
#include "Config.h"
#include "Inverter.h"
#include "A2D.h"
#include "Pins.h"
#include "Scheduler.h"
#include "SineFunctionLookup.h"
#include "stdlib.h"

/*************    Mike & Micah    ***************/
const int InputStageFrequency = 60;
const int OutputStageFrequency = 20;
const int InputStageVp_x10 = 120; //Ensure DC rail does not exceed 200 VDC

/************* Library Definition ***************/
/************* Semantic Versioning***************/
/*************Library Dependencies***************/
/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
#define	PWM_PERIOD_CLOCK_CYCLES		159	//# of operations between PWM module resets
#define	DEADBAND					5	//Period resolution is 62.5nS; This is the delay between turning on/off one and turning on/off the other
#define NUMBER_OF_uS_IN_ONE_SECOND	1000000
#define VOLTAGE_TARGET_DEADBAND		10	//Voltage with one decimal of accuracy
#define ONE_HUNDRED_PERCENT			1000
#define THREE_HUNDRED_SIXTY_DEGREES	3600
#define ONE_HUNDRED_EIGHTY_DEGREES	1800
const uint16_t NOMINAL_DC_RAIL_VOLTAGE_Vx10	= 2400;

/*************	Enumeration	 ***************/
/***********	Flag Definitions	*************/
struct ERROR_FLAGS
{
	int overCurrent;
	int maxSlopeExceeded;
	int targetExceededSupply;
} inverterErrorFlags[NUMBER_OF_INVERTERS_SUPPORTED];

/***********	Structure Definitions	***********/
struct INVERTER_VARIABLES
{
	int targetOutputVoltage_Vx10;
	int targetOutputVoltageShadow_Vx10;
	int targetOutputFrequency_Hz;
	unsigned long targetOutputPeriod_uS;
	int ratedOutputVoltage_Vx10;
	int ratedOutputFrequency_Hz;
	int ratedOutputPeriod_us;
	unsigned long currentTime_uS;	//Time is referenced to the last zero degree crossing
	int angle_degx10;
	int maxCurrentTripPickup_Ax10;
	int maxCurrentTripDelay_us;
	int lastPeakPosCurrent_mA;
	int lastPeakNegCurrent_mA;
	int targetOutputPeriod_cycles;
	int ratedOutputPeriod_cycles;
	int32_t startupDelay_uS;
} InverterConfigData[NUMBER_OF_INVERTERS_SUPPORTED];
 
/***********State Machine Definitions************/
 /*************	Global Variables	**************/
int pwmPeriod_us;

/*************Interrupt Prototypes***************/
/*************Function  Prototypes***************/
int Over_Current_Watch(enum INVERTERS_SUPPORTED inverter);
void Positive_Sine(int step, enum INVERTERS_SUPPORTED inverter);
void Negative_Sine(int step, enum INVERTERS_SUPPORTED inverter);
int Calculate_Amplitude_Factor(enum INVERTERS_SUPPORTED currentInverter);
int Calculate_PWM_Duty_Percent(enum INVERTERS_SUPPORTED currentInverter, unsigned int a_percent);
unsigned int Converter_Time_To_Angle_x10(unsigned int currentTime, unsigned int outputPeriod);
void Update_PWM_Register(enum INVERTERS_SUPPORTED currentInverter, unsigned int theta, int dutyCyclePercent);

/************* Device Definitions ***************/	
/************* Module Definitions ***************/

void Initialize_Inverter(void)
{
	int loop;
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

	for(loop = 0; loop < NUMBER_OF_INVERTERS_SUPPORTED; ++loop)
	{
		InverterConfigData[loop].currentTime_uS = 0;
		
		#ifdef HiI_INVERTER_ENABLED
		if(loop == HIGH_CURRENT_INVERTER)
		{
			InverterConfigData[loop].startupDelay_uS = 0;
			InverterConfigData[loop].targetOutputVoltage_Vx10 = 120;
			InverterConfigData[loop].maxCurrentTripPickup_Ax10 = 800;
			InverterConfigData[loop].targetOutputFrequency_Hz = 60;
			InverterConfigData[loop].targetOutputPeriod_uS = NUMBER_OF_uS_IN_ONE_SECOND / InverterConfigData[loop].targetOutputFrequency_Hz;
		}
		#endif
		#ifdef HiV_INVERTER_ENABLED
		if(loop == HIGH_VOLTAGE_INVERTER)
		{
			InverterConfigData[loop].startupDelay_uS = 1000000;
			InverterConfigData[loop].targetOutputVoltage_Vx10 = 2000;
			InverterConfigData[loop].maxCurrentTripPickup_Ax10 = 100;
			InverterConfigData[loop].targetOutputFrequency_Hz = 16;
			InverterConfigData[loop].targetOutputPeriod_uS = NUMBER_OF_uS_IN_ONE_SECOND / InverterConfigData[loop].targetOutputFrequency_Hz;
		}
		#endif
	}
	
#ifdef HiI_INVERTER_ENABLED
	//OC1 - LOA Hi Current
	OC1RS				= 0;		//Ensures it is off until needed
	OC1R				= PWM_PERIOD_CLOCK_CYCLES+1;	//Ensures it is off until needed
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
	OC2R				= PWM_PERIOD_CLOCK_CYCLES+1;	//Ensures it is off until needed
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
	OC3R				= PWM_PERIOD_CLOCK_CYCLES+1;	//Ensures it is off until needed
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
	OC4R				= PWM_PERIOD_CLOCK_CYCLES+1;	//Ensures it is off until needed
	OC4CON1				= 0;
	OC4CON2				= 0;
	OC4CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC4CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC4CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC4CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC4CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC4CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin
#endif
	
#ifdef HiV_INVERTER_ENABLED
	//OC6 - LOA Hi Voltage
	OC6RS				= 0;		//Ensures it is off until needed
	OC6R				= PWM_PERIOD_CLOCK_CYCLES+1;	//Ensures it is off until needed
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
	OC7R				= PWM_PERIOD_CLOCK_CYCLES+1;	//Ensures it is off until needed
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
	OC8R				= PWM_PERIOD_CLOCK_CYCLES+1;	//Ensures it is off until needed
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
	OC9R				= PWM_PERIOD_CLOCK_CYCLES+1;	//Ensures it is off until needed
	OC9CON1				= 0;
	OC9CON2				= 0;
	OC9CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC9CON1bits.OCM		= 0b111;	//111 = Center-Aligned PWM mode on OC
	OC9CON2bits.SYNCSEL	= 5;		//00101 = Output Compare 5
	OC9CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC9CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC9CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin
#endif
	
	//OC5 - Master Timer 
	/* !!! One [reference timer] to rule them all and in the darkness bind them [together] !!! */
	OC5R = 0;
	OC5RS = PWM_PERIOD_CLOCK_CYCLES;
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
	enum INVERTERS_SUPPORTED currentInverter;
	int current_mA;
	unsigned int amplitudeFactor_Percentx10;
	int dutyCyclePercentx10;

	for(currentInverter = 0; currentInverter < NUMBER_OF_INVERTERS_SUPPORTED; ++currentInverter)
	{
		if(InverterConfigData[currentInverter].startupDelay_uS >= time_uS)
		{
			InverterConfigData[currentInverter].startupDelay_uS -= time_uS;
		}
		else
		{
			//Update Timer
			InverterConfigData[currentInverter].currentTime_uS += time_uS;

			//Convert Time to Angle
			InverterConfigData[currentInverter].angle_degx10 = Converter_Time_To_Angle_x10(InverterConfigData[currentInverter].currentTime_uS, InverterConfigData[currentInverter].targetOutputPeriod_uS);

			//Over-Current Detection
			current_mA = Over_Current_Watch(currentInverter);

			//Calculate Amplitude Factor
			amplitudeFactor_Percentx10 = Calculate_Amplitude_Factor(currentInverter);

			//Calculate PWM Duty Percent
			dutyCyclePercentx10 = Calculate_PWM_Duty_Percent(currentInverter, amplitudeFactor_Percentx10);

			//Update PWM Registers
			Update_PWM_Register(currentInverter, InverterConfigData[currentInverter].angle_degx10, dutyCyclePercentx10);

			//Once per cycle update the target frequency and voltage
			if((InverterConfigData[currentInverter].targetOutputPeriod_uS - InverterConfigData[currentInverter].currentTime_uS) < Get_Task_Period(INVERTER_TASK))
			{
			#ifdef HiI_INVERTER_ENABLED
				//SetThe new target voltage and frequency for the high current inverter
				if(currentInverter == HIGH_CURRENT_INVERTER)
				{
					InverterConfigData[HIGH_CURRENT_INVERTER].targetOutputFrequency_Hz = InputStageFrequency;
					InverterConfigData[HIGH_CURRENT_INVERTER].targetOutputVoltage_Vx10 = InputStageVp_x10;
				}
			#endif
			#ifdef HiV_INVERTER_ENABLED
				//SetThe new target voltage and frequency for the high voltage inverter
				if(currentInverter == HIGH_VOLTAGE_INVERTER)
				{
					InverterConfigData[HIGH_VOLTAGE_INVERTER].targetOutputFrequency_Hz = OutputStageFrequency;
					InverterConfigData[HIGH_VOLTAGE_INVERTER].targetOutputVoltage_Vx10 = (InverterConfigData[currentInverter].targetOutputFrequency_Hz * 2 + (60 - InverterConfigData[currentInverter].targetOutputFrequency_Hz) * 3 / 10) * 14;
				}
			#endif

				//Housekeeping
				Pin_Toggle(PIN_RG7_SWITCHED_GROUND2);	//For triggering purposes
				InverterConfigData[currentInverter].currentTime_uS = 0;
				InverterConfigData[currentInverter].targetOutputPeriod_uS = NUMBER_OF_uS_IN_ONE_SECOND / InverterConfigData[currentInverter].targetOutputFrequency_Hz;
			}
		}
	}

	return;
}

unsigned int Converter_Time_To_Angle_x10(unsigned int currentTime, unsigned int outputPeriod)
{
	/* 
	 Finds the ratio of the current time with the expected time and multiplies it by 360º to get the current angle 
	 Ex. 200uS since 0º, 360º occurs at 1000uS, then you should get 72º as your answer: (200u/1000u) * 360º 
	 */
	unsigned long int temp;
	temp = ((unsigned long int)currentTime * (unsigned long int)THREE_HUNDRED_SIXTY_DEGREES);
	temp /= (unsigned long int)outputPeriod;
	return (unsigned int)temp;
}

int Over_Current_Watch(enum INVERTERS_SUPPORTED currentInverter)
{
	/* 
	 Checks for an over current error condition and sets an error flag appropriately 
	 Each inverter is checked independently of the other, and each has its own error flag that  can be set 
	 */
	int measuredCurrent;

	//Retrieve the circuit currents
	switch(currentInverter)
	{
		#ifdef HiI_INVERTER_ENABLED
		case HIGH_CURRENT_INVERTER:
			measuredCurrent = A2D_Value(A2D_AN9_INPUT_CURRENT);
			break;
		#endif
		#ifdef HiV_INVERTER_ENABLED
		case HIGH_VOLTAGE_INVERTER:
			measuredCurrent = A2D_Value(A2D_AN10_OUTPUT_CURRENT);
			break;
		#endif
		default:
			//Put in terminal window error logging
			break;
	}
		inverterErrorFlags[currentInverter].overCurrent = 1;

	//Check for an over-current condition
	if(measuredCurrent >= InverterConfigData[currentInverter].maxCurrentTripPickup_Ax10)
	{
		inverterErrorFlags[currentInverter].overCurrent = 1;
	}

	return measuredCurrent;
}

int Calculate_Amplitude_Factor(enum INVERTERS_SUPPORTED currentInverter)
{
	long supplyVoltage_Vx10;
	long amplitudeFactor_percentx10;
	long gamma = Get_Task_Period(INVERTER_TASK);
	static long oldAmplitudeFactor_Percentx10 = 0;	//Alpha from last time through
		
	//Retrieve the circuit voltages
	switch(currentInverter)
	{	
		#ifdef HiI_INVERTER_ENABLED
		case HIGH_CURRENT_INVERTER:
			supplyVoltage_Vx10 = A2D_Value(A2D_AN2_SOLAR_PLUS);
			break;
		#endif
		#ifdef HiV_INVERTER_ENABLED
		case HIGH_VOLTAGE_INVERTER:
			supplyVoltage_Vx10 = A2D_Value(A2D_AN12_VDC_BUS_PLUS);
			break;
		#endif
		default:
			#ifdef TERMINAL_WINDOW_DEBUG_ENABLED
				//TODO - Add debug Terminal code
			#endif
			break;
	}

	//Divide by zero Sentinel
	if(supplyVoltage_Vx10 == 0)
	{
		supplyVoltage_Vx10 = 1;
	}

	//Check if we need to dump power
	if((currentInverter == HIGH_CURRENT_INVERTER) && (A2D_Value(A2D_AN12_VDC_BUS_PLUS) < NOMINAL_DC_RAIL_VOLTAGE_Vx10))
	{
		amplitudeFactor_percentx10 = ONE_HUNDRED_PERCENT;
	}
	else
	{
		//Alpha = T/S * 100%
		amplitudeFactor_percentx10 = (long)InverterConfigData[currentInverter].targetOutputVoltage_Vx10 * (long)ONE_HUNDRED_PERCENT;
		amplitudeFactor_percentx10 /= (long)supplyVoltage_Vx10;
	}
	

	//Check to see if we exceeded max slope
	if((((amplitudeFactor_percentx10 - oldAmplitudeFactor_Percentx10) * supplyVoltage_Vx10) / gamma) > InverterConfigData[currentInverter].targetOutputVoltage_Vx10)
	{
		inverterErrorFlags[currentInverter].maxSlopeExceeded = 1;
	}

	//Check if we are exceeding 100%
	if(amplitudeFactor_percentx10 > ONE_HUNDRED_PERCENT)
	{
		amplitudeFactor_percentx10 = ONE_HUNDRED_PERCENT;
		inverterErrorFlags[currentInverter].targetExceededSupply = 1;
	}
	
	oldAmplitudeFactor_Percentx10 = amplitudeFactor_percentx10;
	
	return (int)amplitudeFactor_percentx10;
}

int Calculate_PWM_Duty_Percent(enum INVERTERS_SUPPORTED currentInverter, unsigned int amplitudeFactor_percentx10)
{
	long int dutyCycle_Percentx10;

	//Variable Sentinels
	if(amplitudeFactor_percentx10 > ONE_HUNDRED_PERCENT)
	{
		#ifdef TERMINAL_WINDOW_DEBUG_ENABLED
			//TODO - Add debug Terminal code
		#endif
	}

	dutyCycle_Percentx10 = (long int)amplitudeFactor_percentx10 * (long int)Sine(InverterConfigData[currentInverter].angle_degx10);
	dutyCycle_Percentx10 /= (long int)1000;//Remove the bonus *1000 used to maintain integer resolution increase
	
	if(dutyCycle_Percentx10 < 0)
	{
		dutyCycle_Percentx10 *= -1;
	}
	
	if(dutyCycle_Percentx10 > ONE_HUNDRED_PERCENT)
	{
		dutyCycle_Percentx10 = ONE_HUNDRED_PERCENT;
	}

	return (int)dutyCycle_Percentx10;
}

void Update_PWM_Register(enum INVERTERS_SUPPORTED currentInverter, unsigned int theta_degx10, int dutyCyclePercentx10)
{
	//Sign	++++++++++++...------------
	//HOA	------------...__/-\_______
	//LOA	____________...\_____/-----
	//HOB	__/-\_______...------------
	//LOB	\_____/-----...____________
//	Limits		5					|	144	
//	Angle		0-180º	180-360º	|	0-180º	180-360º
//	HOA			N/A		5/149		|	N/A		5/10
//	LOA			N/A		154/159		|	N/A	15/159
//	HOB			5/149	N/A			|	5/10	N/A
//	LOB			154/159	N/A			|	15/159	N/A
//	Conducting	154					|	15	
//	Circulating	149					|	10	
//	This is why we have a 3x deadband upper limit!

	unsigned long onTime;
	unsigned int circulatingCurrentPeriod;
	unsigned int conductingCurrentPeriod;

	//Variable Sentinels
	if(theta_degx10 >= THREE_HUNDRED_SIXTY_DEGREES)
	{
		theta_degx10 %= THREE_HUNDRED_SIXTY_DEGREES;
	}

	//Calculate On Time
	onTime = PWM_PERIOD_CLOCK_CYCLES;
	onTime *= dutyCyclePercentx10;
	onTime /= ONE_HUNDRED_PERCENT;
	if(onTime > (PWM_PERIOD_CLOCK_CYCLES - (3 * DEADBAND)))//Check to see if we have exceeded a maximum duty cycle
	{
		onTime = (PWM_PERIOD_CLOCK_CYCLES - (3 * DEADBAND));
		#ifdef TERMINAL_WINDOW_DEBUG_ENABLED
			//TODO - Add debug Terminal code
		#endif
	}
	else if(onTime < DEADBAND)//Check to see if we have exceeded a minimum duty cycle
	{
		onTime = DEADBAND;
		#ifdef TERMINAL_WINDOW_DEBUG_ENABLED
			//TODO - Add debug Terminal code
		#endif
	}

	//Calculate Conducting Period
	conductingCurrentPeriod = PWM_PERIOD_CLOCK_CYCLES - (unsigned int)onTime;

	//Calculate Circulating Period
	circulatingCurrentPeriod = PWM_PERIOD_CLOCK_CYCLES - ((unsigned int)onTime + DEADBAND);

	//Set registers
	if(theta_degx10 < ONE_HUNDRED_EIGHTY_DEGREES)
	{
		//Set default values for positive waveform
		switch(currentInverter)
		{
			#ifdef HiI_INVERTER_ENABLED
			case HIGH_CURRENT_INVERTER:
				//LOA - Low for entire Period
				OC1R	= PWM_PERIOD_CLOCK_CYCLES+1;
				OC1RS	= 0;

				//HOA - High for entire Period
				OC2R	= 0;
				OC2RS	= PWM_PERIOD_CLOCK_CYCLES+1;

				//HOB - Firing (circulating current)
				OC3R	= DEADBAND;
				OC3RS	= circulatingCurrentPeriod;

				//LOB - Firing (conducting current)
				OC4R	= conductingCurrentPeriod;
				OC4RS	= PWM_PERIOD_CLOCK_CYCLES;

				break;
			#endif

			#ifdef HiV_INVERTER_ENABLED
			case HIGH_VOLTAGE_INVERTER:
				//LOA - 100% Low
				OC6R	= PWM_PERIOD_CLOCK_CYCLES+1;
				OC6RS	= 0;

				//HOA - 100% High
				OC7R	= 0;
				OC7RS	= PWM_PERIOD_CLOCK_CYCLES+1;

				//HOB - Circulating current
				OC8R	= DEADBAND;
				OC8RS	= circulatingCurrentPeriod;

				//LOB - Conducting current
				OC9R	= conductingCurrentPeriod;
				OC9RS	= PWM_PERIOD_CLOCK_CYCLES;

				break;
			#endif

			default:
				break;
		}
	}
	else
	{
		//Set default values for negative waveform
		switch(currentInverter)
		{
			#ifdef HiI_INVERTER_ENABLED
			case HIGH_CURRENT_INVERTER:
				//HOA - Circulating Current
				OC2R	= DEADBAND;
				OC2RS	= circulatingCurrentPeriod;

				//LOA - Conducting Current
				OC1R	= conductingCurrentPeriod;
				OC1RS	= PWM_PERIOD_CLOCK_CYCLES;

				//LOB - 100% Low
				OC4R	= PWM_PERIOD_CLOCK_CYCLES+1;
				OC4RS	= 0;

				//HOB - 100% High
				OC3R	= 0;
				OC3RS	= PWM_PERIOD_CLOCK_CYCLES+1;

				break;
			#endif

			#ifdef HiV_INVERTER_ENABLED
			case HIGH_VOLTAGE_INVERTER:
				//HOA - Circulating Current
				OC7R	= DEADBAND;
				OC7RS	= circulatingCurrentPeriod;

				//LOA - Conducting Current
				OC6R	= conductingCurrentPeriod;
				OC6RS	= PWM_PERIOD_CLOCK_CYCLES;

				//LOB - 100% Low
				OC9R	= PWM_PERIOD_CLOCK_CYCLES+1;
				OC9RS	= 0;

				//HOB - 100% High
				OC8R	= 0;
				OC8RS	= PWM_PERIOD_CLOCK_CYCLES+1;

				break;
			#endif
			default:
				break;
		}
	}

	return;
}

void Frequency_Ramp(unsigned long time_mS)
{
	OutputStageFrequency++;
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
	InverterConfigData[inverter].targetOutputVoltage_Vx10 = (newRatedVoltage_Vx10 * 1414) / 1000;
	return;
}

int Get_Rated_RMS_Voltage (enum INVERTERS_SUPPORTED inverter)
{
	long temp;
	temp = InverterConfigData[inverter].targetOutputVoltage_Vx10 * 1000;
	temp /= 1414;
	return (int)temp;
}

void Set_Rated_Voltage(int newTarget, enum INVERTERS_SUPPORTED inverter)
{
	if((inverter >= 0) && (inverter < NUMBER_OF_INVERTERS_SUPPORTED))
	{
		InverterConfigData[inverter].targetOutputVoltageShadow_Vx10 = newTarget;
	}

	return;
}

int Get_Voltage_Target(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].targetOutputVoltage_Vx10;
}
 
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
	InverterConfigData[inverter].targetOutputFrequency_Hz = input;
}
 
int Get_Target_Output_Frequency_Hz(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].targetOutputFrequency_Hz;
}
 
int Get_Target_Output_Period_us (enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].targetOutputPeriod_uS;
}

void Set_Target_Output_Period_us(unsigned long value, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].targetOutputPeriod_uS = value;
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
	InverterConfigData[inverter].maxCurrentTripPickup_Ax10 = input;
}
 
int Get_Max_Current_Trip_Pickup_mA(enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].maxCurrentTripPickup_Ax10;
}
 
void Set_Max_Current_Trip_Delay_ms (int input, enum INVERTERS_SUPPORTED inverter)
{
	InverterConfigData[inverter].maxCurrentTripDelay_us = input;
}
 
int Get_Max_Current_Trip_Delay_ms (enum INVERTERS_SUPPORTED inverter)
{
	return InverterConfigData[inverter].maxCurrentTripDelay_us;
}

void Set_PWM_Period_us(int input)
{
	pwmPeriod_us = input;
}
 
int Get_PWM_Period_us(void)
{
	return pwmPeriod_us;
}
