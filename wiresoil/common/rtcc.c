#include "pic24_all.h"
#include "rtcc.h"

#include <stdio.h> // printf


/**********************************************************
 * Function Definitions
 **********************************************************/

/**********************************************************
 *
 * @brief        Get BCD value from user input
 * @param [in]   Character string to display to user
 * @return uint8 BCD value of user input
 *
 **********************************************************/
uint8 RTCC_GetBCDValue(char *sz_1)
{
    char sz_buff[3];
    uint16 u16_bin;
    uint8 u8_bcd;
    outString(sz_1);
    inStringEcho(sz_buff, 2);
    sscanf(sz_buff, "%d", (int *)&u16_bin);
    u8_bcd = u16_bin / 10;
    u8_bcd <<= 4;
    u8_bcd  |= (u16_bin % 10);
    return u8_bcd;
}


/**********************************************************
 *
 * @brief      Convert given character string to BCD
 * @param [in] Character string
 * @return     BCD value of given character string
 *
 **********************************************************/
uint8 RTCC_ParseVal(char *sz_1)
{
    uint16 u16_bin;
    uint8 u8_bcd;
    sscanf(sz_1, "%d", (int *)&u16_bin);
    u8_bcd = u16_bin / 10;
    u8_bcd <<= 4;
    u8_bcd = u8_bcd | (u16_bin % 10);
    return u8_bcd;
}


/**********************************************************
 *
 * @brief      Get initial date settings from user
 * @param [in] Pointer to global RTCC struct
 *
 **********************************************************/
void RTCC_GetDateFromUser(RTCC *r)
{
    r->u8.yr    = RTCC_GetBCDValue("\nEnter year (00-99): ");
    r->u8.month = RTCC_GetBCDValue("\nEnter month (01-12): ");
    r->u8.date  = RTCC_GetBCDValue("\nEnter day of month (01-31): ");
    r->u8.wday  = RTCC_GetBCDValue("\nEnter week day (0 (Sunday)-6 (Saturday)): ");
    r->u8.hour  = RTCC_GetBCDValue("\nEnter hour (00-23): ");
    r->u8.min   = RTCC_GetBCDValue("\nEnter min (00-59): ");
    r->u8.sec   = RTCC_GetBCDValue("\nEnter sec(00-59): ");
}


/**********************************************************
 *
 * @brief      Get initial date settings from user without
 *             requesting the day of the week
 * @param [in] Pointer to global RTCC struct
 *
 **********************************************************/
void RTCC_GetDateFromUserNoWday(RTCC *r)
{
    r->u8.yr    = RTCC_GetBCDValue("\nEnter year (00-99): ");
    r->u8.month = RTCC_GetBCDValue("\nEnter month (01-12): ");
    r->u8.date  = RTCC_GetBCDValue("\nEnter day of month (01-31): ");
    r->u8.wday  = RTCC_ParseVal("0");
    r->u8.hour  = RTCC_GetBCDValue("\nEnter hour (00-23): ");
    r->u8.min   = RTCC_GetBCDValue("\nEnter min (00-59): ");
    r->u8.sec   = RTCC_GetBCDValue("\nEnter sec(00-59): ");
}


/**********************************************************
 *
 * @brief      Set default RTCC values
 * @param [in] Pointer to global RTCC struct
 *
 **********************************************************/
void RTCC_SetDefaultVals(RTCC *r)
{
    //default time: 11/1/2010 12:00:00 am
    r->u8.yr    = RTCC_ParseVal("10");
    r->u8.month = RTCC_ParseVal("11");
    r->u8.date  = RTCC_ParseVal("1");
    r->u8.wday  = RTCC_ParseVal("1");
    r->u8.hour  = RTCC_ParseVal("0");
    r->u8.min   = RTCC_ParseVal("0");
    r->u8.sec   = RTCC_ParseVal("0");
}


/**********************************************************
 *
 * @brief      Copy values in RTCC struct to RTCC regs
 * @param [in] Pointer to global RTCC struct
 *
 **********************************************************/
void RTCC_Set(RTCC *r)
{
    uint8 u8_i;
    __builtin_write_RTCWEN();
    RCFGCALbits.RTCEN = 0;
    RCFGCALbits.RTCPTR = 3;

    for(u8_i = 0; u8_i < 4; ++u8_i)
    {
        RTCVAL = r->regs[u8_i];
    }

    RCFGCALbits.RTCEN = 1;
    RCFGCALbits.RTCWREN = 0;
}


/**********************************************************
 *
 * @brief      Copy values from RTCC regs to RTCC struct
 *             for output
 * @param [in] Pointer to global RTCC struct
 *
 **********************************************************/
void RTCC_Read(RTCC *r)
{
    while(!RCFGCALbits.RTCSYNC);

    uint8 u8_i;
    RCFGCALbits.RTCPTR = 3;

    for(u8_i = 0; u8_i < 4; u8_i++)
    {
        r->regs[u8_i] = RTCVAL;
    }
}


/**********************************************************
 *
 * @brief      Print formatted RTCC data from RTCC struct
 *             to screen
 * @param [in] Pointer to global RTCC struct
 *
 **********************************************************/
void RTCC_Print(RTCC *r)
{
    printf("[%2x/%2x/%2x] %02x:%02x:%02x \n",
           (uint16)r->u8.month, (uint16)r->u8.date,
           (uint16)r->u8.yr,    (uint16)r->u8.hour,
           (uint16)r->u8.min,   (uint16)r->u8.sec);
}
