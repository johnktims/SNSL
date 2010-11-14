/***********************************************************
 * Includes
 **********************************************************/
#include "pic24_all.h"
#include "vdip.h"
#include "snsl.h"
#include <string.h> // strlen
#include <stdlib.h> // malloc; free

// for ssprintf
#include <stdio.h>


/***********************************************************
 * Function Definitions
 **********************************************************/

//**********************************************************
/**
 * @brief Retrieve NODES.TXT and parse the nodes' names
 * @return uint8** The nodes' names in hex
 */
//**********************************************************
uint8** SNSL_ParseNodeNames(void)
{
    if(!VDIP_FileExists(FILE_NODES))
    {
        SNSL_CreateDefaultNodes();
    }

    uint8 *psz_data  = VDIP_ReadFile(FILE_NODES);
    uint32 u32_size  = VDIP_FileSize(FILE_NODES),
           u32_index = 0;

    // Count the newlines to count the number
    // of nodes we need to allocate memory for
    uint32 u32_nodes = 0,
           u32_delimiter_width = 1;

    for(; u32_index < u32_size; ++u32_index)
    {
        // Take care of linux and windows newlines
        if(psz_data[u32_index] == '\r')
        {
            continue;
        }
        if(psz_data[u32_index] == '\n' ||
           psz_data[u32_index] == '\r' ||
           (u32_index == u32_size-1 &&
            psz_data[u32_index] != '\n'))
        {
            if(u32_index < u32_size &&
               psz_data[u32_index] == '\n')
            {
               ++u32_index;
               u32_delimiter_width = 2;
            }
            ++u32_nodes;
        }
    }

    //printf("Nodes found: `%u`\n", (unsigned)u32_nodes);
    //printf("Delim width: `%u`\n", (unsigned)u32_delimiter_width);

    // Allocate space - node addresses are fixed
    // at three characters.
    uint8 **psz_nodes = (uint8**)malloc(sizeof(uint8*) * (u32_nodes + 1));

    // Null terminate the array so it will be easy to traverse
    psz_nodes[u32_nodes] = '\0';

    // Allocate memory for the filenames. This could have
    // been done in one call, but contiguous memory space
    // may be sparse.
    for(u32_index = 0; u32_index < u32_nodes; ++u32_index)
    {
        psz_nodes[u32_index] = (uint8*)malloc(sizeof(uint8) * MAX_NODE_ADDR_LEN);
    }

    // Fill the array
    int u8_hex;
    uint8 sz_hex[2],
          u8_index,
          u8_node = 0;

    for(u32_index = 0; u32_index < u32_size; u32_index += u32_delimiter_width)
    {
        for(u8_index = 0; u8_index < 3; ++u8_index)
        {
            sz_hex[0] = (uint8)psz_data[u32_index++];
            sz_hex[1] = (uint8)psz_data[u32_index++];
            sscanf(sz_hex, "%x", &u8_hex);

            //printf("0,1,h:`%u`,`%u` `%c`, `0x%X`, `%c`,`%d`,`0x%X`\n",
            //    u8_node, u8_index, sz_hex[0], sz_hex[1], u8_hex, u8_hex, u8_hex);
            psz_nodes[u8_node][u8_index] = u8_hex;
        }
        psz_nodes[u8_node++][u8_index] = '\0';
    }
    free(psz_data);

    return psz_nodes;
}


/**
 * @brief Temporary RTSP functions
 * @todo Clean this crap up
 */

void doInsert(UFDATA* p_ufdata, uint8* sz_1)
{
    uint16 u16_j = 0;
    // Copy data
    while (*sz_1)
    {
        p_ufdata->dat.node_name[u16_j] = *sz_1;
        ++sz_1; ++u16_j;
    } //end while
    p_ufdata->dat.node_name[u16_j] = *sz_1; //write null
}

void doCommit(UFDATA* p_ufdata)
{
    union32 u_memaddr;
    u_memaddr.u32 = DATA_FLASH_PAGE;
    doWritePageFlash(u_memaddr, (uint8 *) p_ufdata, FLASH_DATA_SIZE);
}

void doRead(UFDATA* p_ufdata)
{
    union32 u_memaddr;
    u_memaddr.u32 = DATA_FLASH_PAGE;
    doReadPageFlash(u_memaddr, (uint8 *) p_ufdata, FLASH_DATA_SIZE);
}

