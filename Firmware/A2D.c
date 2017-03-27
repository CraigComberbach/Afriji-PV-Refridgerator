/**************************************************************************************************
Target Hardware:		PIC24F
Chip resources used:	A2D Module & Pins setup as analog inputs
Purpose:				Scan A2D, perform DSP to increase resolution, and format accordingly
						It also allows for arbitrary functionality through function pointers to further extend the ADCs capabilities

Version History:
v1.1.0	2015-04-19  Craig Comberbach
	Compiler: XC16 v1.11	IDE: MPLABx 2.20	Tool: ICD3	Computer: Intel Core2 Quad CPU 2.40 GHz, 5 GB RAM, Windows 7 64 bit Home Premium SP1
	Added fine grained sampling - Allows the number of samples to be changed when the burst sampling occurs. Allows from 1-16 samples, default is 16
	Added sample inspection - Allows individual samples to be inspected after a burst completes. This allows manipulation and inspection of the samples to occur if necessary
v1.0.0	2015-01-10  Craig Comberbach
	Compiler: XC16 v1.11	IDE: MPLABx 2.20	Tool: ICD3	Computer: Intel Core2 Quad CPU 2.40 GHz, 5 GB RAM, Windows 7 64 bit Home Premium SP1
	Added full scanning functionality
	Added format/pre/post/finished function pointers to extend capabilities to encompass non-generic situations
	Added DSP based resolution increasing capabilities (As much as 16-bit can be obtained from the 10-bit A2D module)
v0.0.0	2013-07-18  Craig Comberbach
	Compiler: C30 v3.31	IDE: MPLABx 1.80	Tool: RealICE	Computer: Intel Xeon CPU 3.07 GHz, 6 GB RAM, Windows 7 64 bit Professional SP1
	First version
 **************************************************************************************************/
/*************    Header Files    ***************/
#include "Config.h"
#include "A2D.h"

/************* Semantic Versioning***************/
/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
#define	NUMBER_OF_CHANNELS	16
#define SCAN_BUFFER_SIZE	16	//Size of the scan buffer

/*************    Enumeration     ***************/
/*************ArbitraryFunctionality*************/
#define MAX_SCAN_QUEUE_SIZE	16	//Max size of the scan queue

/************* Module Definitions ***************/
#define	STOP_SCAN		AD1CON1bits.ASAM=0	//Stops the scanning of channels
#define	START_SCAN		AD1CON1bits.ASAM=1	//Starts the scanning of channels

/************* Other  Definitions ***************/
/*************  Global Variables  ***************/
volatile short int scanningQueue[MAX_SCAN_QUEUE_SIZE]; //The list of channels to scan, as well as the order to scan them in
volatile short int scanIsComplete;
volatile short int currentQueueElement;
struct A2D_Channel_Attributes
{
	unsigned short int  bitsOfResolutionIncrease;					//The number of bit of increased resolution (Default is 0 which is 10 bits)
	unsigned int samplesRequired;									//The number of samples required to initate an averaging event (includes number of finished samples to make a final averaged sample)
	unsigned int valueForDSP;										//Used as part of calculation to get the current value, includes what is required for resolution increasing as well as averaging
	unsigned int samplesTaken;										//Current number of samples, in multiples of 16 (because samples are taken in bursts of 16)
	int value;														//The most current averaged value, includes resolution increase if used
	int (*formatFunction)(int);										//Used to specify a function that handles the formating of the averaged value
	unsigned long sumOfSamples;										//Sum of all the A2D samples before it undergoes DSP/Averaging
	int autoConversion;												//0 means auto-convert, 1 means external trigger convert
} A2D_Channel[NUMBER_OF_CHANNELS];

/*************Function  Prototypes***************/
void __attribute__((__interrupt__, auto_psv)) _ADC1Interrupt(void);

