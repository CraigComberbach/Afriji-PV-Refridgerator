#ifndef Debug_H
#define Debug_H
/*
Instructions for adding to a new project:
*/

/***********Add to config file header************/
/*
//Debug Library
#define DEBUG_MAJOR	1
#define DEBUG_MINOR	1
#define DEBUG_PATCH	0
*/

/***************Add to config file***************/
/*
#ifndef DEBUG_LIBRARY
	#error "You need to include the Debug library for this code to compile"
#endif
 */

/************* Semantic Versioning***************/
#define DEBUG_LIBRARY

/*************   Magic  Numbers   ***************/
/*************    Enumeration     ***************/
/***********State Machine Definitions************/
/*************Function  Prototypes***************/
void Debug_Initialize(void);
void Debug_Routine(uint32_t time_mS);

#endif
