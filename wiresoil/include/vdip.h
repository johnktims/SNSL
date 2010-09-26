#ifndef _VDIP1_FUNC_H_
#define _VDIP1_FUNC_H_


/***********************************************************
 * Pin Mappings
 **********************************************************/

// Reset pin on the VDIP
#define RESET _LATB13

#define CONFIG_RESET() CONFIG_RB13_AS_DIG_OUTPUT()


/****************************************
 * Common Characters
 ***************************************/
 
// Carriage Return
#define CR 0xd

// Carriage Return
#define LF 0xa

// DIR command
#define DIR 0x1

// RD: Read a file
#define RD 0x4

// WRF: Write File command
#define WRF 0x8

// CLF: Close File command
#define CLF 0xA

// OPR: Open file for reading
#define OPR 0xE

// OPW: Open file for writing
#define OPW 0x9

// IPA: Ascii
#define IPA 0x90

// IPA: Binary
#define IPH 0x91

// Space
#define SPACE 0x20

// End of Command
#define EOC '>'

// SCS: Short Command Set
#define SCS "SCS"

// The largest filename is 12 characters
#define MAX_FILENAME_LEN 12+1


/****************************************
 * Function Definitions
 ***************************************/

void  VDIP_Init(void);
void  VDIP_Reset(void);
void  VDIP_Sync_E(void);
uint8 VDIP_Sync(void);
uint8 VDIP_SCS(void);

void  VDIP_WriteFile(const char *,
                     const char *);
char* VDIP_ReadFile(const char *);

uint32 VDIP_FileSize(const char *);
uint32 VDIP_DirItemCount(void);

char** VDIP_ListDir(void);
void   VDIP_CleanupDirList(char **);

#endif
