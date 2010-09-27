#include "pic24_all.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    //__builtin_write_OSCCONL(OSCCON | 0x02);
    configBasic(HELLO_MSG);

	puts("FERTIG.\n");
	while(1){}
	return 0;
}
