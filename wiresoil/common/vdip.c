/***********************************************************
 * Includes
 **********************************************************/
#include "pic24_all.h"
#include "spi.h"
#include "vdip.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


/***********************************************************
 * Macro Definitions
 **********************************************************/

// Is this a DEBUG build?
#ifndef DEBUG
#define DEBUG 0
#endif // DEBUG

// Print output if this is a DEBUG build
#if DEBUG
#define DEBUG_OUT(msg) \
    (puts("`"msg"`"))
#else
#define DEBUG_OUT(msg)
#endif

// Remove leading newline characters
#define REMOVE_LEADING_NEWLINES(c) \
    while((c) == LF)               \
    {                              \
        c = SPI_ReadWait();        \
    }


/***********************************************************
 * Function Definitions
 **********************************************************/

//**********************************************************
/**
 * @brief Initialize SPI and then Reset the VDIP
 */
//**********************************************************
void VDIP_Init(void)
{
    DEBUG_OUT("VDIP_Init: Started.");

    CONFIG_RESET();
    
    SPI_Init();

    VDIP_Reset();
	
    // Syncs VDIP with PIC
    VDIP_Sync();

    // Put vdip in short command mode
    VDIP_SCS();

    DEBUG_OUT("VDIP_Init: Finished.");
}


//**********************************************************
/**
 * @brief Sync the VDIP with the PIC. 'E' is sent to the
 *        VDIP, and then the PIC just reads until it gets
 *        the E back.
 * @return uint8 1 if everything syncs correctly
 * @todo Add some more error checking to verify that the PIC
 *       has failed to sync with the VDIP.
 */
//**********************************************************
uint8 VDIP_Sync(void)
{
	DEBUG_OUT("VDIP_Sync: Started.");

	// Initialize the sync by sending and E
	SPI_WriteStr("E");
	DELAY_MS(10);

    char c = SPI_ReadWait();
    
    REMOVE_LEADING_NEWLINES(c);

    // Wait for VDIP to send an 'E'
	while (c != 'E')
    {
        c = SPI_ReadWait();
        //putchar(c);
    }

    DEBUG_OUT("VDIP_Sync: Finished.");
    return 1;
}


//**********************************************************
/**
 * @brief Put VDIP in Short Command Set mode.
 * @return uint8 Are we in SCS mode?
 */
//**********************************************************
uint8 VDIP_SCS(void)
{
    DEBUG_OUT("VDIP_SCS: Started. Setting"
	          " Short Command Set");
	          
	//VDIP_Sync();

	SPI_WriteStr(SCS);
	char c = SPI_ReadWait();
	
	REMOVE_LEADING_NEWLINES(c);
	
	if(c == EOC)
	{
	    DEBUG_OUT("VDIP_SCS: Finished."
	              " In Short Command Set");
	    return 1;
	}

    DEBUG_OUT("VDIP_SCS: Finished."
              " NOT In Short Command Set");
	return 0;
}


//**********************************************************
/**
 * @brief Reset the VDIP
 */
//**********************************************************
void VDIP_Reset(void)
{
    DEBUG_OUT("VDIP_Reset: Started.");

    RESET = 0;
    DELAY_MS(100);
    RESET = 1;
    DELAY_MS(100);

    DEBUG_OUT("VDIP_Reset: Finished.");
}


//**********************************************************
/**
 * @brief List the files and folders in the current
 *        directory
 * @return A character array of file and directory names.
 * @see VDIP_CleanupDirList
 */
//**********************************************************
char** VDIP_ListDir(void)
{
    DEBUG_OUT("VDIP_ListDir: Started.");

	//VDIP_Sync();
	    
    // Get the number of items in the directory
    uint32 u32_items = VDIP_DirItemCount();

    // Allocate memory for the array of pointers
    char **data = (char**)malloc(sizeof(char*) * (u32_items + 1));
    
    // Null terminate the array so it will be easy to traverse
    data[u32_items] = '\0';
    uint32 u32_index = 0;
    
    // Allocate memory for the filenames. This could have
    // been done in one call, but contiguous memory space
    // may be sparse.
    for(u32_index = 0; u32_index < u32_items; u32_index++)
    {
        data[u32_index] = (char*)malloc(sizeof(char) * MAX_FILENAME_LEN);
    }

    // Request a directory listing
    SPI_Write(DIR);
    SPI_Write(CR);

    char c = SPI_ReadWait();

    REMOVE_LEADING_NEWLINES(c);

    uint32 u32_row = 0,
           u32_col = 0;

    // Get the file names from the VDIP
    while(c != EOC)
    {
        // File names end with a line feed character
        if(c == LF)
        {
            data[u32_row][u32_col] = '\0';
            ++u32_row;
            u32_col = 0;
        }
        else
        {
            data[u32_row][u32_col++] = c;
        }
        c = SPI_ReadWait();
    }

    DEBUG_OUT("VDIP_ListDir: Finished.");
    return data;
}


//**********************************************************
/**
 * @brief Clean up a multi-dimensional array returned
 *        from VDIP_ListDir
 * @see VDIP_ListDir
 */
