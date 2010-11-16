#ifndef RTCC_H
#define RTCC_H

//for unionRTCC
#include "snsl.h"

/***********************************************************
 * Function Headers
 **********************************************************/

uint8   RTCC_GetBCDValue(char *);
uint8   RTCC_ParseVal(char *);
void    RTCC_GetDateFromUser(unionRTCC *);
void    RTCC_SetDefaultVals(unionRTCC *);
void    RTCC_Set(unionRTCC *);
void    RTCC_Read(unionRTCC *);
void    RTCC_Print(unionRTCC *);

#endif //RTCC_H
