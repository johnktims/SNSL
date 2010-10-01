#ifndef RTCC_H
#define RTCC_H

typedef union _u_RTCC
{
	struct
	{
		uint8 yr,
			  null,
			  date,
			  month,
			  hour,
			  wday,
			  sec,
			  min;
	}u8;
	uint16 regs[4];
}u_RTCC;

uint8 RTCC_GetBCDValue(char *);
u_RTCC RTCC_GetDateFromUser(void);
void RTCC_Set(u_RTCC);
u_RTCC RTCC_Get(void);
void RTCC_Print(u_RTCC);
#endif //RTCC_H