//**********************************************************
/**
 * @brief Retrieve a node's name from its program memory
 * @note  This will only be used on the sensor nodes
 * @return uint8* The node's name
 */
//**********************************************************
void SNSL_GetNodeName(UFDATA *fdata)
{
    doRead(fdata);
}

//**********************************************************
/**
 * @brief Retrieve a node's name from its program memory
 * @note  This will only be used on the sensor nodes
 */
//**********************************************************
void SNSL_SetNodeName(uint8 *node_name)
{
    UFDATA fdata;
    doInsert(&fdata, node_name);
    doCommit(&fdata);  //write the data
}


//**********************************************************
/**
 * @brief Print a node's name
 */
//**********************************************************
void SNSL_PrintNodeName(void)
{
	UFDATA fdata;
	doRead(&fdata);

	outString(fdata.dat.node_name);
}


//**********************************************************
/**
 * @brief Read the first three bytes out of the config file
 *        and store them in the variables that were passed
 *        in by reference.
 * @param[out] The number of hops stored
 * @param[out] The number of timeouts allowed per hop
 * @param[out] The maximum number of times the collector
 *             will attempt to poll a node before it
 *             gives up completely.
 */
//**********************************************************
void SNSL_ParseConfigHeader(uint8 *hops, uint32 *timeout_per_hop,
                       uint8 *max_attempts)
{
    if(!VDIP_FileExists(FILE_CONFIG))
    {
        SNSL_CreateDefaultConfig();
    }

    uint8 *sz_data    = VDIP_ReadFile(FILE_CONFIG);

    *hops             = sz_data[0];

    *timeout_per_hop  = ((uint32)sz_data[1] << 24);
    *timeout_per_hop |= ((uint32)sz_data[2] << 16);
    *timeout_per_hop |= ((uint32)sz_data[3] << 8);
    *timeout_per_hop |=  (uint32)sz_data[4];

    *max_attempts = sz_data[5];
    free(sz_data);
}


//**********************************************************
/**
 * @brief Parses the header of the config file and then
 *        parses out the nodes and the number of associated
 *        attempts.
 * @param[out] The number of hops stored
 * @param[out] The number of timeouts allowed per hop
 * @param[out] The maximum number of times the collector
 *             will attempt to poll a node before it
 *             gives up completely.
 */
//**********************************************************
POLL* SNSL_ParseConfig(uint8 *hops, uint32 *timeout_per_hop,
                       uint8 *max_attempts)
{
    if(!VDIP_FileExists(FILE_CONFIG))
    {
        SNSL_CreateDefaultConfig();
    }

    POLL LAST_POLL;
    LAST_POLL.attempts  = LAST_POLL_FLAG;

    uint8 *sz_data      = VDIP_ReadFile(FILE_CONFIG),
           u8_curr      = 0,
           u8_nodes     = 0,
           u8_node      = 0;

    uint32 u32_index = 0,
           u32_len   = VDIP_FileSize(FILE_CONFIG);

    // Calculate number of nodes
    //
    // Number of bytes in ascii file minus the three
    // config bytes in the header divided by the number
    // of bytes in each node (3 for name + 1 for null).
    // Then add one more element for the one with the
    // sentinal value for keeping track of the last poll
    // in the array of structs
    u8_nodes = ((u32_len-HEADER_LEN)/4) + 1;
    u8_curr  = 0;
    POLL *polls = (POLL *)malloc(sizeof(POLL)*u8_nodes);
    *timeout_per_hop = 0;
    for(u32_index = 0; u32_index < u32_len; ++u32_index)
    {
        u8_curr = sz_data[u32_index];

        switch(u32_index)
        {
            case 0:
                *hops = u8_curr;
                break;
            case 1:
            case 2:
            case 3:
            case 4:
                *timeout_per_hop |= sz_data[u32_index];
                *timeout_per_hop <<= (4-u32_index)*8;
                break;
            case 5:
                *max_attempts = u8_curr;
                break;
            default:
                polls[u8_node].name[0]  = sz_data[u32_index];
                polls[u8_node].name[1]  = sz_data[++u32_index];
                polls[u8_node].name[2]  = sz_data[++u32_index];
                polls[u8_node].name[3]  = NULL;
                polls[u8_node].attempts = sz_data[++u32_index];
                ++u8_node;
        }
        u8_curr = sz_data[u32_index];
    }

    polls[u8_node] = LAST_POLL;
	free(sz_data);
    return polls;
}


