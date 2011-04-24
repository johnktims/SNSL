#include "pic24_all.h"
#include "packet.h"
#include "snsl.h"
#include "rtcc.h"
#include "vdip.h"

#include <stdio.h>  //for 'printf' | 'sprintf'
#include <stdlib.h> // for 'free'
#include <string.h> // for 'memcpy'


/***********************************************************
 * Global Variables
 ***********************************************************/
uint8 u8_numHops, u8_failureLimit, u8_stopPolling,
      u8_pollsCompleted, u8_pollsIgnored, u8_pollsFailed,
      u8_newFileName;

uint32 u32_hopTimeout;

union32 timer_max_val;

RTCC u_RTCC, u_RTCCatStartup;

char filename[13],
     psz_out[128];


/***********************************************************
 * Pin Assignment Macros
 ***********************************************************/
#define SLEEP_INPUT (_RB14)
#define TEST_SWITCH (_RA2)
#define VDIP_POWER  (_RB7)
#define SLEEP_TIME  (_RB8)


/***********************************************************
 * Pin configuration functions
 ***********************************************************/

 /**********************************************************
  *
  * @brief Sleep Input pin configuration
  *
  **********************************************************/
inline void CONFIG_SLEEP_INPUT(void)
{
    CONFIG_RB14_AS_DIG_INPUT();
    DISABLE_RB14_PULLUP();
    DELAY_US(1);
}

/**********************************************************
 *
 * @brief Test Mode Switch pin configuration
 *
 **********************************************************/
inline void CONFIG_TEST_SWITCH(void)
{
    CONFIG_RA2_AS_DIG_INPUT();
    ENABLE_RA2_PULLUP();
    DELAY_US(1);
}

/**********************************************************
 *
 * @brief Sleep time pin configuration
 *
 **********************************************************/
inline void CONFIG_SLEEP_TIME(void)
{
    CONFIG_RB8_AS_DIG_INPUT();
    ENABLE_RB8_PULLUP();
    DELAY_US(1);
}

/**********************************************************
 *
 * @brief Set PIC I/O pins a low power mode
 * @note  Reset all pins to be analog inputs in
 *        order to save power. As such, we have to
 *        re-define all of the digital I/O pins when we
 *        come out of low power mode
 *
 **********************************************************/
void configHighPower(void)
{
    // Turn on the VDIP
    _LATB7 = 0;

    CONFIG_SLEEP_INPUT();
    CONFIG_TEST_SWITCH();
    CONFIG_VDIP_POWER();
    CONFIG_SLEEP_TIME();
    CONFIG_INT1_TO_RP(14);
}

/**********************************************************
 * Configure Timers
 **********************************************************/

/**********************************************************
 *
 * @brief Polling timeout timer
 * @note  A 32-bit timer is used as a polling timeout
 *        so that we don't wait forever on a node to
 *        to respond. Each PIC timer is 16-bit, so timers
 *        2 and 3 have to be chained together to get 32-bit
 *        resolution. See pg. 510-513 in PIC24 Micro book.
 *
 **********************************************************/
void configTimer23(void)
{
    T2CON = T2_OFF | T2_IDLE_CON | T2_GATE_OFF | T2_32BIT_MODE_ON
            | T2_SOURCE_INT | T2_PS_1_1;
}

void startTimer23(void)
{
    timer_max_val.u32 = usToU32Ticks(((u32_hopTimeout * u8_numHops) * 1000), getTimerPrescale(T2CONbits)) - 1;
    PR2 = timer_max_val.u16.ls16;   //LSW
    PR3 = timer_max_val.u16.ms16;   //MSW
    TMR3HLD = 0;        // Clear timer3 value
    TMR2 = 0;           // Clear timer2 value
    _T3IF = 0;          // Clear interrupt flag
    _T3IP = 3;          // Set timer interrupt priority (must
    //    be higher than sleep interrupt)
    _T3IE = 1;          // Enable timer interrupt
    T2CONbits.TON = 1;  // Start the timer
}

