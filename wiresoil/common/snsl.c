/***********************************************************
 * Includes
 **********************************************************/
#include "pic24_all.h"
#include "vdip.h"
#include "snsl.h"

// for sscanf
#include <stdio.h>

// For `malloc` and `free`
#include <stdlib.h>


/***********************************************************
 * Function Definitions
 **********************************************************/

//**********************************************************
/**
 * @brief Retrieve NODES.TXT and parse the nodes' names
 * @return char** The nodes' names in hex
 */
//**********************************************************
char** SNSL_ParseNodeNames(void)
{
    char *psz_data   = VDIP_ReadFile(FILE_NODES);
    uint32 u32_size  = VDIP_FileSize(FILE_NODES),
           u32_index = 0;

    // Count the newlines to count the number
    // of nodes we need to allocate memory for
    uint32 u32_nodes = 0,
           u32_delimiter_width = 1;

    for(; u32_index < u32_size; ++u32_index)
    {
        // Take care of linux and windows newlines
        //
        // Fun fact: Our SPI module automatically
        // converts \r to \n, so we'll have two
        // successive '\n's if the file was created
        // with Windows.
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
    char **psz_nodes = (char**)malloc(sizeof(char*) * (u32_nodes + 1));

    // Null terminate the array so it will be easy to traverse
    psz_nodes[u32_nodes] = '\0';

    // Allocate memory for the filenames. This could have
    // been done in one call, but contiguous memory space
    // may be sparse.
    for(u32_index = 0; u32_index < u32_nodes; ++u32_index)
    {
        psz_nodes[u32_index] = (char*)malloc(sizeof(char) * MAX_NODE_ADDR_LEN);
    }

    // Fill the array
    int u8_hex;
    char sz_hex[2];
    uint8 u8_index,
          u8_node = 0;
    for(u32_index = 0; u32_index < u32_size; u32_index += u32_delimiter_width)
    {
        for(u8_index = 0; u8_index < 3; ++u8_index)
        {
            // Convert the ascii representation of the nodes numbers into hex
            //
            // Note: This was written to get rid of the need for sscanf. It will
            //       NOT work with letters at this point!
            //u8_hex = ((psz_data[u32_index]-0x30)*0x10) + (psz_data[u32_index+1]-0x30);
            //u32_index += 2;

            sz_hex[0] = (char)psz_data[u32_index++];
            sz_hex[1] = (char)psz_data[u32_index++];
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

void doInsert(UFDATA* p_ufdata, char* sz_1){
  uint16 u16_j = 0;
      while (*sz_1) {  //copy data
        p_ufdata->dat.node_name[u16_j] = *sz_1;
        sz_1++; u16_j++;
      } //end while
      p_ufdata->dat.node_name[u16_j] = *sz_1; //write null
}

void doCommit(UFDATA* p_ufdata) {
  union32 u_memaddr;
  u_memaddr.u32 = DATA_FLASH_PAGE;
  doWritePageFlash(u_memaddr, (uint8 *) p_ufdata, FLASH_DATA_SIZE);
}

void doRead(UFDATA* p_ufdata) {
  union32 u_memaddr;
  u_memaddr.u32 = DATA_FLASH_PAGE;
  doReadPageFlash(u_memaddr, (uint8 *) p_ufdata, FLASH_DATA_SIZE);
}

//**********************************************************
/**
 * @brief Retrieve a node's name from its program memory
 * @note  This will only be used on the sensor nodes
 * @return char* The node's name
 */
//**********************************************************
char* SNSL_GetNodeName(void)
{
    //char *name = (char*)malloc(sizeof(char)*MAX_NODE_NAME_LEN);
    UFDATA fdata;
    doRead(&fdata);
    //doInsert(&fdata, name);
    //printf("\nfdata:`%s`;name:`%s`\n", fdata.dat.node_name, name);
    //return name;
    printf("\nWithin GetNodeName: `%s`\n", fdata.dat.node_name);
    return "test";
}

//**********************************************************
/**
 * @brief Retrieve a node's name from its program memory
 * @note  This will only be used on the sensor nodes
 */
//**********************************************************
void SNSL_SetNodeName(char *node_name)
{
    UFDATA fdata;
    doInsert(&fdata, node_name);
    doCommit(&fdata);  //write the data
}