//**********************************************************
/**
 * @brief Write the three header bytes and the nodes to
 *        the VDIP.
 * @param[in] The number of hops stored
 * @param[in] The number of timeouts allowed per hop
 * @param[in] The maximum number of times the collector
 *            will attempt to poll a node before it
 *            gives up completely.
 * @param[in] The array of POLLs
 */
//**********************************************************
void SNSL_WriteConfig(uint8 hops, uint32 timeout_per_hop,
                      uint8 max_attempts, POLL *polls)
{
    VDIP_DeleteFile(FILE_CONFIG);

    uint8  u8_i = 0;
    uint32 u32_index = 0;

    // Count polls
    uint8 u8_polls = 0;
    while(polls[u8_polls].attempts != LAST_POLL_FLAG)
    {
        ++u8_polls;
    }

    // Alloc memory for output string
    //
    // Each entry needs four bytes(3 for name and 1 for access attempts)
    // plus the three config bytes in the header
    uint8 *sz_out = (uint8 *)malloc(sizeof(uint8)*((u8_polls*4)+HEADER_LEN));
    //outString("WriteConfig: (u8_polls*4)+HEADER_LEN = ");
    //outUint8((u8_polls*4)+HEADER_LEN);
    //outChar('\n');

    // Save config options
    sz_out[u32_index]   = hops;
    sz_out[++u32_index] = timeout_per_hop >> 24;
    sz_out[++u32_index] = timeout_per_hop >> 16;
    sz_out[++u32_index] = timeout_per_hop >> 8;
    sz_out[++u32_index] = timeout_per_hop;
    sz_out[++u32_index] = max_attempts;

    // Save poll names and the number of attempts
    while(polls[u8_i].attempts != LAST_POLL_FLAG)
    {
        sz_out[++u32_index] = polls[u8_i].name[0];
        sz_out[++u32_index] = polls[u8_i].name[1];
        sz_out[++u32_index] = polls[u8_i].name[2];
        sz_out[++u32_index] = polls[u8_i].attempts;
        //outString("\n\nAttempts:\n");
        //outUint8(sz_out[u32_index]);
        ++u8_i;
    }

    ++u32_index;
    /*u8_i = 0;
    while (u8_i < u32_index){
        outUint8(sz_out[u8_i]);
        u8_i++;
    }*/

    VDIP_WriteFileN(FILE_CONFIG, sz_out, u32_index);
    free(sz_out);
}


//**********************************************************
/**
 * @brief Find an entry in the array of POLLs
 * @param[in] The 3-byte name of the node
 * @param[in] The array of POLLs to be searched
 * @return The index of the match or UNKNOWN_NODE
 */
//**********************************************************
uint8 SNSL_SearchConfig(uint8 *name, POLL *polls)
{
    uint8 u8_i;
    for(u8_i = 0; polls[u8_i].attempts != LAST_POLL_FLAG; ++u8_i)
    {
        /*
        outString("Checking: `");
        outUint8(polls[u8_i].name[0]);
        outUint8(polls[u8_i].name[1]);
        outUint8(polls[u8_i].name[2]);
        outString("` vs `");
        outUint8(polls[u8_i].name[0]);
        outUint8(polls[u8_i].name[1]);
        outUint8(polls[u8_i].name[2]);
        outString("`\n");
        */
        //if(strcmp(polls[u8_i].name, name) == 0)
        if(polls[u8_i].name[0] == name[0] &&
           polls[u8_i].name[1] == name[1] &&
           polls[u8_i].name[2] == name[2])
        {
            /*
            outString("  Found: attempts=`");
            outUint8(polls[u8_i].attempts);
            outString("`\n");
            */
            //return polls[u8_i].attempts;
            return u8_i;
        }
    }
    /*
    outString("  NOT Found: attempts=`");
    outUint8(0);
    outString("`\n");
    */
    return UNKNOWN_NODE;
}


//**********************************************************
/**
 * @brief Merge the ascii names with any previous attempts
 *        that may already exist in the binary config file.
 * @return An array of POLLs.
 */
