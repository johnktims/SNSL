#include "pic24_all.h"
#include "vdip.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    //__builtin_write_OSCCONL(OSCCON | 0x02);
    configBasic(HELLO_MSG);

	//config SPI for VDIP1
	VDIP_Init();

	//syncs VDIP with PIC
	VDIP_Sync();

	// put vdip in short command mode
	VDIP_SCS();
	puts("FERTIG.\n");
	while(1){}
	return 0;
}
