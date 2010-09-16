#include "pic24_all.h"
#include "spi.h"
#include "vdip.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    __builtin_write_OSCCONL(OSCCON | 0x02);
    configBasic(HELLO_MSG);

	//config SPI for VDIP1
	VDIP_Init();

	//syncs VDIP with PIC			
	VDIP_Sync();

	// put vdip in short command mode
	VDIP_SCS();

	VDIP_Sync();

	puts("##SYNC FUNCTION\n");
    SPI_Write(0x90); //ipa
    SPI_Write(0x0d); // cr

    puts("##OPEN LOG.TXT\n");
    SPI_Write(0x09);  //OPW
    SPI_Write(0x20); // space
    SPI_WriteStr("log.txt");
    VDIP_Sync();

    puts("##WRITE TO LOG.TXT\n");
    SPI_Write(0x08); //WRF
    SPI_Write(0x20); // space
    SPI_Write(4);
    SPI_Write(0x0d); // CR

    puts("##WRITE MESSAGE\n");
    SPI_WriteStr("Four");
    DELAY_MS(100);

    puts("##CLOSE FILE\n");
    SPI_Write(0x0a); //CLF
    SPI_Write(0x20); //space
    SPI_WriteStr("LOG.TXT");
    VDIP_Sync();
    DELAY_MS(1000);

    VDIP_WriteFile("TEST.TXT", "This is a test. YAY! This sentence needs to be over two hundred and fifty six characters long. I hope I have achieved this. This is a test. YAY! This sentence needs to be over two hundred and fifty six characters long. I hope I have achieved this. This is a test. YAY! This sentence needs to be over two hundred and fifty six characters long. I hope I have achieved this.");

    puts("FERTIG.\n");
    while(1){}
	
	return 0;
	
	
	/*** WORKING
	SPI_WriteStr("ECS");
	
	VDIP_Sync();
	
	puts("##SYNC FUNCTION\n");
    SPI_WriteStr("IPA");
    DELAY_MS(100);
    VDIP_Sync();
    
    puts("##OPEN LOG.TXT\n");
    SPI_WriteStr("OPW log.txt");
    DELAY_MS(100);
    VDIP_Sync();
    //VDIP_CleanupAfterCommand();

    puts("##WRITE TO LOG.TXT\n");
    SPI_WriteStr("WRF 4");
    SPI_Write('F');
    SPI_Write('o');
    SPI_Write('u');
    SPI_Write('r');
    DELAY_MS(100);
    //VDIP_CleanupAfterCommand();
    
    puts("##CLOSE FILE\n");
    SPI_WriteStr("CLF LOG.TXT");
    VDIP_Sync();
    DELAY_MS(5000);
    
    puts("FERTIG.\n");
    //VDIP_CleanupAfterCommand();
    while(1){}
	
	return 0;
	******/
	
	/*
	// Read a new file
	printf("FileSize: `%u`\n", (unsigned)VDIP_FileSize("FUN.TXT"));
	char *str = VDIP_ReadFile("TEST1.TXT");
	printf("`TEST1.TXT` = `%s`\n", str);
	free(str);

	// Get the size of the file
	VDIP_FileSize("TEST1.TXT");
	printf("FILE SIZE: `%u`\n",
			(unsigned)VDIP_FileSize("FUN.TXT"));
	printf("FILE SIZE: `%u`\n",
			(unsigned)VDIP_FileSize("TEST1.TXT"));

	printf("DIR ITEMS: `%u`\n",
			(unsigned)VDIP_DirItems());

	printf("DIR ITEMS: `%u`\n",
			(unsigned)VDIP_DirItems());

	char **data = VDIP_ListDir();
	uint32 u32_index = 0;
	while(data[u32_index] != '\0')
	{
		printf("`%u`:`%s`\n", (unsigned)u32_index, data[u32_index]);
		++u32_index;
	}
	VDIP_CleanupDirList(data);
    */

// {{{
    char *fp = "TEST1.TXT";
    char *str = NULL;
    /*
    
    char *str = VDIP_ReadFile(fp);
	printf("BEFORE: TEST1.TXT = `%s`\n", str);
	free(str);
    */
    
	puts("OUTPUT: `");
	VDIP_ReadFile(fp);
	printf("`\n Size: `%u`\n", (unsigned)VDIP_FileSize(fp));
	
	str = VDIP_ReadFile(fp);
	printf("AFTER: TEST1.TXT = `%s`\n", str);
	free(str);
// }}}

    	
	char **data = VDIP_ListDir();
	uint32 u32_index = 0;
	while(data[u32_index] != '\0')
	{
		printf("`%u`:`%s`\n", (unsigned)u32_index, data[u32_index]);
		++u32_index;
	}
	VDIP_CleanupDirList(data);
	
	puts("FERTIG.\n");
	while(1){}
	return 0;
}
