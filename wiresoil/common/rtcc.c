#include "pic24_all.h"
#include "rtcc.h"
#include<stdio.h>

/*
Logging format

Node Addr(3), Node Name(12), TS(6), Temp Samps(5*2bytes),
redox samples(4*2), ref voltage(2)
*/

uint8 RTCC_GetBCDValue(char *sz_1) {
	char sz_buff[8];
	uint16 u16_bin;
	uint8 u8_bcd;
	outString(sz_1);
	inStringEcho(sz_buff, 7);
	sscanf(sz_buff, "%d", (int *)&u16_bin);
	u8_bcd = u16_bin/10;
	u8_bcd <<= 4;
	u8_bcd  |= (u16_bin % 10);
	return u8_bcd;
}


u_RTCC RTCC_GetDateFromUser(void)
{
    u_RTCC ur;
	ur.u8.yr    = RTCC_GetBCDValue("Enter year (0-99): ");
	ur.u8.month = RTCC_GetBCDValue("Enter month (1-12): ");
	ur.u8.date  = RTCC_GetBCDValue("Enter day of month (1-31): ");
	ur.u8.wday  = RTCC_GetBCDValue("Enter week day (0-6): ");
	ur.u8.hour  = RTCC_GetBCDValue("Enter hour (0-23): ");
	ur.u8.min   = RTCC_GetBCDValue("Enter min (0-59): ");
	ur.u8.sec   = RTCC_GetBCDValue("Enter sec(0-59): ");
	return ur;
}


void RTCC_Set(u_RTCC ur) {
	uint8 u8_i;
	__builtin_write_RTCWEN();
	RCFGCALbits.RTCEN = 0;
	RCFGCALbits.RTCPTR = 3;
	for(u8_i = 0; u8_i < 4; u8_i++)
	{
    	RTCVAL = ur.regs[u8_i];
    }   	
	RCFGCALbits.RTCEN = 1;
	RCFGCALbits.RTCWREN = 0;
}


u_RTCC RTCC_Get(void)
{
	u_RTCC ur;
	uint8 u8_i;
	RCFGCALbits.RTCPTR = 3;
	for(u8_i = 0; u8_i < 4; u8_i++)
	{
		ur.regs[u8_i] = RTCVAL;
	}
	return ur;
}


void RTCC_Print(u_RTCC ur)
{
	printf("day(wday)/mon/yr: %2x(%2x)/%2x/%2x, %02x:%02x:%02x \n",
		(uint16)ur.u8.date,  (uint16)ur.u8.wday,
		(uint16)ur.u8.month, (uint16)ur.u8.yr,
		(uint16)ur.u8.hour,  (uint16)ur.u8.min,
		(uint16)ur.u8.sec);
}
