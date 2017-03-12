#ifndef VARIABLE_TRANSFORMER_H
#define VARIABLE_TRANSFORMER_H
/***********Add to config file header************/
/*
//Variable_Transformer Library
#define VARIABLE_TRANSFORMER_MAJOR	0
#define VARIABLE_TRANSFORMER_MINOR	1
#define VARIABLE_TRANSFORMER_PATCH	0
*/

/***************Add to config file***************/
/*
#ifndef VARIABLE_TRANSFORMER_LIBRARY
	#error "You need to include the Variable_Transformer library for this code to compile"
#endif
 */

/*************Semantic  Versioning***************/
#define VARIABLE_TRANSFORMER_LIBRARY

/*************Library Dependencies***************/
/*************   Magic  Numbers   ***************/
/*************    Enumeration     ***************/
/*************    Structures      ***************/
/***********State Machine Definitions************/
/*************Function  Prototypes***************/
void Initialize_Variable_Transformer(void);
void Variable_Transformer_Routine(unsigned long time_mS);

#endif
