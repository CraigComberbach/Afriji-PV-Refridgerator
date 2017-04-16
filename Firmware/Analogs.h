#ifndef ANALOGS_H
#define	ANALOGS_H

int Afriji_Celcius_Formating(int raw);
int HiI_Formating(int value);
int LoI_Formating(int value);
int LoV_Formating_AN0(int value);
int LoV_Formating_AN1(int value);
int LoV_Formating_AN2(int value);
int HiV_Formating_AN12(int value);
int HiV_Formating_AN13(int value);
int HiV_Formating_AN14(int value);
int HiV_Formating_AN15(int value);

void Switched_Ground_On(int channel);
void Switched_Ground_Off(int channel);

#endif	/* ANALOGS_H */