void A2D_Routine(uint32_t time_mS)
{
	int channel;

	if(scanIsComplete)
	{
		//Reset for next time
		scanIsComplete = 0;

		//Add the sample to the sum of the samples in the raw variable
		A2D_Channel[0].sumOfSamples += ADC1BUF0;	//A2D_AN0_TRANSFORMER_PRIMARY_MINUS
		A2D_Channel[1].sumOfSamples += ADC1BUF1;	//A2D_AN1_TRANSFORMER_PRIMARY_PLUS
		A2D_Channel[2].sumOfSamples += ADC1BUF2;	//A2D_AN2_SOLAR_PLUS
		A2D_Channel[3].sumOfSamples += ADC1BUF3;	//A2D_AN3_TEMP2
//		A2D_Channel[4].sumOfSamples += ;			//A2D_AN4_UNUSED
//		A2D_Channel[5].sumOfSamples += ;			//A2D_AN5_UNUSED
		A2D_Channel[6].sumOfSamples += ADC1BUF4;	//A2D_AN6_TEMP1
		A2D_Channel[7].sumOfSamples += ADC1BUF5;	//A2D_AN7_TEMP3
		A2D_Channel[8].sumOfSamples += ADC1BUF6;	//A2D_AN8_TEMP4
		A2D_Channel[9].sumOfSamples += ADC1BUF7;	//A2D_AN9_INPUT_CURRENT
		A2D_Channel[10].sumOfSamples += ADC1BUF8;	//A2D_AN10_OUTPUT_CURRENT
		A2D_Channel[11].sumOfSamples += ADC1BUF9;	//A2D_AN11_TEMP5
		A2D_Channel[12].sumOfSamples += ADC1BUFA;	//A2D_AN12_VDC_BUS_PLUS
		A2D_Channel[13].sumOfSamples += ADC1BUFB;	//A2D_AN13_TRANSFORMER_SECONDARY_PLUS
		A2D_Channel[14].sumOfSamples += ADC1BUFC;	//A2D_AN14_VOUT_PLUS
		A2D_Channel[15].sumOfSamples += ADC1BUFD;	//A2D_AN15_VOUT_MINUS

		//Increment the number of samples read in
		A2D_Channel[0].samplesTaken++;	//A2D_AN0_TRANSFORMER_PRIMARY_MINUS
		A2D_Channel[1].samplesTaken++;	//A2D_AN1_TRANSFORMER_PRIMARY_PLUS
		A2D_Channel[2].samplesTaken++;	//A2D_AN2_SOLAR_PLUS
		A2D_Channel[3].samplesTaken++;	//A2D_AN3_TEMP2
//		A2D_Channel[4].samplesTaken++;	//A2D_AN4_UNUSED
//		A2D_Channel[5].samplesTaken++;	//A2D_AN5_UNUSED
		A2D_Channel[6].samplesTaken++;	//A2D_AN6_TEMP1
		A2D_Channel[7].samplesTaken++;	//A2D_AN7_TEMP3
		A2D_Channel[8].samplesTaken++;	//A2D_AN8_TEMP4
		A2D_Channel[9].samplesTaken++;	//A2D_AN9_INPUT_CURRENT
		A2D_Channel[10].samplesTaken++;	//A2D_AN10_OUTPUT_CURRENT
		A2D_Channel[11].samplesTaken++;	//A2D_AN11_TEMP5
		A2D_Channel[12].samplesTaken++;	//A2D_AN12_VDC_BUS_PLUS
		A2D_Channel[13].samplesTaken++;	//A2D_AN13_TRANSFORMER_SECONDARY_PLUS
		A2D_Channel[14].samplesTaken++;	//A2D_AN14_VOUT_PLUS
		A2D_Channel[15].samplesTaken++;	//A2D_AN15_VOUT_MINUS

		//Check if we are ready for DSP/averaging
		for(channel = 0; channel < NUMBER_OF_CHANNELS; channel++)
		{
			if(A2D_Channel[channel].samplesTaken >= A2D_Channel[channel].samplesRequired)
			{
				//Perform the DSP/averaging
				A2D_Channel[channel].sumOfSamples /= A2D_Channel[channel].valueForDSP; //Create average DSP value

				//Apply formats externally
				A2D_Channel[channel].value = A2D_Channel[channel].formatFunction((int)A2D_Channel[channel].sumOfSamples);

				//House keeping - Reset the counter and storage variable
				A2D_Channel[channel].samplesTaken = 0;
				A2D_Channel[channel].sumOfSamples = 0;
			}
		}
	}

	return;
}

void Trigger_A2D_Scan(void)
{
	START_SCAN;
	return;
}

void __attribute__((__interrupt__, auto_psv)) _ADC1Interrupt(void)
{
	//Clear interrupt flag
	IFS0bits.AD1IF = 0;

	//Temporarily turn off automatic scanning until we figure out what to do with our current samples
	STOP_SCAN;

	//Let the A2D routine know that we have finished
	scanIsComplete = 1;

	return;
}