/**********************************************************
 * Interrupt Handlers
 **********************************************************/

/**********************************************************
 *
 * @breif Wake PIC from sleep
 * @note  This interrupt exists only to wake the PIC from
 *        sleep mode. It is attached to the SLEEP_INPUT
 *        pin and is configured to fire whenever on the
 *        rising edge of that pin. As such, it's only duty
 *        is to clear its own flag.
 *
 **********************************************************/
void _ISRFAST _INT1Interrupt(void)
{
    _INT1IF = 0;        //clear interrupt flag
}

/**********************************************************
 *
 * @breif Stop waiting for data input
 * @note  This interrupt fires whenever the timout timer
 *        expires. This is used to return from the blocking
 *        input read function and to denote that the node
 *        being polled is not going to respond.
 *
 **********************************************************/
void _ISRFAST _T3Interrupt(void)
{
    u8_stopPolling = 1;
    T2CONbits.TON  = 0;             //stop the timer
    outString("Timer Expired\n");
    _T3IF = 0;                      //clear interrupt flag
}

/**********************************************************
 * Polling Functions
 **********************************************************/

/**********************************************************
 *
 * @brief  Wait for a character from the wireless module
 * @return The received character
 * @note   If we don't receive a character, we will wait
 *         until the timeout fires and return to the calling
 *         function.
 *
 **********************************************************/
uint8 blocking_inChar(void)
{
    uint8 u8_char = '~';
    startTimer23();

    // Wait for character
    while(!isCharReady2() && !u8_stopPolling);

    if(!u8_stopPolling)
    {
        T2CONbits.TON = 0;
        u8_char = inChar2();
    }

    return u8_char;
}


/**********************************************************
 *
 * @brief  Determine whether or not the mesh is awake
 * @return 1 if the mesh is up, and 0 otherwise
 * @note   This function reads in a message over UART-2 from
 *         the SNAP module. Once the SNAP has woken the 
 *         mesh, it sends a message to the PIC letting it
 *         know that it can begin the polling process.
 *
 **********************************************************/
uint8 isMeshUp(void)
{
    uint8 u8_c = inChar2();

    if(u8_c == 0x04)
    {
        u8_c = inChar2();

        if(u8_c == 0x01)
        {
            inChar2();
            inChar2();
            inChar2();
            return 1;
        }
    }

    return 0;
}

/**********************************************************
 *
 * @brief  Lock mesh in awake mode
 * @note   This function sends a message to the SNAP module
 *         instructing it to lock the mesh into awake mode.
 *         This is used so that the mesh doesn't try to go
 *         to sleep while we're in the middle of polling.
 *
 **********************************************************/
void sendStayAwake(void)
{
    //Packet Format: 0x1E + length + groupID LSB + grpID MSB + TTL + 0x02 + 'sleepyNodeFalse'
    const char sz_data[] = "sleepyModeFalse";
    SendPacketHeader();  //0x1E
    outChar2(0x10);      //packet length = 1 + string length
    outChar2(0x01);      //LSB of group ID is 0001
    outChar2(0x00);      //MSB of group ID
    outChar2(0x05);      //TTL
    outChar2(0x02);      //MRPC packet type
    outString2(sz_data);
}

/**********************************************************
 *
 * @brief  Poll a sensor node and get its data
 * @input  c_ad1 | c_ad2 | c_ad3 : node address to poll
 *         hr | min | sec: timestamp to send as ACK
 * @return 0 if failed and we're logging failures | 1 if 
 *         successful | 2 if failed and we're not logging
 *         failures
 * @note   This function polls the supplied node and sends
 *         the supplied timestamp as an ACK.
 *
 **********************************************************/