//**********************************************************
POLL* SNSL_MergeConfig(void)
{
    // Parse the user-entered values which are in Ascii form
    //outString("SNSL_MergeConfig: Started\n");
    //outString("Parsing Ascii File\n");
    uint8 **nodes = SNSL_ParseNodeNames();

    //outString("Parsing nodes from binary config file\n");
    // Get nodes in config file to update the new array
    // with the current number of attempts;
    uint8 hops, max_attempts;
    uint32 timeout_per_hop;
    POLL *old_config = SNSL_ParseConfig(&hops,
                &timeout_per_hop, &max_attempts);

    // Count nodes
    uint8 u8_nodes = 0;
    while(nodes[u8_nodes])
    {
        ++u8_nodes;
    }

    // Copy the node names into the POLL array
    uint8 u8_i, u8_j, u8_search;
    POLL *polls = (POLL *)malloc(sizeof(POLL)*(u8_nodes+1));
    for(u8_i = 0; u8_i < u8_nodes; ++u8_i)
    {
        for(u8_j = 0; u8_j < 3; ++u8_j)
        {
            polls[u8_i].name[u8_j] = nodes[u8_i][u8_j];
        }
        polls[u8_i].name[u8_j] = NULL;

        u8_search = SNSL_SearchConfig(polls[u8_i].name, old_config);
        polls[u8_i].attempts = 0;
        if(u8_search != UNKNOWN_NODE)
        {
            polls[u8_i].attempts = old_config[u8_search].attempts;
        }

        /*
        outString("Config value for `");
        outUint8(polls[u8_i].name[0]);
        outUint8(polls[u8_i].name[1]);
        outUint8(polls[u8_i].name[2]);
        outString("`= `");
        outUint8(polls[u8_i].attempts);
        outString("`\n");
        */
    }
    polls[u8_nodes].attempts = LAST_POLL_FLAG;

    //outString("Writing to binary config\n");
    //SNSL_WriteConfig(hops, timeout_per_hop, max_attempts, polls);
    //outString("SNSL_MergeConfig: Finished\n");
    VDIP_CleanupDirList(nodes);
    free(old_config);

    return polls;
}


//**********************************************************
/**
 * @brief Print out all the nodes' names and numbers of
 *        failed attempts as well as any other
 *        configuration settings.
 */
//**********************************************************
void SNSL_PrintConfig(void)
{
    uint8 hops, max;
    uint32 timeout;
    POLL *polls = SNSL_ParseConfig(&hops, &timeout, &max);

    uint8 u8_i = 0;
    while(polls[u8_i].attempts != 0xff)
    {
        printf("Node: `%02X%02X%02X`; Attempts: `%u`\n",
                    polls[u8_i].name[0],
                    polls[u8_i].name[1],
                    polls[u8_i].name[2],
                    polls[u8_i].attempts);
        ++u8_i;
    }
	free(polls);
}


//**********************************************************
/**
 * @brief Configure PIC for low power mode
 * @note  This is a rewrite of the function from pic24_util.c. This will only work for the PIC24FJ64GA102.
 */
//**********************************************************
void SNSL_ConfigLowPower(void)
{
	//Config all digital I/O pins as inputs
	//TRISx regs control I/O status: 1 = input 0 = output
	TRISB = 0xFFFF;
	TRISA = 0xFFFF;

	//Config all analog pins as digital I/O
	//AD1PCFG reg controls AD pin config: 1 = digital input 0 = analog input
	AD1PCFG = 0xFFFF;

	//Enable all pullups except SOSCI(CN1) and SOSCO (CN0)
	//1 = pull-up on 0 = pull-up off
	//CNPU2 = CN31 -> CN16
	//CNPU2 = 0xFFFF;
	//CNPU1 = CN15 -> CN0 (don't want pull-up on CN1 and CN0)
	//CNPU1 = 0xFFFC;
}

//**********************************************************
/**
 * @brief Write event log entry for a polling event (either
 *        start or stop)
 * @param[in] The struct of uint8 timestamp values
 * @param[in] The event type flag (0 == poll stopped, 1 == poll started)
 */
//**********************************************************

