#ifndef PINS_H
#define	PINS_H
/*
Instructions for adding to a new project:
Add the code with the header "Add to config file header" to the config.h file, and the code labeled "Add to config file"
to config.c. The pin definitions (PIN_DEFINITIONS) that are placed into config.h can be in any order and do not need to
line up incrementally based on the actual pins. This is because they must be declared and matched to a PIN_DEFINITION.

During initialization, you need to create a definition of each pin so that they can be properly linked during runtime:
	Eg. Pin_Definition(PIN_RA0,	Rx0, &TRISA, &ODCA, &LATA, &PORTA); //RA0
This matches the unique defined PIN_DEFINITION with the addresses of the pins related registers. After that, the pins should
be initialized with the desired starting conditions:
	Eg. Pin_Initialize(PIN_RA0,	LOW, PUSH_PULL, INPUT);	//RA0
This will ensure the pins are setup correctly and initialized so as to not cause any problems later in the program.

The code will operate on the pins a few cycles slower than a direct call would, however, the benefit is that pins can be
universally remapped quickly and easily and most pins do not need to be changed instantaneously.

The functions Pin_Low and Pin_High allow the pin to be set specifically
The function Pin_Toggle allows a pins output to be toggled
The function Pin_Write allows the output to be determined at run time if it isn't known at compile time
The functions Pin_Set_ODC and Pin_Set_TRIS allow the Open-Drain and Tristate to be changed during run time
The functions Pin_Get_ODC and Pin_Get_TRIS allow the current Open-Drain and Tristate values to be read during run time
The function Pin_Read allows the current pin state to be read in
*/

/***********Add to config file header************/
/*
enum PIN_DEFINITIONS
{
	PIN_RA0,	//RA0
	PIN_RA1,	//RA1
	PIN_RA2,	//RA2
	PIN_RA3,	//RA3
	PIN_RA4,	//RA4
	PIN_RA5,	//RA5
	PIN_RA6,	//RA6
	PIN_RA7,	//RA7
	PIN_RA8,	//RA8
	PIN_RA9,	//RA9
	PIN_RA10,	//RA10
	PIN_RA11,	//RA11
	PIN_RA12,	//RA12
	PIN_RA13,	//RA13
	PIN_RA14,	//RA14
	PIN_RA15,	//RA15
//	ETC...
	NUMBER_OF_PINS
};

//Pins Library
#define PINS_MAJOR	2
#define PINS_MINOR	0
#define PINS_PATCH	0
*/

/***************Add to config file***************/
/*
#ifndef PINS_LIBRARY
	#error "You need to include the Pins library for this code to compile"
#endif
 */

/************* Semantic Versioning***************/
#define PINS_LIBRARY

/*************   Magic  Numbers   ***************/
#define LOW			0
#define HIGH		1
#define OUTPUT		0
#define INPUT		1
#define PUSH_PULL	0
#define OPEN_DRAIN	1

/*************    Enumeration     ***************/
enum
{
	Rx0 = 0x0001,
	Rx1 = 0x0002,
	Rx2 = 0x0004,
	Rx3 = 0x0008,
	Rx4 = 0x0010,
	Rx5 = 0x0020,
	Rx6 = 0x0040,
	Rx7 = 0x0080,
	Rx8 = 0x0100,
	Rx9 = 0x0200,
	Rx10 = 0x0400,
	Rx11 = 0x0800,
	Rx12 = 0x1000,
	Rx13 = 0x2000,
	Rx14 = 0x4000,
	Rx15 = 0x8000
};

/*************Structure Definitions**************/
/***********State Machine Definitions************/
/*************Function  Prototypes***************/

/**
 * Creates a link between the Pins unique designator and the TRIS/LAT/PORT/ODC/pin registers
 * @param pinID The unique ID that this pin is referred to as
 * @param mask A Rxn number between 0 and 15 that indicates the pin number
 * @param TRISregister Address of Tristate register
 * @param ODCregister Address of Open Drain Control register
 * @param LATregister Address of Latch register
 * @param PORTregister Address of Port register
 */
void Pin_Definition(int pinID, int mask, volatile unsigned int *TRISregister, volatile unsigned int *ODCregister, volatile unsigned int *LATregister, volatile unsigned int *PORTregister);

/**
 * Initializes a pin
 * @param pinID The unique ID that this pin is referred to as
 * @param latch The desired state the LAT should start in (1 = High, 0 = Low)
 * @param odc The desired state the ODC should be in (1 = Open-Drain, 0 = Push-Pull)
 * @param tris The desired state the TRIS should start in (1 = Input, 0 = Output)
 */
void Pin_Initialize(int pinID, int latch, int odc, int tris);

/**
 * Sets the pin low (LAT register)
 * @param pinID The unique ID that this pin is referred to as
 */
void Pin_Low(int pinID);

/**
 * Sets the pin high (LAT register)
 * @param pinID The unique ID that this pin is referred to as
 */
void Pin_High(int pinID);

/**
 * Toggles the pins (LAT register)
 * @param pinID The unique ID that this pin is referred to as
 */
void Pin_Toggle(int pinID);

/**
 * Allows programatically setting of a pin (LAT register)
 * @param pinID The unique ID that this pin is referred to as
 * @param newState (1 = High, 0 = Low)
 */
void Pin_Write(int pinID, int newState);

/**
 * Sets the ODC register
 * @param pinID The unique ID that this pin is referred to as
 * @param newState (1 = Open-Drain, 0 = Push-Pull)
 */
void Pin_Set_ODC(int pinID, int newState);

/**
 * Sets the TRIS register
 * @param pinID The unique ID that this pin is referred to as
 * @param newState (1 = Input, 0 = Output)
 */
void Pin_Set_TRIS(int pinID, int newState);

/**
 * Reads the current pin (PORT register)
 * @param pinID The unique ID that this pin is referred to as
 * @return (1 = High, 0 = Low)
 */
int Pin_Read(int pinID);

/**
 * Reads the current ODC register
 * @param pinID The unique ID that this pin is referred to as
 * @return the current ODC value (1 = Open-Drain, 0 = Push-Pull)
 */
int Pin_Get_ODC(int pinID);

/**
 * Reads the current TRIS register
 * @param pinID The unique ID that this pin is referred to as
 * @return (1 = Input, 0 = Output)
 */
int Pin_Get_TRIS(int pinID);

#endif	/* PINS_H */
