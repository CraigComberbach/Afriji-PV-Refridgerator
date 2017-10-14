//Add individual task timing statistics
//Add global timing statistics

/**************************************************************************************************
Authours:				Craig Comberbach
Target Hardware:		PIC24F
Chip resources used:	Uses timer1
Code assumptions:		
Purpose:				Allows easy scheduling of tasks with minimal overhead

Version History:
v0.1.0	2016-05-30	Craig Comberbach	Compiler: XC16 v1.11	IDE: MPLABx 3.30	Tool: ICD3		Computer: Intel Core2 Quad CPU 2.40 GHz, 5 GB RAM, Windows 10 Home 64-bit
 Added limited recurrence
 Refactored the Task Master function to be simpler
 Added Auto-Magic? calculation of Timer1 during initialization
v0.0.0	2016-05-26	Craig Comberbach	Compiler: XC16 v1.11	IDE: MPLABx 3.30	Tool: ICD3		Computer: Intel Core2 Quad CPU 2.40 GHz, 5 GB RAM, Windows 10 Home 64-bit
 First version - Most functionality implemented
 Does not implement limited recurrence tasks, only permanent ones
 
**************************************************************************************************/
/*************    Header Files    ***************/
#include "Config.h"
#include "Scheduler.h"

/************* Semantic Versioning***************/
#if SCHEDULER_MAJOR != 0
	#error "Scheduler library has had a change that loses some previously supported functionality"
#elif SCHEDULER_MINOR != 1
	#error "Scheduler library has new features that this code may benefit from"
#elif SCHEDULER_PATCH != 0
	#error "Scheduler library has had a bug fix, you should check to see that we weren't relying on a bug for functionality"
#endif

/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
/*************    Enumeration     ***************/
/***********State Machine Definitions************/
/*************  Global Variables  ***************/
#ifdef CONSERVE_MEMORY
	uint16_t schedulerPeriod_uS;
#else
	uint32_t schedulerPeriod_uS;
#endif
//#define TASK_PROFILING_ENABLED
struct SCHEDULED_TASKS
{
	#ifdef CONSERVE_MEMORY
		void (*task)(uint16_t);
		uint16_t period_uS;
		uint16_t countDown_uS;
		uint16_t recurrence;
		uint16_t recurrenceCount;
		#ifdef TASK_PROFILING_ENABLED
			uint16_t minExecutionTime_FCYticks;
			uint16_t avgExecutionTime_FCYticks;
			uint32_t sumExecutionTime_FCYticks;
			uint16_t maxExecutionTime_FCYticks;
			uint16_t currentExecutionTime_FCYticks;
		#endif
	#else
		void (*task)(uint32_t);
		uint32_t period_uS;
		uint32_t countDown_uS;
		uint32_t recurrence;
		uint32_t recurrenceCount;
		#ifdef TASK_PROFILING_ENABLED
			uint32_t minExecutionTime_FCYticks;
			uint32_t sumExecutionTime_FCYticks;
			uint32_t avgExecutionTime_FCYticks;
			uint32_t maxExecutionTime_FCYticks;
			uint32_t currentExecutionTime_FCYticks;
		#endif
	#endif
} scheduledTasks[NUMBER_OF_SCHEDULED_TASKS];
uint32_t minGlobalExecutionTime_uS;
uint32_t avgGlobalExecutionTime_uS;
uint32_t maxGlobalExecutionTime_uS;
int16_t delayFlag = 0;

/*************Function  Prototypes***************/
void __attribute__((interrupt, auto_psv)) _T1Interrupt(void);

/************* Device Definitions ***************/
/************* Module Definitions ***************/
/************* Other  Definitions ***************/