void SNSL_LogPollEvent(uint8 type, unionRTCC *u_RTCC)
{
    //[d/m/y h:m:s] Polling Started
    //[d/m/y h:m:s] Polling Stopped

    uint8 log_format0[] = "[%02x/%02x/%02x %02x:%02x:%02x] Polling Started\n";
    uint8 log_format1[] = "[%02x/%02x/%02x %02x:%02x:%02x] Polling Stopped\n";
    uint8 log_out[50];

    if (type == 0x00) {
        sprintf(log_out, log_format0,
            u_RTCC->u8.month, u_RTCC->u8.date, u_RTCC->u8.yr, u_RTCC->u8.hour,
            u_RTCC->u8.min, u_RTCC->u8.sec);
    }
    else if (type == 0x01) {
        sprintf(log_out, log_format1,
            u_RTCC->u8.month, u_RTCC->u8.date, u_RTCC->u8.yr, u_RTCC->u8.hour,
            u_RTCC->u8.min, u_RTCC->u8.sec);
    }

    log_out[49] = 0x0;
    outString(log_out);
    VDIP_WriteFile("LOG.TXT", log_out);
}

//**********************************************************
/**
 * @brief Write event log entry for a node ignored due to
 *        too many response failures
 * @param[in] The 3-byte name of the node
 * @param[in] The struct of uint8 timestamp values
 */
//**********************************************************

void SNSL_LogNodeSkipped(uint8 c_ad1, uint8 c_ad2, uint8 c_ad3, unionRTCC *u_RTCC)
{
    uint8 log_format[] = "[%02x/%02x/%02x %02x:%02x:%02x] Node %02X%02X%02X Ignored (Too Many Failures)\n";
    uint8 log_out[60];

    sprintf(log_out, log_format,
        u_RTCC->u8.month, u_RTCC->u8.date, u_RTCC->u8.yr, u_RTCC->u8.hour,
                        u_RTCC->u8.min, u_RTCC->u8.sec, c_ad1, c_ad2, c_ad3);
    outString(log_out);
    VDIP_WriteFile("LOG.TXT", log_out);
}

//**********************************************************
/**
 * @brief Write event log entry for a node that failed to
 *        respond
 * @param[in] The 3-byte name of the node
 * @param[in] The struct of uint8 timestamp values
 */
//**********************************************************

void SNSL_LogResponseFailure(uint8 c_ad1, uint8 c_ad2, uint8 c_ad3, unionRTCC *u_RTCC)
{
    uint8 log_format[] = "[%02x/%02x/%02x %02x:%02x:%02x] Node %02X%02X%02X Failed to Respond\n";
    uint8 log_out[60];

    sprintf(log_out, log_format,
        u_RTCC->u8.month, u_RTCC->u8.date, u_RTCC->u8.yr, u_RTCC->u8.hour,
                        u_RTCC->u8.min, u_RTCC->u8.sec, c_ad1, c_ad2, c_ad3);

    outString(log_out);
    VDIP_WriteFile("LOG.TXT", log_out);
}

uint32 SNSL_Pow(uint8 base, uint8 power)
{
    uint32 u32_retVal = base;
    if (power == 0) {
        return 1;
    }
    uint8 i;
    for ( i = 1; i < power; i++) {
        u32_retVal *= base;
    }

    return u32_retVal;
}

uint32 SNSL_Atoi(uint8 *str)
{

    uint32 u32_return;
    uint8 u8_i = 0,
          u8_j = 0;

    // Count numbers in array
    while(str[++u8_i]);
    --u8_i;

    u32_return = 0;
    while(u8_i > 0)
    {
        u32_return += (str[u8_j]-0x30)*(SNSL_Pow(10,u8_i));
        --u8_i;
        ++u8_j;
    }
    u32_return += (str[u8_j] - 0x30)*(SNSL_Pow(10, u8_i));
    return (uint32)u32_return;
}


//**********************************************************
/**
 * @brief Create a config file with sane default values
 */
//**********************************************************
void SNSL_CreateDefaultConfig(void)
{
    POLL LAST_POLL;
    LAST_POLL.attempts = LAST_POLL_FLAG;
    SNSL_WriteConfig(4, (uint32)300, 16, &LAST_POLL);
}


//**********************************************************
/**
 * @brief Create a config file with sane default values
 */
//**********************************************************
void SNSL_CreateDefaultNodes(void)
{
    // Create a blank file
}
