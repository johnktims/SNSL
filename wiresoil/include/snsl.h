#ifndef SNSL_H
#define SNSL_H

#include "pic24_all.h"
#include "vdip.h"

#define HEADER_LEN         0x6
#define MAX_NODE_ADDR_LEN  (3+1)
#define MAX_NODE_NAME_LEN  (12+1)
#define FILE_NODES         "NODES.TXT"
#define FILE_CONFIG        "CONFIG.TXT"

#define LAST_POLL_FLAG     0xff
#define UNKNOWN_NODE       0xff


/***********************************************************
 * RTSP Structures
 **********************************************************/ 
typedef struct _REC
{
  uint8 node_name[MAX_NODE_NAME_LEN];
}REC;

#define LAST_IMPLEMENTED_PMEM 0x00ABFF
#define DATA_FLASH_PAGE (((LAST_IMPLEMENTED_PMEM/FLASH_PAGESIZE)*FLASH_PAGESIZE)-FLASH_PAGESIZE)
#define NUM_ROWS (((sizeof(REC))/FLASH_ROWBYTES) + 1)
#define FLASH_DATA_SIZE (NUM_ROWS*FLASH_ROWBYTES)

typedef union _UFDATA
{
    REC  dat;

    // Worst case allocates extra row, but
    // ensures RAM data block is multiple
    // of row size
    uint8 fill[FLASH_DATA_SIZE];
}UFDATA;


/***********************************************************
 * Structures
 **********************************************************/ 
typedef struct _POLL
{
    uint8 name[MAX_NODE_ADDR_LEN],
          attempts;
}POLL;

typedef union _unionRTCC {
    struct { //four 16 bit registers
                uint8 yr;
                uint8 null;
                uint8 date;
                uint8 month;
                uint8 hour;
                uint8 wday;
                uint8 sec;
                uint8 min;
    }u8;
    uint16 regs[4];
}unionRTCC;

/***********************************************************
 * Function Definitions
 **********************************************************/
uint8** SNSL_ParseNodeNames(void);
void    SNSL_GetNodeName(UFDATA *);
void    SNSL_SetNodeName(uint8 *);
void    SNSL_PrintNodeName(void);

POLL*   SNSL_ParseConfig(uint8 *, uint32 *, uint8 *);
void    SNSL_WriteConfig(uint8, uint32, uint8, POLL *);
POLL*   SNSL_MergeConfig(void);
void    SNSL_PrintConfig(void);
uint8   SNSL_SearchConfig(uint8 *, POLL *);
void    SNSL_ParseConfigHeader(uint8 *, uint32 *, uint8 *);

void    SNSL_logPollEvent(unionRTCC *, uint8);
void    SNSL_logNodeSkipped(uint8, uint8, uint8, unionRTCC *);
void    SNSL_logResponseFailure(uint8, uint8, uint8, unionRTCC *);

uint32  SNSL_pow(uint8, uint8);

#endif // SNSL_H
