/**************************************************************************************************
Target Hardware:		PIC24F
Chip resources used:	UART
Purpose:				Allows the use of a terminal window for debug and development

Version History:
v0.0.0	YYYY-MM-DD  Craig Comberbach
	Compiler: C30 v3.31	IDE: MPLABx 1.80	Tool: RealICE	Computer: Intel Xeon CPU 3.07 GHz, 6 GB RAM, Windows 7 64 bit Professional SP1
	First version
 **************************************************************************************************/
/*************    Header Files    ***************/
#include "Config.h"
#include "debug.h"
#include "A2D.h"

/************* Semantic Versioning***************/
/************Arbitrary Functionality*************/
/*************   Magic  Numbers   ***************/
#define CONVERT_TO_ASCII	48+	//Aligns the numbers to the ascii value

/*************    Enumeration     ***************/
/*************ArbitraryFunctionality*************/
/************* Module Definitions ***************/
/************* Other  Definitions ***************/
/*************  Global Variables  ***************/
const char startupMessage[] = "Created by Craig Comberbach, Mike MacKay, and Micah Melnyk for Afriji on";
const char displayHeader[] = "\n    Vin     Iin  XfmrP-  XfmrP+  XfmrS+     DC+   Vout+   Vout-    Iout      T1      T2      T3      T4      T5\n";

/*************Function  Prototypes***************/
void Debug_Message(int value, char units);
void Remove_Leading_Zeros(int value, char cleaned[6]);

void Debug_Routine(uint32_t time_uS)
{
	static int displayHeaderTimeout = 1;
	int loop = 0;

	--displayHeaderTimeout;
	if(displayHeaderTimeout == 0)
	{
		displayHeaderTimeout = 10;
		while(displayHeader[loop] != '\0')
		{
			while(U1STAbits.UTXBF == 1);
			U1TXREG = displayHeader[loop];
			loop++;
		}
	}

	//Print out formated values to the terminal window
	Debug_Message(A2D_Value(2), 'V');	//Vin
	Debug_Message(A2D_Value(9), 'A');	//Iin
	Debug_Message(A2D_Value(0), 'V');	//XfmrP-
	Debug_Message(A2D_Value(1), 'V');	//XfmrP+
	Debug_Message(A2D_Value(13), 'V');	//XfmrS+
	Debug_Message(A2D_Value(12), 'V');	//DC+
	Debug_Message(A2D_Value(14), 'V');	//Vout+
	Debug_Message(A2D_Value(15), 'V');	//Vout-
	Debug_Message(A2D_Value(10), 'A');	//Iout
	Debug_Message(A2D_Value(6), 'C');	//T1
	Debug_Message(A2D_Value(3), 'C');	//T2
	Debug_Message(A2D_Value(7), 'C');	//T3
	Debug_Message(A2D_Value(8), 'C');	//T4
	Debug_Message(A2D_Value(11), 'C');	//T5

	//Move to a newline
	while(U1STAbits.UTXBF == 1);
	U1TXREG = '\n';

	return;
}

