#ifndef A2D_H
#define A2D_H
/*
Instructions for adding to a new project:
Add the code with the header "Add to config file header" to the config.h file, and the code labeled "Add to config file"
to config.c. The analog pin definitions (A2D_PIN_DEFINITIONS) that are placed into config.h need to line up incrementally based
on the analog channel (eg AN5 needs to be enumerated to 5).

The initiliaze routine needs to be run once on startup. After that the you are required to run both A2D_Channel_Settings() and
then A2D_Add_To_Scan_Queue() before the A2D will work. Calling the A2D_Add_To_Scan_Queue() function several times adds multiple
instances of the channel to the queue. This is a way to get a quicker update rate on a channel if there are several other
channels. Running A2D_Channel_Settings() once a conversion is under way will reset the channel to the specified new settings
and restart the conversion process. It would typically only be called multiple times if a different resolution was temporarily
required, or if the format/pre/post/finished functions needed to be changed for use in different configurations.

The A2D_Advanced_Channel_Settings() function takes extra optional arguments. Use it instead of A2D_Channel_Setting(). Calling
A2D_Channel_Setting() will return all the extra optional settings to the default setting. The optional features include a way
to call a function before the A2D scan burst begins, allowing switched pins (ie thermistor power) to be turned on, or set a flag,
or... Another feature allows you to call a function when the busrt scan is finished, thus allowing you to automagically turn off
the switched pin used earlier, or clear the flag. Yet another feature allows a function to be called when the scan is complete and
the value has been properly averaged and formated, useful if some code needs to run as soon as a new value is available (PID
routines for examples). There is a feature to allow custom averaging to take place, this will intercept the A2D burst before it
is added to the sample sum (prior to averaging and formatting, which can be useful for looking at individual samples (ie True Random
Number Generator). The last feature allows a custom burst size, which works really well with the CTMU, as only the first sample is
valid, each successive sample will damage the result.

Reading the current value is done through A2D_Value(). Note: This value is not the raw A2D value, rather it is already formatted
and at the correct resolution as specificied according to the latest calling of A2D_Channel_Settings();

A2D_Routine() needs to be called on a regular basis. The more often it is called, the faster channels will update their values.
Calling the routine before a conversion is done will not interrupt the current conversion, though it will have no other effect.

The markup above each function will pop-up as a helpful reminder of the arguments each function will take, as well as what value
is returned, and what the function will do.

Calculating how much time a full update will take to complete is done via the following formulae:
 * (4^r*s*n*t)/(b*q)
 * Where:
 * r = Bits of resolution increase from 10-bit (0 for 10 bit, 1 for 11 bit, etc)
 * s = Number of samples at the requested resolution for an updated value
 * n = Number of channels in the queue (NOTE: Includes repeated channels)
 * t = The time interval between calling the A2D_Routine (Eg. Time of main loop)
 * b = The number of samples per scan
 * q = Number of times a channel appears in the queue (Can be more than once for repeated channels)
*/

/***********Add to config file header************/
/*
enum A2D_PIN_DEFINITIONS
{
	A2D_A0 = 0,		//A0
	A2D_A1 = 1,		//A1
	A2D_A2 = 2,		//A2
	A2D_A3 = 3,		//A3
	A2D_A4 = 4,		//A4
	A2D_A5 = 5,		//A5
	A2D_A6 = 6,		//A6
	A2D_A7 = 7,		//A7
	A2D_A8 = 8,		//A8
	A2D_A9 = 9,		//A9
	A2D_A10 = 10,	//A10
	A2D_A11 = 11,	//A11
	A2D_A12 = 12,	//A12
	A2D_A13 = 13,	//A13
	A2D_A14 = 14,	//A14
	A2D_A15 = 15	//A15
};

//A2D Library
#define A2D_MAJOR	1
#define A2D_MINOR	1
#define A2D_PATCH	0
*/

/***************Add to config file***************/
/*
#ifndef A2D_LIBRARY
	#error "You need to include the A2D library for this code to compile"
#endif
 */

/************* Semantic Versioning***************/
#define A2D_LIBRARY

/*************   Magic  Numbers   ***************/
#define NO_FORMATING			(void*)0
#define NO_PREFUNCTION			(void*)0
#define NO_POSTFUNCTION			(void*)0
#define NO_FINISHED_FUNCTION	(void*)0
#define NORMAL_AVERAGING		(void*)0
#define AUTO_CONVERSION			0
#define MANUAL_CONVERSION		1
enum A2D_SAMPLE_SIZE
{
	A2D_ONE_SAMPLE,			//1
	A2D_TWO_SAMPLES,		//2
	A2D_THREE_SAMPLES,		//3
	A2D_FOUR_SAMPLES,		//4
	A2D_FIVE_SAMPLES,		//5
	A2D_SIX_SAMPLES,		//6
	A2D_SEVEN_SAMPLES,		//7
	A2D_EIGHT_SAMPLES,		//8
	A2D_NINE_SAMPLES,		//9
	A2D_TEN_SAMPLES,		//10
	A2D_ELEVEN_SAMPLES,		//11
	A2D_TWELVE_SAMPLES,		//12
	A2D_THIRTEEN_SAMPLES,	//13
	A2D_FOURTEEN_SAMPLES,	//14
	A2D_FIFTEEN_SAMPLES,	//15
	A2D_MAX_SAMPLES,		//16
};

