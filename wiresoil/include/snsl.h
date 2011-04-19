#ifndef SNSL_H
#define SNSL_H

#include "pic24_all.h"
#include "vdip.h"

#define HEADER_LEN         (6)
#define MAX_NODE_ADDR_LEN  (3+1)
#define MAX_NODE_NAME_LEN  (12+1)
#define FILE_NODES         ((uint8*)"NODES.TXT")
#define FILE_CONFIG        ((uint8*)"CONFIG.TXT")
#define FILE_LOG           ((uint8*)"LOG.TXT")
#define DEFAULT_NODE_NAME  ((uint8*)"default")

#define LAST_POLL_FLAG     (0xff)
#define UNKNOWN_NODE       (0xff)

#define MAX_STORED_SAMPLES (5)
#define NUM_ADC_PROBES     (10)

#define MESH_SLEEP_MINS    (5)

#define STATUS_REPLACEABLE (0)
#define STATUS_IN_USE      (1)
#define STATUS_INVALID     (MAX_STORED_SAMPLES + 1)


/***********************************************************
 * RTSP Structures
 **********************************************************/
typedef struct _REC
{
    uint8 node_name[MAX_NODE_NAME_LEN];
} REC;

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
} UFDATA;


/***********************************************************
 * Structures
 **********************************************************/
typedef struct _POLL
{
    uint8 name[MAX_NODE_ADDR_LEN],
          attempts;
} POLL;

typedef union _unionRTCC
{
    struct   //four 16 bit registers
    {
        uint8 yr;
        uint8 null;
        uint8 date;
        uint8 month;
        uint8 hour;
        uint8 wday;
        uint8 sec;
        uint8 min;
    } u8;
    uint16 regs[4];
} unionRTCC;

typedef union _FLOAT
{
    float f;
    char s[sizeof(float)];
} FLOAT;

typedef struct _STORED_SAMPLE
{
    FLOAT samples[NUM_ADC_PROBES];
    unionRTCC ts;
    int8 status;
} STORED_SAMPLE;

/***********************************************************
 * Function Headers
 **********************************************************/
void    SNSL_ConfigLowPower(void);

uint8 **SNSL_ParseNodeNames(void);
void    SNSL_GetNodeName(UFDATA *);
void    SNSL_SetNodeName(uint8 *);
void    SNSL_PrintNodeName(void);

void    SNSL_CreateDefaultConfig(void);
void    SNSL_CreateDefaultNodes(void);

POLL   *SNSL_ParseConfig(uint8 *, uint32 *, uint8 *);
void    SNSL_WriteConfig(uint8, uint32, uint8, POLL *);
POLL   *SNSL_MergeConfig(void);
void    SNSL_PrintConfig(void);
uint8   SNSL_SearchConfig(uint8 *, POLL *);
void    SNSL_ParseConfigHeader(uint8 *, uint32 *, uint8 *);

void    SNSL_LogPollEvent(uint8, unionRTCC *);
void    SNSL_LogNodeSkipped(uint8, uint8, uint8, unionRTCC *);
void    SNSL_LogResponseFailure(uint8, uint8, uint8, uint8, unionRTCC *);
void    SNSL_LogPollingStats(unionRTCC *, uint8, uint8, uint8);

uint32  SNSL_Pow(uint8, uint8);
uint32  SNSL_Atoi(uint8 *);
void    SNSL_PrintPolls(POLL *);

void    SNSL_InitSamplesBuffer(STORED_SAMPLE *);
int     SNSL_TimeToSec(unionRTCC);
int     SNSL_TimeDiff(unionRTCC, unionRTCC);

uint8   SNSL_TotalByStatus(STORED_SAMPLE *, uint8);
uint8   SNSL_TotalSamplesInUse(STORED_SAMPLE *);
uint8   SNSL_TotalReplaceableSamples(STORED_SAMPLE *);

uint8   SNSL_FirstReplaceableSample(STORED_SAMPLE *);
uint8   SNSL_NewestSample(STORED_SAMPLE *);
uint8   SNSL_OldestSample(STORED_SAMPLE *);

void    SNSL_InsertSample(STORED_SAMPLE *, STORED_SAMPLE);
void    SNSL_PrintSamples(STORED_SAMPLE *);
uint8   SNSL_ACKSample(STORED_SAMPLE *, unionRTCC);

#endif // SNSL_H
