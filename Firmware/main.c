/**************************************************************************************************
Target Hardware:		PIC24FJ256GA106
Code assumptions:		
Purpose:				Firmware for Afriji Solar DC to AC appliance power supply device
Notes:					

Version History:
v0.0.A	2017-03-12      Craig Comberbach, Michael MacKay
Compiler: XC16 v1.26	IDE: MPLABx 3.30	Tool: ICD3	Computer: Intel Core2 Quad CPU 2.40 GHz, 5 GB RAM, Windows 10 64 bit Home
 Revision History:
 * v0.0.A   -   Initial prototype firmware
**************************************************************************************************/
/*************    Header Files    ***************/
#include "Config.h"
#include "Scheduler.h"
#include "A2D.h"
#include "Inverter.h"
#include "Pins.h"
#include "Debug.h"

/************* Library Definition ***************/
/************* Semantic Versioning***************/
#if MAJOR_FIRMWARE_VERSION != 0
    #error "Firmware has had a change that loses some previously supported functionality"
#elif MINOR_FIRMWARE_VERSION != 1
    #error "Firmware has new features that this code may benefit from"
#elif PATCH_FIRMWARE_VERSION != 0
	#error "Firmware has had a bug fix, you should check to see that we weren't relying on a bug for functionality"
#endif
/*************Library Dependencies***************/
/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
/*************    Enumeration     ***************/
/***********  Structure Definitions  ************/
/***********State Machine Definitions************/
/*************  Global Variables  ***************/
/*************Interrupt Prototypes***************/
/*************Function  Prototypes***************/
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
	Initialize_Scheduler(40/*uS*/);
	Schedule_Task(STARTUP_TASK,		&Heart_Beat_Task,	1000/*uS Delay*/,		50000/*uS Period*/,		20/*Repetitions*/);
	Schedule_Task(HEART_BEAT_TASK,	&Heart_Beat_Task,	1000000/*uS Delay*/,	500000/*uS Period*/,	PERMANENT_TASK);
	Schedule_Task(INVERTER_TASK,	&Inverter_Routine,	1000000/*uS Delay*/,	40/*uS Period*/,		PERMANENT_TASK);
	Schedule_Task(A2D_TASK,			&A2D_Routine,		1000666/*uS Delay*/,	1000/*uS Period*/,		PERMANENT_TASK);//No longer than once ever 8mS will allow the result to be captured in time to be used with a 60Hz waveform
	Schedule_Task(FREQUENCY_RAMP,	&Frequency_Ramp,	2000000/*uS Delay*/,	200000/*uS Period*/,	40/*Repetitions*/);
	Schedule_Task(TERMINAL_TASK,	&Debug_Routine,		2000000/*uS Delay*/,	1000000/*uS Period*/,	PERMANENT_TASK);

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