/*************    Enumeration     ***************/
enum RESOLUTION
{
	RESOLUTION_10_BIT,	//0 (Max samples = 65520)
	RESOLUTION_11_BIT,	//1 (Max samples = 16380)
	RESOLUTION_12_BIT,	//2 (Max samples = 4095)
	RESOLUTION_13_BIT,	//3 (Max samples = 1023)
	RESOLUTION_14_BIT,	//4 (Max samples = 255)
	RESOLUTION_15_BIT,	//5 (Max samples = 63)
	RESOLUTION_16_BIT	//6 (Max samples = 15)
};

/***********State Machine Definitions************/
/*************Function  Prototypes***************/
/**
 * Used to setup the A2D Module to the standard default settings
 */
void A2D_Initialize(void);

/**
 * Sets up the fundamental settings of the scan, allowing you to change the number of samples and increasing resolution, in addition to adding custom formating
 * @param channel The A2D channel that is to be scanned, these are enumerated in the project config file
 * @param desiredResolution The desired additional bits of resolution (between 0(10-bit) and 6(16-bit)), you can use enum RESOLUTION in this header file to simplify
 * @param numberOfAverages The number of desired "readings" to be averaged (This will be different than the number of samples taken if additional resolution is selected). This number must be 16 or greater, must not be 65536 or bigger and must be a multiple of 16.
 * @param formatFunction Function pointer that will format the raw A2D values, the function must accept an integer representing the raw A2D value, it should also return the formatted value as an integer
 * @return 1 = Channel was updated successfully, 0 = Value out of range, no changes were made
 */
int A2D_Channel_Settings(int channel, enum RESOLUTION desiredResolutionIncrease, int numberOfAverages, int (*formatFunction)(int));

/**
 * Sets up the fundamental settings of the scan, allowing you to change the number of samples and increasing resolution, in addition to adding custom formating and inserting function before/after a scan as well as when a channel is finished scanning, as well as detailing how averaging is done, and how many samples are taken in a burst
 * @param channel The A2D channel that is to be scanned, these are enumerated in the project config file
 * @param desiredResolution The desired additional bits of resolution (between 0(10-bit) and 6(16-bit)), you can use enum RESOLUTION in this header file to simplify
 * @param numberOfAverages The number of desired "readings" to be averaged (This will be different than the number of samples taken if additional resolution is selected). This number must be 16 or greater, must not be 65536 or bigger and must be a multiple of 16.
 * @param formatFunction Function pointer that will format the raw A2D values, the function must accept an integer representing the raw A2D value, it should also return the formatted value as an integer
 * @param preFunction Function pointer that will run just as the channel is starting to be scanned (eg switched pin turning on), the function must accept an integer indicating A2D channel, it should not return a value
 * @param postFunction Function pointer that will run just as the channel is finished being scanned (eg switched pin turning off), the function must accept an integer indicating A2D channel, it should not return a value
 * @param finishedFunction Function pointer that will run when the sampling/averaging is completed and a new value is ready (eg functions that need to run as soon as a sample is ready), the function must accept an integer indicating A2D channel, it should not return a value
 * @param inspectionFunction Allows the interception of the raw A2D values before they are summed and averaged. Only allows access to the current buffer (max 16 intergers). Useful for cherry picking values, or for example generating truly random numbers. The function should accept an integer (A2D channel, volatile unsigned int (pointer to A2D buffer), another integer (size of buffer, typically 16), and must return an integer
 * @param sampleSize This value is used to specify how many samples are to be taken in a single burst. This is useful if you only want a limited sample size, some possible uses are with the CTMU module (only the first reading is valid). The number is 0 centered (16-samples should be a value of 15), because of this the enum in the A2D.h header file is provided to simplify this
 * @param triggerType This takes one of two arguments, either AUTO_CONVERSION or MANUAL_CONVERSION. Auto conversion is the default and the A2D routine will take care of everything, Manual however turns that control over to an external function
 *  * @return 1 = Channel was updated successfully, 0 = Value out of range, no changes were made
 */
 int A2D_Advanced_Channel_Settings(int channel, enum RESOLUTION desiredResolutionIncrease, int numberOfAverages, int (*formatPointer)(int), void (*preFunction)(int), void (*postFunction)(int), void (*finishedFunction)(int), int (*inspectionFunction)(int, volatile unsigned int *, int), enum A2D_SAMPLE_SIZE sampleSize, int triggerType);

/**
 * Adds the channel to the scanning queue
 * @param channel The channel that is to be added to the scanning queue, these are declared in the controller config file
 * @return 1 = Success, 0 = Failure - Channel out of range
 */
int A2D_Add_To_Scan_Queue(int channel);

/**
 * Calculates and updates averaged/formatted result, through the magic of DSP it will also increase the resolution if required
 * Calling this function multiple times will only do something if a conversion has completed
 * Calling this function will start a conversion if it is not already converting
  */
void A2D_Routine(uint32_t time_mS);

/**
 * Returns the current value of the selected channel (Optionally formatted)
 * @param channel The analog channel that you require the formatted value of, these are declared in the controller config file
 * @return The formatted A2D values
 */
int A2D_Value(int channel);

#endif
