#include "pic24_all.h"
#define VDIP_DEBUG 1
#include "vdip.h"
#include "snsl.h"

int main(void)
{
    __builtin_write_OSCCONL(OSCCON | 0x02);
    configBasic(HELLO_MSG);
	outString("Hello world");
	
	uint8 au8_search_node[] = {0x00,
    	                       0x32,
    	                       0x64,
    	                       0};
	CONFIG_VDIP_POWER();
	_LATB7 = 0;

	//config SPI for VDIP1
	VDIP_Init();

    /*
	VDIP_PrintListDir();

    VDIP_WriteFile("LOG1.TXT", "This is a test.");
    
    VDIP_PrintListDir();
    
    VDIP_DeleteFile("LOG1.TXT");
    */

    uint8 hops=2, timeout=4, max = 5;
    //VDIP_PrintListDir();

    SNSL_PrintConfig();

    // Merge the Ascii and Binary files so that any new ascii values
    // are initialized to 0 and any existing names use their current
    // values in the binary config file.
    POLL *polls = SNSL_MergeConfig();
    
    // Find a node and update its number of attempts
    uint8 u8_search = SNSL_SearchConfig(au8_search_node, polls);
    polls[u8_search].attempts += 25;

    // Write the new config file out
    SNSL_WriteConfig(hops, timeout, max, polls);

    SNSL_PrintConfig();
    //VDIP_PrintListDir();

	outString("FERTIG.\n");
	while(1){}
	return 0;
}
