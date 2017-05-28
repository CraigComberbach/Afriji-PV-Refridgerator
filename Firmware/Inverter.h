#ifndef INVERTER_H
#define INVERTER_H
/***********Add to config file header************/
/*
//Inverter Library
#define INVERTER_MAJOR	0
#define INVERTER_MINOR	1
#define INVERTER_PATCH	0

enum INVERTERS_SUPPORTED
{
	NUMBER_OF_INVERTERS_SUPPORTED
};
*/

/***************Add to config file***************/
/*
#ifndef INVERTER_LIBRARY
	#error "You need to include the Inverter library for this code to compile"
#endif
 */

/*************Semantic  Versioning***************/
#define INVERTER_LIBRARY

/*************Library Dependencies***************/
/*************   Magic  Numbers   ***************/
/*************    Enumeration     ***************/
/*************    Structures      ***************/
/***********State Machine Definitions************/
/*************Function  Prototypes***************/
void Initialize_Inverter(void);
void Inverter_Routine(unsigned long time_mS);
void Frequency_Ramp(unsigned long time_mS);
void Set_Output_Hz(int newFrequency_Hz, enum INVERTERS_SUPPORTED inverter);
int Get_Output_Hz(enum INVERTERS_SUPPORTED inverter);
void Set_Rated_Hz (int newFrequency_Hz, enum INVERTERS_SUPPORTED inverter);
int Get_Rated_Hz(enum INVERTERS_SUPPORTED inverter);
void Set_Rated_RMS_Voltage (int newRatedVoltage_Vx10, enum INVERTERS_SUPPORTED inverter);
int Get_Rated_RMS_Voltage (enum INVERTERS_SUPPORTED inverter);
#endif
