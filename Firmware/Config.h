#ifndef CONFIG_H
#define	CONFIG_H

//Hardware specific include file
#include <p24FJ256GA106.h>
#include <stdint.h>

//System Variables
//Clock Frequency
#define FOSC_Hz			32000000			//32MHz
#define FOSC_kHz		FOSC_Hz/1000		//32MHz
#define FOSC_MHz		FOSC_kHz/1000		//32MHz
#define FCY_Hz			FOSC_Hz/2			//16MHz
#define FCY_kHz			FCY_Hz/1000			//16MHz
#define FCY_MHz			FCY_kHz/1000		//16MHz
#define FCY_PERIOD_nS	1000000000/FCY_Hz	//250nS
#define FCY_PERIOD_pS	1000000000/FCY_kHz	//250000pS

//Compiled information
extern const char compiledOnDate[];
extern const char compiledAtTime[];
#define MAJOR_FIRMWARE_VERSION	0
#define MINOR_FIRMWARE_VERSION	1
#define PATCH_FIRMWARE_VERSION	0

//UART Modules
#define	UART_TERMINAL		1

//OCM Modules
#define OCM_BUZZER			1
#define OCM_RED_BACKLIGHT	4
#define OCM_GREEN_BACKLIGHT	5
#define OCM_BLUE_BACKLIGHT	6

enum SCHEDULER_DEFINITIONS
{
	STARTUP_TASK,
	HEART_BEAT_TASK,
	INVERTER_TASK,
	A2D_TASK,
	FREQUENCY_RAMP,
	NUMBER_OF_SCHEDULED_TASKS
};

enum INVERTERS_SUPPORTED
{
	HIGH_CURRENT,
	#ifdef DUAL_INVERTER
	HIGH_VOLTAGE,
	#endif
	NUMBER_OF_INVERTERS_SUPPORTED
};

//Pins
enum PIN_DEFINITIONS
{
	//PORTB
	PIN_RB0_TRANSFORMER_PRIMARY_MINUS,		//RB0
	PIN_RB1_TRANSFORMER_PRIMARY_PLUS,		//RB1
	PIN_RB2_VSOLAR_PLUS,					//RB2
	PIN_RB3_TEMP2,							//RB3
	PIN_RB4_PGD,							//RB4 - PGD
	PIN_RB5_PGC,							//RB5 - PGC
	PIN_RB6_TEMP1,							//RB6
	PIN_RB7_TEMP3,							//RB7
	PIN_RB8_TEMP4,							//RB8
	PIN_RB9_INPUT_CURRENT,					//RB9
	PIN_RB10_OUTPUT_CURRENT,				//RB10
	PIN_RB11_TEMP5,							//RB11
	PIN_RB12_VDC_BUS_PLUS,					//RB12
	PIN_RB13_TRANSFORMER_SECONDARY_PLUS,	//RB13
	PIN_RB14_VOUT_PLUS,						//RB14
	PIN_RB15_VOUT_MINUS,					//RB15
	//PORTC
	PIN_RC12,								//RC12 - Unused
	PIN_RC13,								//RC13 - Unused
	PIN_RC14,								//RC14 - Unused
	PIN_RC15,								//RC15 - Unused
	//PORTD
	PIN_RD0_HOB_HiA,						//RD0
	PIN_RD1_TERMINAL_TX,					//RD1
	PIN_RD2,								//RD2 - Unused
	PIN_RD3,								//RD3 - Unused
	PIN_RD4_LOB_HiA,						//RD4
	PIN_RD5_LOA_HiA,						//RD5
	PIN_RD6,								//RD6 - Unused
	PIN_RD7,								//RD7 - Unused
	PIN_RD8_HOA_HiA,						//RD8 - Unused
	PIN_RD9_GREEN_LED,						//RD9
	PIN_RD10_RED_LED,						//RD10
	PIN_RD11_BLUE_LED,						//RD11
	//PORTE
	PIN_RE0,								//RE0 - Unused
	PIN_RE1,								//RE1 - Unused
	PIN_RE2,								//RE2 - Unused
	PIN_RE3,								//RE3 - Unused
	PIN_RE4,								//RE4 - Unused
	PIN_RE5_SWITCHED_GROUND5,				//RE5
	PIN_RE6_SWITCHED_GROUND4,				//RE6
	PIN_RE7_SWITCHED_GROUND3,				//RE7
	//PORTF
	PIN_RF0,								//RF0 - Unused
	PIN_RF1,								//RF1 - Unused
	PIN_RF2_HOB_HiV,						//RF2 - Unused
	PIN_RF3_HOA_HiV,						//RF3 - Unused
	PIN_RF4_LOA_HiV,						//RF4
	PIN_RF5_LOB_HiV,						//RF5
	PIN_RF6,								//RF6 - Unused
	//PORTG
	PIN_RG2,								//RG2 - Unused
	PIN_RG3,								//RG3 - Unused
	PIN_RG6_SWITCHED_GROUND1,				//RG6
	PIN_RG7_SWITCHED_GROUND2,				//RG7
	PIN_RG8,								//RG8 - Unused
	PIN_RG9,								//RG9 - Unused
	NUMBER_OF_PINS
};

enum A2D_PIN_DEFINITIONS
{
	A2D_AN0_TRANSFORMER_PRIMARY_MINUS = 0,		//A0
	A2D_AN1_TRANSFORMER_PRIMARY_PLUS = 1,		//A1
	A2D_AN2_SOLAR_PLUS = 2,						//A2
	A2D_AN3_TEMP2 = 3,							//A3
	A2D_AN4_UNUSED = 4,							//A4
	A2D_AN5_UNUSED = 5,							//A5
	A2D_AN6_TEMP1 = 6,							//A6
	A2D_AN7_TEMP3 = 7,							//A7
	A2D_AN8_TEMP4 = 8,							//A8
	A2D_AN9_INPUT_CURRENT = 9,					//A9
	A2D_AN10_OUTPUT_CURRENT = 10,				//A10
	A2D_AN11_TEMP5 = 11,						//A11
	A2D_AN12_VDC_BUS_PLUS = 12,					//A12
	A2D_AN13_TRANSFORMER_SECONDARY_PLUS = 13,	//A13
	A2D_AN14_VOUT_PLUS = 14,					//A14
	A2D_AN15_VOUT_MINUS = 15,					//A15
	NUMBER_OF_A2D_PINS
};

//Libraries
//Pins Library
#define PINS_MAJOR	2
#define PINS_MINOR	0
#define PINS_PATCH	0

//Scheduler Library
#define SCHEDULER_MAJOR	0
#define SCHEDULER_MINOR	1
#define SCHEDULER_PATCH	0

//Function Prototype
void Configure_For_Afriji(void);

#endif	/* CONFIG_H */
