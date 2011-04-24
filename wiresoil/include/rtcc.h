#ifndef RTCC_H
#define RTCC_H

#include "snsl.h" // RTCC union


/***********************************************************
 * Function Headers
 ***********************************************************/
uint8  RTCC_GetBCDValue(char *);
uint8  RTCC_ParseVal(char *);
void   RTCC_GetDateFromUserNoWday(RTCC *);
void   RTCC_GetDateFromUser(RTCC *);
void   RTCC_SetDefaultVals(RTCC *);
void   RTCC_Set(RTCC *);
void   RTCC_Read(RTCC *);
void   RTCC_Print(RTCC *);

#endif //RTCC_H
