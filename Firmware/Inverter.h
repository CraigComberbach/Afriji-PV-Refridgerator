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
void Inverter_Routine(unsigned long time_mS);
void Initialize_Inverter(void);
void Set_Target_Delay_uS(int newDelay_uS, enum INVERTERS_SUPPORTED inverter);
int Get_Target_Delay_uS(enum INVERTERS_SUPPORTED inverter);

#endif