void Task_Master(void)
{
	int16_t loop;
	int16_t time;
	
	for(loop = 0; loop < NUMBER_OF_SCHEDULED_TASKS; ++loop)
	{
		ClrWdt();
		if(scheduledTasks[loop].countDown_uS <= schedulerPeriod_uS)
		{
			ClrWdt();
			if((scheduledTasks[loop].recurrenceCount < scheduledTasks[loop].recurrence) || (scheduledTasks[loop].recurrence == PERMANENT_TASK))
			{
				ClrWdt();
				//Document how many times this task has run (safely rolls over)
				if(++scheduledTasks[loop].recurrenceCount == 0)
					scheduledTasks[loop].recurrenceCount = 1;

				scheduledTasks[loop].countDown_uS = scheduledTasks[loop].period_uS;	//Reset for next time
				time = TMR1;//Record when the task started
				scheduledTasks[loop].task(scheduledTasks[loop].period_uS);			//Run the current task, send the time since last execution
				time = TMR1 - time;//Record how long the task took

				//Task profiling
				#ifdef TASK_PROFILING_ENABLED
					//Minimum execution Time
					if(time < scheduledTasks[loop].minExecutionTime_FCYticks)
						scheduledTasks[loop].minExecutionTime_FCYticks = time;

					//Maximum Execution time
					if(time > scheduledTasks[loop].maxExecutionTime_FCYticks)
						scheduledTasks[loop].maxExecutionTime_FCYticks = time;

					//Average Execution Time
					scheduledTasks[loop].sumExecutionTime_FCYticks += time;
					scheduledTasks[loop].avgExecutionTime_FCYticks = scheduledTasks[loop].sumExecutionTime_FCYticks / scheduledTasks[loop].recurrenceCount;
					Nop();
				#endif
			}
		}
		else
			scheduledTasks[loop].countDown_uS -= schedulerPeriod_uS;
	}

	delayFlag = 0;

	return;
}

void Initialize_Scheduler(uint32_t newPeriod_uS)
{
	uint32_t period;
	//Ensure the period makes sense
	if(newPeriod_uS != 0)
		schedulerPeriod_uS = newPeriod_uS;
	else
		while(1)//TODO - DEBUG ME! I should never execute
			ClrWdt();

	period = FCY_MHz;
	period *= newPeriod_uS;
	
	if(period > 65535)
		while(1);//This period can never run, we only have 16 bits of register

	//Auto-Magically setup Timer1
	PR1				= (uint16_t)period;
	IEC0bits.T1IE	= 1;
	T1CONbits.TCS	= 0;	//0 = Internal clock (FOSC/2)
//	T1CONbits.TSYNC	= 1;	//When TCS = 0: This bit is ignored.
	T1CONbits.TCKPS	= 0;	//00 = 1:1
	T1CONbits.TGATE	= 0;	//0 = Gated time accumulation disabled
	T1CONbits.TSIDL	= 0;	//0 = Continue module operation in Idle mode
	T1CONbits.TON	= 1;	//1 = Starts 16-bit Timer1
	
	return;
}

void Schedule_Task(enum SCHEDULER_DEFINITIONS taskName, void (*newTask)(uint32_t), uint32_t newInitialDelay_uS, uint32_t newPeriod_uS, uint16_t newRepetitions)
{
	//Task Information
	if(*newTask  != (void*)0)
		scheduledTasks[taskName].task = newTask;
	else
		while(1)//TODO - DEBUG ME! I should never execute
			ClrWdt();

	if(newPeriod_uS < schedulerPeriod_uS)
		while(1)//TODO - DEBUG ME! I should never execute
			ClrWdt();

	if(newInitialDelay_uS < schedulerPeriod_uS)
		while(1)//TODO - DEBUG ME! I should never execute
			ClrWdt();

	//Timing Information
	scheduledTasks[taskName].countDown_uS = newInitialDelay_uS;
	scheduledTasks[taskName].period_uS = newPeriod_uS;

	//Recurrence Information
	scheduledTasks[taskName].recurrence = newRepetitions;
	scheduledTasks[taskName].recurrenceCount = 0;

	#ifdef TASK_PROFILING_ENABLED
		//Runtime Statistics Information
		scheduledTasks[taskName].currentExecutionTime_FCYticks = 0;
		scheduledTasks[taskName].minExecutionTime_FCYticks = ~0;
		scheduledTasks[taskName].avgExecutionTime_FCYticks = 0;
		scheduledTasks[taskName].maxExecutionTime_FCYticks = 0;
	#endif

	return;
}

uint32_t Get_Task_Period(enum SCHEDULER_DEFINITIONS taskName)
{
	return scheduledTasks[taskName].period_uS;
}

void Set_Task_Period(enum SCHEDULER_DEFINITIONS taskName, uint32_t newPeriod_uS)
{
	//Timing Information
	scheduledTasks[taskName].period_uS = newPeriod_uS;

	return;
}

int8_t Waiting_To_Run_Tasks(void)
{
	return delayFlag;
}

void __attribute__((interrupt, auto_psv)) _T1Interrupt(void)
{
	IFS0bits.T1IF = 0;
	delayFlag = 1;
	TMR1 = 0;

	return;
}