uint8 doPoll(char c_ad1, char c_ad2, char c_ad3, uint8 hr, uint8 min, uint8 sec)
{
    /*Send data request message to node*/
    u8_stopPolling = 0;
    printf("Polling Node %02X%02X%02X\n", c_ad1, c_ad2, c_ad3);
    SendPacketHeader();
    outChar2(0x05);     //packet length
    outChar2(c_ad1);    //node address
    outChar2(c_ad2);
    outChar2(c_ad3);
    outChar2(0x03); //Appdata packet (forward data directly to PIC)
    outChar2(MONITOR_REQUEST_DATA_STATUS);
    outChar2(hr);   //ACK timestamp
    outChar2(min);
    outChar2(sec);

    WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();
    /*This is the maximum input packet size for a data point
    size = (2 for required header fields [packet type and header]) + timestamp (6B) + 5 temperatures (4B*5=20B) + 
            4 redox values (4B*5 = 16B) + reference voltage (4B) + node name (12B)*/
    uint8 size = 2 + 6 + sizeof(float) * 5 + sizeof(float) * 4 + sizeof(float) + 12; // == 60
    uint8 poll_data[size];
    uint8 i = 0;

    // Clear out the  poll buffer
    for(i = 0; i < size; ++i)
    {
        poll_data[i] = 0;
    }

    volatile uint8 tmp,
             packet_length = 0,
             remaining_polls = 0,
             u8_char;

    // Parse received packet header
    // full packet header == 7 bytes
    // (header (1B) | node address (3B) | packet type (1B) | size (1B) | remaining polls (1B)
    for(tmp = 0; tmp < 7; tmp++)
    {
        u8_char = blocking_inChar();    //read in header

        /* For some reason, the SNAP likes to send a 0 before the packet header. This
        *  statement looks for the 0 and throws it out, then calls the read again so we
        *  don't get off on our input count*/
        if(u8_char == 0 && tmp == 0)
        {
            u8_char = blocking_inChar();
        }

        /* If the retrun value from blocking_inChar() is ever '~", then we know that the 
        *  timeout interrupt fired and we never got any data. We should then stop the polling
        *  process and return that the poll failed.*/
        if(u8_char == '~')
        {
            u8_stopPolling = 1;
            break;
        }
        
        /* If we get to here, the we can actually start reading in the data we want*/
        else
        {
            /* Save the packet length so we know how much more data to read in. Packet length
            *  is the 6th byte, but we throw away the header at the beginning, hence the 4th byte
            *  we read in is the length.*/
            if(tmp == 4)
            {
                /*The SNAP requires that the header and type fields are included in the packet
                * length, so we have to subtract 2 to get the length of the data field.*/
                packet_length = u8_char - 2;
            }
            /* We also need to know if there are any remaining data points that the sensor has
            *  so we can retrieve them later*/
            else if(tmp == 6)
            {
                remaining_polls = u8_char;
            }
        }
    }

    /* Parse remaining bytes in packet*/
    for(tmp = 0; tmp < packet_length; ++tmp)
    {
        u8_char = blocking_inChar();

        /* If the retrun value from blocking_inChar() is ever '~", then we know that the 
        *  timeout interrupt fired and we never got any data. We should then stop the polling
        *  process and return that the poll failed.*/
        if(u8_char == '~')
        {
            u8_stopPolling = 1;
            break;
        }
        else
        {
            poll_data[tmp] = u8_char;
        }
    }

    /* Now that we have all of the data, we need to format it and log it out. If we've gotten to
    *  this point and we haven't had a read error (u8_stopPolling == 0), then we can continue*/
    if(!u8_stopPolling)
    {
        /* Null-terminate packet data so we know where to stop.*/
        poll_data[packet_length] = '\0';

        // Print raw packet data in hex to the debug output
        outString("RAW PACKET:");
        tmp = 0;

        for(tmp = 0; tmp < size - 1; ++tmp)
        {
            outUint8(poll_data[tmp]);
            outChar(' ');
        }
        outString("\n");
        
        //Calculate where the start of the node name is
        //size - 2 == max packet message - header bytes - max node name size == start of node name
        uint8 nameStart = size - 12 - 2;
        
        // Variable-length node name, so put it in a null-terminated string for easy output
        char psz_node_name[MAX_NODE_NAME_LEN];

        for(i = nameStart; i < packet_length; ++i)
        {
            psz_node_name[i - nameStart] = poll_data[i];
        }

        psz_node_name[i - nameStart] = 0x0;
        
        //Print node name to debug output
        printf("NODE NAME: `%s`\n", psz_node_name);

        /*Use the FLOAT union defined in snsl.h to convert the read in character strings to 
        * printable float values. We have 10 values we have to convert to floats (5 temps | 4 redox | 1 ref),
        * so create an array of FLOAT unions to hold these values.*/
        FLOAT probes[10];
        int x, offset = 6;

        /*Start copying out of the poll_data holder into the FLOAT unions. We have to skip over
        * the 6 bytes of timestamp data, so we set offset to 6 then add the size of a float to it every
        * iteration*/
        for(x = 0; x < 10; offset += sizeof(float), ++x)
        {
            memcpy(probes[x].s, poll_data + offset, sizeof(float));
        }

        /*Format and assemble the data point into the form it will be logged*/
        // Date(MM/DD/YYY),time(HH:MM:SS),1.250,vref,sp1,sp2,sp3,sp4,temp1,temp2,temp3,temp4,temp5,nodeName
        char psz_fmt[] =
            "%02x/%02x/%02x,"                // timestamp [MM/DD/YY]
            "%02x:%02x:%02x,"                // timestamp [HH:MM:SS]
            "1.250,"                         // some voltage reference (constant value)
            "%3.3f,"                         // battery voltage
            "%3.3f,%3.3f,%3.3f,%3.3f,"       // redox samples
            "%3.3f,%3.3f,%3.3f,%3.3f,%3.3f," // temperature samples
            "%s\n";                          // node name
        uint8 *p = poll_data;
        sprintf(psz_out, psz_fmt,
                p[0], p[1], p[2], p[3], p[4], p[5], // Timestamp
                probes[0].f,                   // battery voltage
                probes[1].f, probes[2].f, probes[3].f, probes[4].f, // redox samples
                probes[5].f, probes[6].f, probes[7].f, probes[8].f, probes[9].f, // temperature samples
                psz_node_name); // node name
        psz_out[127] = 0x0; //null terminate for easy printing

        printf("FORMATTED PACKET:%s", psz_out); //print formatted packet to the debug output

        VDIP_WriteFile((uint8 *)filename, (uint8 *)psz_out);    //write the data point to the data file

        /* If we are not sending a new data poll request (hr != 0xff) and we have received the last
        *  old data point (remaining_polls == 0x01) send the single ACK packet with the timestamp of
        *  the point we just received*/
        if(remaining_polls == 0x01 && hr != 0xff)
        {
            SendPacketHeader();
            outChar2(0x05);    // Packet length
            outChar2(c_ad1);
            outChar2(c_ad2);
            outChar2(c_ad3);
            outChar2(0x03);    // Appdata packet (forward data directly to PIC)
            outChar2(ACK_PACKET);
            outChar2(p[3]);
            outChar2(p[4]);
            outChar2(p[5]);
            WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();
        }
        /* Otherwise, if we have remaining data points to retrieve, call doPoll again with the timestamp
        *  of the point we just received*/
        else if(remaining_polls != 0x00)
        {
            doPoll(c_ad1, c_ad2, c_ad3, p[3], p[4], p[5]);
        }

        return 0x01;    //the poll was successful
    }
    else if(SLEEP_TIME)
    {
        /* If the SLEEP_TIME pin is HIGH, we are in normal mode, so we need to return that
        *  the poll request failed and we want to record the failure*/
        return 0x00;
    }
    else
    {
        /* If the SLEEP_TIME pin is LOW, we are in setup mode, so we don't want to record
        *  that the node failed. Return that the poll request failed, but note that we don't
        *  don't want to record this*/
        return 0x02;
    }
}

