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
#include "Scheduler.h"
#include "A2D.h"
#include "Inverter.h"
#include "Pins.h"

/************* Library Definition ***************/
/************* Semantic Versioning***************/
/*************Library Dependencies***************/
/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
/*************    Enumeration     ***************/
/***********  Structure Definitions  ************/
/***********State Machine Definitions************/
/*************  Global Variables  ***************/
/*************Interrupt Prototypes***************/
/*************Function  Prototypes***************/
void Heart_Beat_Task(unsigned long time_mS);

/************* Device Definitions ***************/
/************* Module Definitions ***************/
/************* Other  Definitions ***************/

_CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF & BKBUG_OFF & COE_OFF & ICS_PGx3 & FWDTEN_ON & WINDIS_OFF & FWPSA_PR32 & WDTPS_PS1024 )
_CONFIG2( IESO_OFF & FNOSC_FRCPLL & FCKSM_CSDCMD & OSCIOFNC_ON & IOL1WAY_ON & POSCMOD_NONE )
_CONFIG3( WPDIS_WPDIS )

int main(void)
{
	ClrWdt();
	Configure_For_Afriji();

	//Scheduled Tasks
	Initialize_Scheduler(100/*uS*/);
	Schedule_Task(STARTUP_TASK,					&Heart_Beat_Task,				1000/*uS Delay*/,		50000/*uS Period*/,		20/*Repetitions*/);
	Schedule_Task(HEART_BEAT_TASK,				&Heart_Beat_Task,				1000000/*uS Delay*/,	2048000/*uS Period*/,	PERMANENT_TASK);
	Schedule_Task(INVERTER_TASK,				&Inverter_Routine,				1000000/*uS Delay*/,	1000/*uS Period*/,		PERMANENT_TASK);
	Schedule_Task(A2D_TASK,						&A2D_Routine,					1000666/*uS Delay*/,	1000/*uS Period*/,		PERMANENT_TASK);

	while(1)
	{
		ClrWdt();
		while(Waiting_To_Run_Tasks())
		{
			ClrWdt();
			Task_Master();
			ClrWdt();
		}
		ClrWdt();
	}

	return 1;
}

void Heart_Beat_Task(unsigned long time_mS)
{
	Pin_Toggle(PIN_RD9_GREEN_LED);
	return;
}
