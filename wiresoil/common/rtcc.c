/***********************************************************
 * Includes
 **********************************************************/
#include "pic24_all.h"
#include "rtcc.h"

//for printf
#include<stdio.h>

/***********************************************************
 * Function Definitions
 **********************************************************/

//**********************************************************
/**
 * @brief Get BCD value from user input
 * @param[in] Character string to display to user
 * @return uint8 BCD value of user input
 */
//**********************************************************

uint8 RTCC_GetBCDValue(char *sz_1) {
	char sz_buff[3];
	uint16 u16_bin;
	uint8 u8_bcd;
	outString(sz_1);
	inStringEcho(sz_buff, 2);
	sscanf(sz_buff, "%d", (int *)&u16_bin);
	u8_bcd = u16_bin/10;
	u8_bcd <<= 4;
	u8_bcd  |= (u16_bin % 10);
	return u8_bcd;
}

//**********************************************************
/**
 * @brief Convert given character string to BCD
 * @param[in] Character string
 * @return uint8 BCD value of given character string
 */
//**********************************************************

uint8 RTCC_ParseVal(char *sz_1) {
    uint16 u16_bin;
    uint8 u8_bcd;
    sscanf(sz_1, "%d", (int *)&u16_bin);
    u8_bcd = u16_bin/10;
    u8_bcd <<= 4;
    u8_bcd = u8_bcd | (u16_bin % 10);
    return u8_bcd;
}

//**********************************************************
/**
 * @brief Get initial date settings from user
 * @param[in] Pointer to global unionRTCC struct
 */
//**********************************************************

void RTCC_GetDateFromUser(unionRTCC *ur) {
    
	ur->u8.yr    = RTCC_GetBCDValue("\nEnter year (00-99): ");
	ur->u8.month = RTCC_GetBCDValue("\nEnter month (01-12): ");
	ur->u8.date  = RTCC_GetBCDValue("\nEnter day of month (01-31): ");
	ur->u8.wday  = RTCC_GetBCDValue("\nEnter week day (0 (Sunday)-6 (Saturday)): ");
	ur->u8.hour  = RTCC_GetBCDValue("\nEnter hour (00-23): ");
	ur->u8.min   = RTCC_GetBCDValue("\nEnter min (00-59): ");
	ur->u8.sec   = RTCC_GetBCDValue("\nEnter sec(00-59): ");
}

void RTCC_GetDateFromUserNoWday(unionRTCC *ur) {
    ur->u8.yr    = RTCC_GetBCDValue("\nEnter year (00-99): ");
	ur->u8.month = RTCC_GetBCDValue("\nEnter month (01-12): ");
	ur->u8.date  = RTCC_GetBCDValue("\nEnter day of month (01-31): ");
	ur->u8.wday  = RTCC_ParseVal("0");
	ur->u8.hour  = RTCC_GetBCDValue("\nEnter hour (00-23): ");
	ur->u8.min   = RTCC_GetBCDValue("\nEnter min (00-59): ");
	ur->u8.sec   = RTCC_GetBCDValue("\nEnter sec(00-59): ");
}    

//**********************************************************
/**
 * @brief Set default RTCC values
 * @param[in] Pointer to global unionRTCC struct
 */
//**********************************************************

void RTCC_SetDefaultVals(unionRTCC *ur) {
    //default time: 11/1/2010 12:00:00 am
    ur->u8.yr    = RTCC_ParseVal("10");
    ur->u8.month = RTCC_ParseVal("11");
    ur->u8.date  = RTCC_ParseVal("1");
    ur->u8.wday  = RTCC_ParseVal("1");
    ur->u8.hour  = RTCC_ParseVal("0");
    ur->u8.min   = RTCC_ParseVal("0");
    ur->u8.sec   = RTCC_ParseVal("0");
}    

//**********************************************************
/**
 * @brief Copy values in unionRTCC struct to RTCC regs
 * @param[in] Pointer to global unionRTCC struct
 */
//**********************************************************

void RTCC_Set(unionRTCC *ur) {
	uint8 u8_i;
	__builtin_write_RTCWEN();
	RCFGCALbits.RTCEN = 0;
	RCFGCALbits.RTCPTR = 3;
	for(u8_i = 0; u8_i < 4; ++u8_i)
	{
    	RTCVAL = ur->regs[u8_i];
    }   	
	RCFGCALbits.RTCEN = 1;
	RCFGCALbits.RTCWREN = 0;
}

//**********************************************************
/**
 * @brief Copy values from RTCC regs to unionRTCC struct for output
 * @param[in] Pointer to global unionRTCC struct
 */
//**********************************************************

void RTCC_Read(unionRTCC *ur) {
	uint8 u8_i;
	RCFGCALbits.RTCPTR = 3;
	for(u8_i = 0; u8_i < 4; u8_i++)
	{
		ur->regs[u8_i] = RTCVAL;
	}
}

//**********************************************************
/**
 * @brief Print formatted RTCC data from unionRTCC struct to screen
 * @param[in] Pointer to global unionRTCC struct
 */
//**********************************************************

void RTCC_Print(unionRTCC *ur) {
	printf("[%2x/%2x/%2x] %02x:%02x:%02x \n",
		(uint16)ur->u8.month,  (uint16)ur->u8.date,
		(uint16)ur->u8.yr, (uint16)ur->u8.hour,
		(uint16)ur->u8.min,  (uint16)ur->u8.sec);
}
