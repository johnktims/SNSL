#ifndef SNSL_H
#define SNSL_H

#include "pic24_all.h"
#include "vdip.h"

#define MAX_NODE_ADDR_LEN 3+1
#define MAX_NODE_NAME_LEN 12+1
#define FILE_NODES "NODES.TXT"

// RTSP
typedef struct _REC {
  char node_name[MAX_NODE_NAME_LEN];
}REC;

#define LAST_IMPLEMENTED_PMEM 0x00ABFF
#define DATA_FLASH_PAGE (((LAST_IMPLEMENTED_PMEM/FLASH_PAGESIZE)*FLASH_PAGESIZE)-FLASH_PAGESIZE)
#define NUM_ROWS (((sizeof(REC))/FLASH_ROWBYTES) + 1)
#define FLASH_DATA_SIZE (NUM_ROWS*FLASH_ROWBYTES)

typedef union _UFDATA{
  REC  dat;
  
  // Worst case allocates extra row, but
  // ensures RAM data block is multiple
  // of row size
  char fill[FLASH_DATA_SIZE];
}UFDATA;

// Functions
char** SNSL_ParseNodeNames(void);
char*  SNSL_GetNodeName(void);
void   SNSL_SetNodeName(char *);

#endif // SNSL_H
