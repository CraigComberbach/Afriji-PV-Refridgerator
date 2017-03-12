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

/************* Library Definition ***************/
/************* Semantic Versioning***************/
/*************Library Dependencies***************/
/************Arbitrary Functionality*************/
//#define	OC1RS = PERIOD;
//#define	OC1R = PERIOD / 2;	//Duty Cycle
//#define	OC2RS = PERIOD - 2;
//#define	OC2R
/*************   Magic  Numbers   ***************/
#define	PERIOD	1599
/*************    Enumeration     ***************/
/***********  Structure Definitions  ************/
/***********State Machine Definitions************/
/*************  Global Variables  ***************/
unsigned int inverterLevel[] =
{
//100kHz @ 1024 points
//  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,
//  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  6,  6,  6,  6,  6,  6,
//  6,  6,  6,  6,  6,  8,  8,  8,  8,  8,  8,  8,  8,  8, 10, 10, 10, 10, 10, 10, 10, 10, 10, 12, 12, 12, 12, 12, 12, 12, 14, 14,
// 14, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 18, 18, 18, 18, 18, 18, 20, 20, 20, 20, 20, 20, 22, 22, 22, 22, 22, 22, 24,
// 24, 24, 24, 24, 24, 26, 26, 26, 26, 26, 26, 28, 28, 28, 28, 28, 30, 30, 30, 30, 30, 32, 32, 32, 32, 32, 34, 34, 34, 34, 34, 36,
// 36, 36, 36, 36, 38, 38, 38, 38, 38, 40, 40, 40, 40, 40, 42, 42, 42, 42, 44, 44, 44, 44, 44, 46, 46, 46, 46, 46, 48, 48, 48, 48,
// 50, 50, 50, 50, 50, 52, 52, 52, 52, 54, 54, 54, 54, 56, 56, 56, 56, 56, 58, 58, 58, 58, 60, 60, 60, 60, 62, 62, 62, 62, 64, 64,
// 64, 64, 66, 66, 66, 66, 66, 68, 68, 68, 68, 70, 70, 70, 70, 72, 72, 72, 72, 74, 74, 74, 74, 76, 76, 76, 76, 78, 78, 78, 78, 81,
// 81, 81, 81, 81, 81, 83, 83, 83, 83, 85, 85, 85, 85, 87, 87, 87, 87, 89, 89, 89, 89, 91, 91, 91, 91, 93, 93, 93, 93, 93, 95, 95,
// 95, 95, 97, 97, 97, 97, 99, 99, 99, 99,101,101,101,101,103,103,103,103,103,105,105,105,105,107,107,107,107,109,109,109,109,109,
//111,111,111,111,113,113,113,113,113,115,115,115,115,115,117,117,117,117,119,119,119,119,119,121,121,121,121,121,123,123,123,123,
//123,125,125,125,125,125,127,127,127,127,127,129,129,129,129,129,131,131,131,131,131,133,133,133,133,133,133,135,135,135,135,135,
//135,137,137,137,137,137,137,139,139,139,139,139,139,141,141,141,141,141,141,143,143,143,143,143,143,143,145,145,145,145,145,145,
//145,145,147,147,147,147,147,147,147,149,149,149,149,149,149,149,149,149,151,151,151,151,151,151,151,151,151,153,153,153,153,153,
//153,153,153,153,153,153,155,155,155,155,155,155,155,155,155,155,155,155,155,157,157,157,157,157,157,157,157,157,157,157,157,157,
//157,157,157,157,157,157,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,
//158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,157,157,157,157,157,157,
//157,157,157,157,157,157,157,157,157,157,157,157,157,155,155,155,155,155,155,155,155,155,155,155,155,155,153,153,153,153,153,153,
//153,153,153,153,153,151,151,151,151,151,151,151,151,151,151,149,149,149,149,149,149,149,149,147,147,147,147,147,147,147,147,145,
//145,145,145,145,145,145,143,143,143,143,143,143,143,141,141,141,141,141,141,141,139,139,139,139,139,139,137,137,137,137,137,137,
//135,135,135,135,135,133,133,133,133,133,133,131,131,131,131,131,129,129,129,129,129,129,127,127,127,127,127,125,125,125,125,125,
//123,123,123,123,123,121,121,121,121,121,119,119,119,119,117,117,117,117,117,115,115,115,115,115,113,113,113,113,111,111,111,111,
//111,109,109,109,109,107,107,107,107,105,105,105,105,105,103,103,103,103,101,101,101,101, 99, 99, 99, 99, 99, 97, 97, 97, 97, 95,
// 95, 95, 95, 93, 93, 93, 93, 91, 91, 91, 91, 89, 89, 89, 89, 87, 87, 87, 87, 87, 85, 85, 85, 85, 83, 83, 83, 83, 81, 81, 81, 81,
// 78, 78, 78, 78, 76, 76, 76, 76, 74, 74, 74, 74, 72, 72, 72, 72, 72, 70, 70, 70, 70, 68, 68, 68, 68, 66, 66, 66, 66, 64, 64, 64,
// 64, 62, 62, 62, 62, 60, 60, 60, 60, 60, 58, 58, 58, 58, 56, 56, 56, 56, 54, 54, 54, 54, 54, 52, 52, 52, 52, 50, 50, 50, 50, 48,
// 48, 48, 48, 48, 46, 46, 46, 46, 44, 44, 44, 44, 44, 42, 42, 42, 42, 42, 40, 40, 40, 40, 38, 38, 38, 38, 38, 36, 36, 36, 36, 36,
// 34, 34, 34, 34, 34, 32, 32, 32, 32, 32, 30, 30, 30, 30, 30, 30, 28, 28, 28, 28, 28, 26, 26, 26, 26, 26, 26, 24, 24, 24, 24, 24,
// 22, 22, 22, 22, 22, 22, 20, 20, 20, 20, 20, 20, 18, 18, 18, 18, 18, 18, 18, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 14, 14, 14,
// 14, 12, 12, 12, 12, 12, 12, 12, 12, 10, 10, 10, 10, 10, 10, 10, 10,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  6,  6,  6,  6,  6,
//  6,  6,  6,  6,  6,  6,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
//  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0
//10kHz @ 180 points
27,55,83,110,138,166,193,221,248,276,
303,330,357,384,411,438,464,491,517,543,
569,595,621,646,671,697,721,746,770,795,
818,842,865,889,911,934,956,978,1000,1022,
1043,1063,1084,1104,1124,1143,1162,1181,1199,1218,
1235,1252,1269,1286,1302,1318,1333,1348,1362,1376,
1390,1403,1416,1429,1441,1452,1463,1474,1484,1494,
1503,1512,1520,1528,1535,1542,1549,1555,1560,1565,
1570,1574,1578,1581,1583,1586,1587,1589,1589,1590,
1589,1589,1587,1586,1583,1581,1578,1574,1570,1565,
1560,1555,1549,1542,1535,1528,1520,1512,1503,1494,
1484,1474,1463,1452,1441,1429,1416,1403,1390,1376,
1362,1348,1333,1318,1302,1286,1269,1252,1235,1218,
1199,1181,1162,1143,1124,1104,1084,1063,1043,1022,
1000,978,956,934,911,889,865,842,818,795,
770,746,721,697,671,646,621,595,569,543,
517,491,464,438,411,384,357,330,303,276,
248,221,193,166,138,110,83,55,27,0
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

void Initialize_Inverter(void)
{
	//OC5 - Ct
	OC5RS = PERIOD;
	OC5R = 0;
	OC5CON1				= 0;
	OC5CON2				= 0;
	OC5CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC5CON1bits.OCM		= 0b110;	//110 = Edge-Aligned PWM mode on OCx
	OC5CON2bits.SYNCSEL	= 0b11111;	//11111 = This OC module
	OC5CON2bits.OCINV	= 1;		//0 = OCx output is not inverted
	OC5CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC5CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC6 - Dt
	OC6RS = PERIOD;
	OC6R = 0;
	OC6CON1				= 0;
	OC6CON2				= 0;
	OC6CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC6CON1bits.OCM		= 0b110;	//110 = Edge-Aligned PWM mode on OCx
	OC6CON2bits.SYNCSEL	= 0b00101;	//00101 = Output Compare 5
	OC6CON2bits.OCINV	= 1;		//0 = OCx output is not inverted
	OC6CON2bits.OCTRIG	= 0;		//1 = Trigger OCx from source designated by SYNCSELx bits
	OC6CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC7 - Cb
	OC7RS = PERIOD;
	OC7R = 0;
	OC7CON1				= 0;
	OC7CON2				= 0;
	OC7CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC7CON1bits.OCM		= 0b101;	//101= Double Compare Continuous Pulse mode: initialize OCx pin low, toggle OCx state continuously on alternate matches of OCxR and OCxRS
	OC7CON2bits.SYNCSEL	= 0b00101;	//00101 = Output Compare 5
	OC7CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC7CON2bits.OCTRIG	= 0;		//0 = Synchronize OCx with source designated by SYNCSELx bits
	OC7CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	//OC8 - Db
	OC8RS = PERIOD;
	OC8R = 0;
	OC8CON1				= 0;
	OC8CON2				= 0;
	OC8CON1bits.OCTSEL	= 0b111;	//111 = Peripheral Clock (FCY)
	OC8CON1bits.OCM		= 0b101;	//101= Double Compare Continuous Pulse mode: initialize OCx pin low, toggle OCx state continuously on alternate matches of OCxR and OCxRS
	OC8CON2bits.SYNCSEL	= 0b00101;	//00101 = Output Compare 5
	OC8CON2bits.OCINV	= 0;		//0 = OCx output is not inverted
	OC8CON2bits.OCTRIG	= 0;		//1 = Trigger OCx from source designated by SYNCSELx bits
	OC8CON2bits.OCTRIS	= 0;		//0 = Output Compare Peripheral x connected to the OCx pin

	return;
}

void Inverter_Routine(unsigned long time_mS)
{
	static int currentStep = 0;

	if(currentStep < 180)
		Positive_Wave(currentStep);
	else
		Negative_Wave(currentStep-180);

	++currentStep;
	if(currentStep >= 360)
		currentStep = 0;

    //Assign it to the registers
//	OC5RS = PERIOD;
//	OC5R = PERIOD / 2;	//Duty Cycle
//	OC6RS = PERIOD - 2;
//	OC6R = PERIOD - 6;

    return;
}

void Positive_Wave(int currentStep)
{
	//Generate Positive Sine Wave
	OC5R = inverterLevel[currentStep];	//OC5 - Ct
	OC7R = inverterLevel[currentStep];	//OC7 - Cb

	//Generate Reset
	OC6R = 0;	//OC6 - Dt
	OC8R = PERIOD - 6;//OC8 - Db (Start of Recharge)
	OC8RS = PERIOD - 2;//OC8 - Db (End of Recharge)
}

void Negative_Wave(int currentStep)
{
	//Generate Negative Sine Wave
	OC6R = inverterLevel[currentStep];	//OC6 - Dt
	OC8R = inverterLevel[currentStep];	//OC8 - Db

	//Generate Reset
	OC5R = 0;	//OC5 - Ct
	OC7R = PERIOD - 6;//OC7 - Cb (Start of Recharge)
	OC7RS = PERIOD - 2;//OC7 - Cb (End of Recharge)
}
