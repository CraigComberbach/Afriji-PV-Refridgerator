#ifndef SCHEDULER_H
#define	SCHEDULER_H

/*
Instructions for adding to a new project:
*/

/***********Add to config file header************/
/*
//Scheduler Library
#define SCHEDULER_MAJOR	0
#define SCHEDULER_MINOR	1
#define SCHEDULER_PATCH	0
*/

/***************Add to config file***************/
/*
#ifndef SCHEDULER_LIBRARY
	#error "You need to include the Scheduler library for this code to compile"
#endif
 */

/************* Semantic Versioning***************/
#define SCHEDULER_LIBRARY

/*************   Magic  Numbers   ***************/
#define PERMANENT_TASK	0

/*************    Enumeration     ***************/
/***********State Machine Definitions************/
/*************Function  Prototypes***************/
void Task_Master(void);
void Initialize_Scheduler(uint32_t newPeriod_uS);
void Schedule_Task(enum SCHEDULER_DEFINITIONS taskDuJour, void (*newTask)(uint32_t), uint32_t newInitialDelay_uS, uint32_t newPeriod_uS, uint16_t newRepetitions);
int8_t Waiting_To_Run_Tasks(void);

#endif	/* SCHEDULER_H */