/**********************************************************
 *
 * @brief  Return mesh to normal mode
 * @note   This function sends a message to the SNAP module
 *         instructing it that polling is finished, and it
 *         needs to return the mesh to normal mode.
 *
 **********************************************************/
void sendEndPoll(void)
{
    const char sz_data[] = "pollingStopped";
    SendPacketHeader(); //0x1E
    outChar2(0x0F); //packet length
    outChar2(0x01);
    outChar2(0x00);
    outChar2(0x05); //TTL
    outChar2(0x02); //MRPC packet type
    outString2(sz_data);
}


/****************************MAIN******************************************/
int main(void)
{
    SNSL_ConfigLowPower();
    configClock();
    configDefaultUART(DEFAULT_BAUDRATE);
    configUART2(DEFAULT_BAUDRATE);
    CONFIG_SLEEP_INPUT();
    CONFIG_TEST_SWITCH();
    CONFIG_VDIP_POWER();
    CONFIG_SLEEP_TIME();
    CONFIG_INT1_TO_RP(14);
    __builtin_write_OSCCONL(OSCCON | 0x02);
    RTCC_SetDefaultVals(&u_RTCC);
    RTCC_Set(&u_RTCC);
    u8_stopPolling = 0;
    configTimer23();

    while(!RCFGCALbits.RTCSYNC);

    RTCC_Read(&u_RTCCatStartup);

    while(1)
    {
        if(!TEST_SWITCH)    //we are in setup mode, so print the config menu
        {
            VDIP_Init();

            while(!VDIP_DiskExists())
            {
                outString("\nPlease insert a disk into the VDIP before "
                          "continuing. Press any key to continue....\n");
                inChar();
                continue;
            }

            uint8 u8_menuIn,
                  u8_numHops,
                  u8_failureLimit;
            uint32 u32_hopTimeout;
            outString("\n\n\n");
            outString("Setup Mode:\n\n");
            outString("Choose an option -\n");
            outString("1. Configure clock\n");
            outString("2. Set maximum number of network hops\n");
            outString("3. Set timout per hop\n");
            outString("4. Set node failure limit\n");
            outString("5. Reset node ignore list\n");
            outString("6. Exit Setup\n");
            outString("--> ");
            u8_menuIn = inCharEcho();

            SNSL_ParseConfigHeader(&u8_numHops, &u32_hopTimeout, &u8_failureLimit);
            POLL *polls = SNSL_MergeConfig();

            if(u8_menuIn == '1')
            {
                outString("\n\nSetting Clock:\n");
                RTCC_GetDateFromUserNoWday(&u_RTCC);
                RTCC_Set(&u_RTCC);
                RTCC_Read(&u_RTCCatStartup);
                outString("\n\nClock set. Returning to menu....\n");
                WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            }
            else if(u8_menuIn == '2')
            {
                outString("\n\nSetting network hops:\n");
                outString("\nThis value is the maximum number of hops "
                          "from the collector to the farthest node\n"
                          "\nCurrent number of hops: ");
                outUint8Decimal(u8_numHops);
                outString("\nNew number of hops[001-255]: ");
                char hops_buff[4];
                inStringEcho(hops_buff, 3);
                u8_numHops = SNSL_Atoi((uint8 *)hops_buff);
                outString("\nNew number of hops saved. Returning to menu....\n");
                WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            }
            else if(u8_menuIn == '3')
            {
                outString("\n\nSetting timeout value:\n");
                outString("\nThis value is the timeout per network hop (default is 300ms).\n");
                printf("\nCurrent timeout value: %lu\n", u32_hopTimeout);
                outString("New timeout value (milliseconds)[0300-9999]: ");
                char timeout_buff[5];
                inStringEcho(timeout_buff, (uint16)4);
                u32_hopTimeout = SNSL_Atoi((uint8 *)timeout_buff);
                /***NOTE**
                for some reason, the SNSL_Atoi does not work for > 6 digits. The
                return value seems to overflow to the max int size. This isn't a
                problem due to the constraints set to 4 digits, so it's being
                ignored*/
                outString("\nNew timeout value saved. Returning to menu....\n");
                WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            }
            else if(u8_menuIn == '4')
            {
                outString("\n\nSetting new failure threshold:\n");
                outString("\nThis value is the number of failures to respond "
                          "a sensor node has before it will be ignored.\n"
                          "\nCurrent failure limit: ");
                outUint8Decimal(u8_failureLimit);
                outString("\nNew failure limit [001-255]: ");
                char failure_buff[4];
                inStringEcho(failure_buff, 3);
                u8_failureLimit = (uint8)SNSL_Atoi((uint8 *)failure_buff);
                outString("\nNew failure limit saved. Returning to menu....\n");
                WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            }
            else if(u8_menuIn == '5')
            {
                outString("\n\nResetting node ignore list....");
                uint8 u8_i = 0;

                while(polls[u8_i].attempts != LAST_POLL_FLAG)
                {
                    polls[u8_i].attempts = 0;
                    ++u8_i;
                }

                outString("\nNode ignore list reset. Returning to menu....\n");
                WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            }
            else if(u8_menuIn == '6')
            {
                outString("\n\nExiting setup......\n");
                outString("\nPlease move SETUP swtich from 'MENU' to 'NORM' to exit.");

                while(!TEST_SWITCH) {}

                outString("\n\nReturning to normal mode....\n\n");
            }

            SNSL_WriteConfig(u8_numHops, u32_hopTimeout, u8_failureLimit, polls);
            free(polls);
        }
        else
        {
            DELAY_MS(500);  // Wait for SLEEP_INPUT to stabilize before attaching interrupt
            _INT1IF = 0;    // Clear interrupt flag
            _INT1IP = 2;    // Set interrupt priority
            _INT1EP = 0;    // Set rising edge (positive) trigger
            _INT1IE = 1;    // Enable the interrupt
            _LATB7 = 1;     // Cut power to VDIP
            u8_newFileName = 1;

            while(1)
            {
                SLEEP();
                NOP();      //insert nop after wake from sleep to solve stabiity issues

                /* we've woken up from sleep, so we need to poll all of the sensor nodes
                *  and then return to sleep*/
                configHighPower();  //reconfigure pins from low-power to high-power mode
                uint8 u8_pollReturn = 0;
                POLL *polls;
                u8_pollsCompleted = u8_pollsIgnored = u8_pollsFailed = 0;

                /* SLEEP_INPUT is the wire from the SNAP module that we use to wake up.
                *  While the mesh is asleep, SLEEP_INPUT is LOW. When the mesh wakes, SLEEP_INPUT
                *  goes high, the interrupt fires, and we wake from the above SLEEP() loop. We 
                *  stay awake while the SLEEP_INPUT line is HIGH*/
                while(SLEEP_INPUT)
                {
                    if(isCharReady2())
                    {
                        if(isMeshUp() == 0x01)
                        {
                            sendStayAwake();
                            WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();
                            VDIP_Init();

                            /*Check to see if VDIP is present. If not, return mesh to sleep and
                            * check again on the next cycle*/
                            if(!VDIP_DiskExists())
                            {
                                outString("ERROR: VDIP does not exist. Returning to sleep.\n");
                                sendEndPoll();
                                continue;
                            }

                            /* This block is used to calculate what the filename for the data output file
                            *  should be. The file name is always the month and day of the last time the 
                            *  collector was turned on, plus another number to differentiate it if there are
                            *  multiple files present with the same month and day combination.*/
                            uint8 fileCount = 1;
                            if(u8_newFileName)
                            {
                                sprintf(filename, "%02x%02x%02d.TXT", u_RTCCatStartup.u8.month, u_RTCCatStartup.u8.date, fileCount);

                                while(VDIP_FileExists((uint8 *)filename))
                                {
                                    sprintf(filename, "%02x%02x%02d.TXT", u_RTCCatStartup.u8.month, u_RTCCatStartup.u8.date, ++fileCount);
                                }
                                
                                u8_newFileName = 0; //use this flag so that we only change the file name during the first poll after power up
                                VDIP_Sync();
                            }
                            
                            /* Get the list of node names and their response failures from the VDIP NODES.TXT and CONFIG.TXT files*/
                            polls = SNSL_MergeConfig();

                            /* Get the remaining user configured settings from CONFIG.TXT*/
                            SNSL_ParseConfigHeader(&u8_numHops, &u32_hopTimeout, &u8_failureLimit);

                            /* Start polling: log polling started event with timestamp*/
                            uint8 u8_i = 0;
                            RTCC_Read(&u_RTCC);
                            SNSL_LogPollEvent(0x00, &u_RTCC);  //log polling started

                            /* The structure has a dummy entry at teh end (similar to null-terminated) so we know when to stop
                            *  looping over the structure*/
                            while(polls[u8_i].attempts != LAST_POLL_FLAG)
                            {
                                /* Check that the current node hasn't failed more times than the threshold*/
                                if(polls[u8_i].attempts <= u8_failureLimit)
                                {
                                    /*Send first poll request of cycle to node w/ 0xff as timestamp to indicate this is the first request*/
                                    u8_pollReturn = doPoll(polls[u8_i].name[0],
                                                           polls[u8_i].name[1],
                                                           polls[u8_i].name[2],
                                                           0xff, 0xff, 0xff);
                                    /*Poll was successful, reset failure count and increment total poll count for this cycle*/
                                    if(u8_pollReturn == 0x01)
                                    {
                                        polls[u8_i].attempts = 0;
                                        u8_pollsCompleted++;
                                    }
                                    /*Poll failed, and we're logging failures, so increment failure count, log the response failure
                                    * in the log file and increment the total failures for this cycle*/
                                    else if(u8_pollReturn == 0x00)
                                    {
                                        ++polls[u8_i].attempts;
                                        RTCC_Read(&u_RTCC);
                                        SNSL_LogResponseFailure(polls[u8_i].attempts, polls[u8_i].name[0],
                                                                polls[u8_i].name[1], polls[u8_i].name[2],
                                                                &u_RTCC);
                                        u8_pollsFailed++;
                                    }
                                    /*Poll failed, but we're not tracking failures. Keep failure count at 0, but log that the failure occured*/
                                    else if(u8_pollReturn == 0x02)
                                    {
                                        RTCC_Read(&u_RTCC);
                                        polls[u8_i].attempts = 0;
                                        SNSL_LogResponseFailure(polls[u8_i].attempts, polls[u8_i].name[0],
                                                                polls[u8_i].name[1], polls[u8_i].name[2],
                                                                &u_RTCC);
                                    }
                                }
                                /*The node has exceeded the failure threshold, so it was ignored. Log that it was ignored and increment the
                                * total ignored count for this cycle*/
                                else
                                {
                                    RTCC_Read(&u_RTCC);
                                    SNSL_LogNodeSkipped(polls[u8_i].name[0],
                                                        polls[u8_i].name[1],
                                                        polls[u8_i].name[2],
                                                        &u_RTCC);
                                    u8_pollsIgnored++;
                                }

                                ++u8_i;
                            }

                            sendEndPoll();
                            /*Write out the response failures to CONFIG.TXT*/
                            SNSL_WriteConfig(u8_numHops, u32_hopTimeout, u8_failureLimit, polls);
                            free(polls);
                            /*Log that polling has stopped and log out the statistics for this polling cycle*/
                            RTCC_Read(&u_RTCC);
                            SNSL_LogPollEvent(0x01, &u_RTCC);
                            SNSL_LogPollingStats(&u_RTCC, u8_pollsCompleted, u8_pollsIgnored, u8_pollsFailed);
                        }
                    }
                }

                // Turn the VDIP off
                _LATB7 = 1;
                // Return to low-power mode
                SNSL_ConfigLowPower();
            }
        }
    }

    return 0;
}