void Debug_Initialize(void)
{
	int loop = 0;

	//Setup Feedback UART
	U1BRG				= 34;	//~115200 (114285)
	IEC0bits.U1TXIE		= 0;	//0 = Interrupt request not enabled
	IEC0bits.U1RXIE		= 0;	//0 = Interrupt request not enabled

	//Enable TX pin
//	U1STAbits.URXDA		= 
//	U1STAbits.OERR		= 
//	U1STAbits.FERR		= 
//	U1STAbits.PERR		= 
//	U1STAbits.RIDLE		= 
//	U1STAbits.ADDEN		= 
//	U1STAbits.URXISEL	= 
//	U1STAbits.TRMT		= 
//	U1STAbits.UTXBF		= 
//	U1STAbits.UTXBRK	= 
//	U1STAbits.UTXINV	= 
//	U1STAbits.UTXISEL0	= 
//	U1STAbits.UTXISEL1	= 
	
	//Setup for 8-N-1,enable pins, and enable module
	U1MODEbits.STSEL	= 0;	//0 = One Stop bit
	U1MODEbits.PDSEL	= 0b00;	//00 = 8-bit data, no parity
	U1MODEbits.BRGH		= 1;	//1 = High-Speed mode (baud clock generated from FCY/4)
//	U1MODEbits.RXINV	= 
//	U1MODEbits.ABAUD	= 
//	U1MODEbits.LPBACK	= 
//	U1MODEbits.WAKE		= 
	U1MODEbits.UEN		= 0b00;	//00 = UxTX and UxRX pins are enabled and used; UxCTSand UxRTS/BCLKx pins controlled by port latches
//	U1MODEbits.RTSMD	= 
//	U1MODEbits.IREN		= 
//	U1MODEbits.USIDL	= 
	U1MODEbits.UARTEN	= 1;	//1 = UARTx is enabled; all UARTx pins are controlled by UARTx as defined by UEN<1:0>
	U1STAbits.UTXEN		= 1;	//1 = Transmit enabled; UxTX pin controlled by UARTx

	//Startup message
	loop = 0;
	while(startupMessage[loop] != '\0')
	{
		if(U1STAbits.UTXBF == 0)
		{
			U1TXREG = startupMessage[loop];
			loop++;
		}
	}
	while(U1STAbits.UTXBF == 1);
	U1TXREG = '\n';

	//Compiled on date message
	loop = 0;
	while(compiledOnDate[loop] != '\0')
	{
		if(U1STAbits.UTXBF == 0)
		{
			U1TXREG = compiledOnDate[loop];
			loop++;
		}
	}
	while(U1STAbits.UTXBF == 1);
	U1TXREG = ' ';

	//Compiled on time message
	loop = 0;
	while(compiledAtTime[loop] != '\0')
	{
		if(U1STAbits.UTXBF == 0)
		{
			U1TXREG = compiledAtTime[loop];
			loop++;
		}
	}
	while(U1STAbits.UTXBF == 1);
	U1TXREG = '\n';
	while(U1STAbits.UTXBF == 1);
	U1TXREG = '\n';

	return;
}

void Debug_Message(int value, char units)
{
	int loop;
	char message[6] = "      ";

	Remove_Leading_Zeros(value, message);

	for(loop = 0; loop < 6; ++loop)
	{
		while(U1STAbits.UTXBF == 1);
		U1TXREG = message[loop];		
	}

	while(U1STAbits.UTXBF == 1);
	U1TXREG = units;
	
	while(U1STAbits.UTXBF == 1);
	U1TXREG = ' ';
	
	return;
}

void Remove_Leading_Zeros(int value, char cleaned[6])
{
	int loop;
	int digit = 10000;
	int isNegative = 0;
	int stillLeading = 1;
	
	//Ensure a positive number
	if(value < 0)
	{
		isNegative = 1;
		value *= -1;
	}

	//Zero is a special case (read as super easy)
	if(value == 0)
	{
		for(loop = 0; loop < 6; ++loop)
		{
			if(loop >= 5)
				cleaned[loop] = '0';
			else
				cleaned[loop] = ' ';
		}
		return;
	}
	
	//Convert to ASCII
	for(loop = 1; loop < 6; ++loop)
	{
		//Check if we are using a leading zero
		if((stillLeading == 1) && (value < digit))
			cleaned[loop] = ' ';
		else
		{
			stillLeading = 0;
			cleaned[loop] = (char)(CONVERT_TO_ASCII(value/digit));
			value %= digit;
		}

		//Advance the digit
		digit /= 10;
	}

	//Add the Negative Sign
	if(isNegative)
	{
		for(loop = 5; loop >= 0; --loop)
		{
			if(cleaned[loop] == ' ')
			{
				cleaned[loop] = '-';
				break;
			}
		}
	}
	
	return;
}
