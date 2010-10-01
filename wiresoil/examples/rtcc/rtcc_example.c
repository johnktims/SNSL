#include "pic24_all.h"
#include "rtcc.h"

int main(void)
{
	__builtin_write_OSCCONL(OSCCON | 0x02);
	configBasic(HELLO_MSG);
	u_RTCC ur = RTCC_GetDateFromUser();
	RTCC_Set(ur);

	while(1)
    {
		while(!RCFGCALbits.RTCSYNC)
		{
    		doHeartbeat();
        }
		RTCC_Get();
		RTCC_Print(ur);
		DELAY_MS(30);
	}
}
