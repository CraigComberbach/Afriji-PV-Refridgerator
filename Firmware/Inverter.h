#ifndef INVERTER_H
#define INVERTER_H
/***********Add to config file header************/
/*
//Inverter Library
#define INVERTER_MAJOR	0
#define INVERTER_MINOR	1
#define INVERTER_PATCH	0
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
enum INVERTERS_SUPPORTED
{
	HIGH_CURRENT,
	HIGH_VOLTAGE,
	NUMBER_OF_INVERTERS_SUPPORTED
};

/*************    Structures      ***************/
/***********State Machine Definitions************/
/*************Function  Prototypes***************/
void Inverter_Routine(unsigned long time_mS);
void Initialize_Inverter(void);

#endif
