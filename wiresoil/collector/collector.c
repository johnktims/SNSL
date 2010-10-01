#define DEBUG 1

#include "pic24_all.h"
#include "snsl.h"
#include <stdio.h>

int main(void)
{
    // __builtin_write_OSCCONL(OSCCON | 0x02);
    configBasic(HELLO_MSG);

	// Config SPI for VDIP1
	VDIP_Init();
	
	VDIP_Sync();
	char **data = SNSL_ParseNodeNames();
	uint32 u32_index = 0;
	uint8 u8_index = 0;
	while(data[u32_index] != '\0')
	{
    	for(u8_index = 0; u8_index < 3; ++u8_index)
    	{
		    printf("[`%u`,`%u`] =`0x%X`,`%d`,`%c`,`%u`\n",
		        (unsigned)u32_index, (unsigned)u8_index,
		        data[u32_index][u8_index],
		        data[u32_index][u8_index],
		        data[u32_index][u8_index],
		        data[u32_index][u8_index]);
		} 
		puts("\n");
		++u32_index;
	}
	VDIP_CleanupDirList(data);

	puts("FERTIG.\n");
	while(1){}
	return 0;
}