//**********************************************************
void VDIP_CleanupDirList(char **data)
{
    uint32 u32_index = 0;
    while(data[u32_index] != '\0')
    {
        free(data[u32_index++]);
    }
    free(data);
}


//**********************************************************
/**
 * @brief Return the number of files and folders in the
 *        current directory
 * @return The number of files and folders in the current
 *         directory.
 */
//**********************************************************
uint32 VDIP_DirItemCount(void)
{
    DEBUG_OUT("VDIP_DirItemCount: Started.");
    
	VDIP_Sync();

    // Request a directory listing
    SPI_Write(DIR);
    SPI_Write(CR);

    char c = SPI_ReadWait();
    
    REMOVE_LEADING_NEWLINES(c);
    
    // This will contain the number of files and
    // folders in the current directory.
    uint32 u32_items = 0;
    
    // Get the file names from the VDIP
    while(c != EOC)
    {
        // All file names are delimited by carriage
        // returns, so just count them.
        if(c == LF)
        {
            ++u32_items;
        }
        c = SPI_ReadWait();
    }

    DEBUG_OUT("VDIP_DirItemCount: Finished.");
    return u32_items;
}

//**********************************************************
/**
 * @brief Find the size of the given file
 * @param[in] name The name of the file
 * @return uint8 The size of the file in bytes
 */
//**********************************************************
uint32 VDIP_FileSize(const char *name)
{
    DEBUG_OUT("VDIP_FileSize: Started.");
    
	//VDIP_Sync();

    // Make a DIR request
    SPI_Write(DIR);
    SPI_Write(SPACE);
    SPI_WriteStr(name);

    // The returned data will have this format:
    // FILENAME.EXT XXXX, where XXXX are four
    // bytes telling the size of the file

    // Parse the string
    uint32 u32_size = 0;
    char ch_prior_space = 0,
         ch_shift = 0,
         c = SPI_ReadWait();

    while(c != EOC)
    {
        if(c == SPACE)
        {
            ch_prior_space = 1;
        }
        else
        {
            if(ch_prior_space)
            {
                // We have passed the space, start
                // to shift in the four bytes.
                u32_size |= (c << ch_shift++);
                ch_shift *= 8;
            }
        }

        c = SPI_ReadWait();
    }

    DEBUG_OUT("VDIP_FileSize: Finished.");
    return u32_size;
}


//**********************************************************
/**
 * @brief Read a file into a string
 * @param[in] name The name of the file
 * @return char* A string containing the contents
 *               of the file
 */
//**********************************************************
char* VDIP_ReadFile(const char *name)
{
    DEBUG_OUT("VDIP_ReadFile: Started.");
	
	//VDIP_Sync();

    uint32 u32_bytes = VDIP_FileSize(name) + 1,
           u32_index = 0;
    char *data = (char*)malloc(u32_bytes);

    //printf("ReadFile->FileSize = `%u`\n", (unsigned)u32_bytes);

    SPI_Write(RD);
    SPI_Write(SPACE);
    SPI_WriteStr(name);
    
    char c = SPI_ReadWait();
    
    REMOVE_LEADING_NEWLINES(c);
    
    while(c != EOC)
    {
        data[u32_index++] = c;
        c = SPI_ReadWait();
    }
    data[u32_index] = '\0';

    DEBUG_OUT("VDIP_ReadFile: Finished.");
    return data;
}


//**********************************************************
/**
 * @brief Open a file for writing
 * @param[in] name The name of the file
 * @param[in] data The data to write
 */
//**********************************************************
void VDIP_WriteFile(const char *name, const char *data)
{
    DEBUG_OUT("VDIP_WriteFile: Started.");

    uint32 u32_size  = strlen(data);

    VDIP_Sync();

  	DEBUG_OUT("Put in Ascii mode");
    //SPI_Write(IPA);
    SPI_Write(IPH);
    SPI_Write(CR);

    DEBUG_OUT("Open the file for writing");
    SPI_Write(OPW);
    SPI_Write(SPACE);
    SPI_WriteStr(name);
    VDIP_Sync();

    // Tell the VDIP how much data to expect
    SPI_Write(WRF);
    SPI_Write(SPACE);

	// Shift the bytes in, MSB first
    int8 i8_byte  = 0;
    int32 u32_index;
    for(u32_index = 24; u32_index >= 0; u32_index -= 8)
    {
        i8_byte = (int8)(u32_size >> u32_index);
        //printf("WRF: %d/%u\n", i8_byte, (unsigned)u32_size);
        SPI_Write(i8_byte);
    }
    SPI_Write(CR);

    DEBUG_OUT("Write the actual message.");
    SPI_WriteStr(data);
    DELAY_MS(100);

    DEBUG_OUT("Close the file");
    SPI_Write(CLF);
    SPI_Write(SPACE);
    SPI_WriteStr(name);
    DELAY_MS(100);
    VDIP_Sync();

    DEBUG_OUT("VDIP_WriteFile: Finished.");
}
