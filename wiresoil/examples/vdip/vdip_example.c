#include "pic24_all.h"
#define VDIP_DEBUG 1
#include "vdip.h"
#include "snsl.h"


int main(void)
{
    __builtin_write_OSCCONL(OSCCON | 0x02);
    configBasic(HELLO_MSG);
    
	//outString("Hello world");
	
	uint8 au8_search_node[] = {0x00,
    	                       0x32,
    	                       0x64};
	CONFIG_VDIP_POWER();
	_LATB7 = 0;

	//config SPI for VDIP1
	VDIP_Init();
	
	outString("Node Addresses Loaded:\n");
	uint8 **polls = SNSL_ParseNodeNames();
	uint8 i = 0;
	while(polls[i])
	{
	    printf("%2x%2x%2x \n", polls[i][0], polls[i][1], polls[i][2]);
	//printf("%2x%2x%2x \n", polls[1][0], polls[1][1], polls[1][2]);*/
	    ++i;
	}
	
	return 0;
	
	uint8 hops, thresh;
	uint32 timeout;
	
	SNSL_ParseConfigHeader(&hops, &timeout, &thresh);
	printf("Config Values: \n Mesh Hops: %u \n Timeout: %lu \n Failure Limit: %u \n", hops, timeout, thresh);
	
	/*printf("ND: %d\n", VDIP_DiskExists());

    VDIP_PrintListDir();

    printf("FS: %d\n", VDIP_FileExists("NODES.TXT"));
    printf("FS: %u\n", VDIP_FileExists("NODES1.TXT"));
    printf("FS: %d\n", VDIP_FileExists("NODES.TXT"));
    
    printf("FS: %d\n", VDIP_FileExists(FILE_NODES));
    printf("FS: %d\n", VDIP_FileExists(FILE_CONFIG));
    if(!VDIP_FileExists(FILE_CONFIG))
    {
        puts("File Created");
        SNSL_CreateDefaultConfig();
    }
    else
    {
        puts("File Exists!");
    }    

    VDIP_PrintListDir();
        
    SNSL_PrintConfig();
    puts("BEREIT");
    while(1){}
    return 0;
    
    
	VDIP_PrintListDir();

    VDIP_WriteFile("LOG1.TXT", "This is a test.");
    
    VDIP_PrintListDir();
    
    VDIP_DeleteFile("LOG1.TXT");
    

    uint8 hops=2, max = 5;
    uint32 timeout=4;
    //VDIP_PrintListDir();

    SNSL_PrintConfig();
    
    SNSL_ParseConfigHeader(&hops, &timeout, &max);

    // Merge the Ascii and Binary files so that any new ascii values
    // are initialized to 0 and any existing names use their current
    // values in the binary config file.
    POLL *polls = SNSL_MergeConfig();

    // Find a node and update its number of attempts
    uint8 u8_search = SNSL_SearchConfig(au8_search_node, polls);
    polls[u8_search].attempts += 25;

    // Write the new config file out
    SNSL_WriteConfig(hops, timeout, max, polls);

	uint8 u8_i = 0;
	while(u8_i < 1)
    {
        SNSL_PrintConfig();
        ++u8_i;
    }

	outString("FERTIG.\n");*/
	while(1){}
	return 0;
}