void A2D_Initialize(void)
{
	//AD1 Interrupt
	IFS0bits.AD1IF = 0;				//0 = Interrupt request has not occurred
	IEC0bits.AD1IE = 1;				//1 = Interrupt request is enabled

	//A/D Input Scan Select Register (Low)
	AD1CSSL = 0b1111001111111111;	//0 = Analog channel omitted from input scan

	//A/D Port Configuration Register
	AD1PCFGL = 0b0000110000000000;	//0= Pin configured in Analog mode; I/O port read disabled, A/D samples pin voltage
//	AD1PCFGH = 0;

	//A/D Input Select Register
	AD1CHSbits.CH0SA = 0b0000;		//0000 = AVDD
	AD1CHSbits.CH0NA = 0;			//0 = Channel 0 negative input is VR-
	AD1CHSbits.CH0SB = 0b0000;		//0000 = AVDD
	AD1CHSbits.CH0NB = 0;			//0 = Channel 0 negative input is VR-

	//A/D Control Register 3
	#warning "The code to setup the A2D timing has not been configured"
	AD1CON3bits.ADCS = 0b111111;	//111111 = 64 * TCY
	AD1CON3bits.SAMC = 0b11111;		//11111 = 31 TAD
	AD1CON3bits.ADRC = 1;			//1 = A/D internal RC clock

	//A/D Control Register 2
	AD1CON2bits.ALTS = 0;			//0 = Always uses MUX A input multiplexer settings
	AD1CON2bits.BUFM = 0;			//0 = Buffer is configured as one 16-word buffer (ADC1BUFn<15:0>)
	AD1CON2bits.SMPI = 0b1101;		//1101 = Interrupts at the completion of conversion for each 14th sample/convert sequence
	//AD1CON2bits.BUFS is Read Only
	AD1CON2bits.CSCNA = 1;			//1 = Scan inputs
	AD1CON2bits.VCFG = 0b000;		//000 = Vr+ = AVDD, Vr- = AVSS

	//A/D Control Register 1
//	AD1CON1bits.DONE is Read Only
//	AD1CON1bits.SAMP is Read Only
	AD1CON1bits.ASAM = 0;		//0 = Sampling begins when SAMP bit is set
	AD1CON1bits.SSRC = 0b111;	//111 = Internal counter ends sampling and starts conversion (auto-convert)
	AD1CON1bits.FORM = 0b00;	//00 = Integer (0000 00dd dddd dddd)
	AD1CON1bits.ADSIDL = 1;		//1 = Discontinue module operation when device enters Idle mode
	AD1CON1bits.ADON = 1;		//1 = A/D Converter module is operating

	return;
}

int A2D_Channel_Settings(int channel, enum RESOLUTION desiredResolutionIncrease, int numberOfAverages, int (*formatFunction)(int))
{
	unsigned long samplesRequired;

	//Check if we are within a valid range of channels
	if((channel < 0) || (channel >= NUMBER_OF_CHANNELS))
		return 0;

	//Determine the number of samples required
	samplesRequired = numberOfAverages;
	switch(desiredResolutionIncrease)
	{
		case RESOLUTION_10_BIT:
			samplesRequired *= 1;//4^0
			break;
		case RESOLUTION_11_BIT:
			samplesRequired *= 4;//4^1
			break;
		case RESOLUTION_12_BIT:
			samplesRequired *= 16;//4^2
			break;
		case RESOLUTION_13_BIT:
			samplesRequired *= 64;//4^3
			break;
		case RESOLUTION_14_BIT:
			samplesRequired *= 256;//4^4
			break;
		case RESOLUTION_15_BIT:
			samplesRequired *= 1024;//4^5
			break;
		case RESOLUTION_16_BIT:
			samplesRequired *= 4096;//4^6
			break;
		default:
			return 0;
	}

	//Set values
	A2D_Channel[channel].value = 0;
	A2D_Channel[channel].sumOfSamples = 0;
	A2D_Channel[channel].bitsOfResolutionIncrease = desiredResolutionIncrease;
	A2D_Channel[channel].samplesRequired = (int)samplesRequired;
	A2D_Channel[channel].samplesTaken = 0;
	A2D_Channel[channel].formatFunction = formatFunction;
	
	//Calculate the DSP and averaging number
	A2D_Channel[channel].valueForDSP = numberOfAverages;
	switch(desiredResolutionIncrease)
	{
		case RESOLUTION_10_BIT:
			break;
		case RESOLUTION_11_BIT:
			A2D_Channel[channel].valueForDSP *= 2;//2^1;
			break;
		case RESOLUTION_12_BIT:
			A2D_Channel[channel].valueForDSP *= 4;//2^2;
			break;
		case RESOLUTION_13_BIT:
			A2D_Channel[channel].valueForDSP *= 8;//2^3;
			break;
		case RESOLUTION_14_BIT:
			A2D_Channel[channel].valueForDSP *= 16;//2^4;
			break;
		case RESOLUTION_15_BIT:
			A2D_Channel[channel].valueForDSP *= 32;//2^5;
			break;
		case RESOLUTION_16_BIT:
			A2D_Channel[channel].valueForDSP *= 64;//2^6;
			break;
	}

	//Success!
	return 1;
}

int A2D_Value(int channel)
{
	return A2D_Channel[channel].value; //Return the value - NOTE: it has already been formatted
}
