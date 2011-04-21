#include "pic24_all.h"
#include "packet.h"
#include "snsl.h"
#include "rtcc.h"

#include <stdio.h>
#include <stdlib.h>

/****************************GLOBAL VARIABLES****************************/
volatile uint8 u8_numHops, u8_failureLimit, u8_stopPolling,
         u8_pollsCompleted, u8_pollsIgnored, u8_pollsFailed,
         u8_newFileName, psz_out[128];

volatile uint32 u32_hopTimeout;

volatile union32 timer_max_val;

volatile unionRTCC u_RTCC, u_RTCCatStartup;

volatile char filename[13];

/****************************PIN CONFIGURATION****************************/
#define SLEEP_INPUT _RB14
#define TEST_SWITCH _RA2
#define VDIP_POWER	_RB7
#define SLEEP_TIME	_RB8

/// Sleep Input pin configuration
inline void CONFIG_SLEEP_INPUT(void)
{
    //use RB14 for mode input
    CONFIG_RB14_AS_DIG_INPUT();
    DISABLE_RB14_PULLUP();
    DELAY_US(1);
}

//Test Mode Switch pin configuration
inline void CONFIG_TEST_SWITCH(void)
{
    CONFIG_RA2_AS_DIG_INPUT();
    ENABLE_RA2_PULLUP();
    DELAY_US(1);
}

//Sleep time pin configuration
inline void CONFIG_SLEEP_TIME(void)
{
    CONFIG_RB8_AS_DIG_INPUT();
    ENABLE_RB8_PULLUP();
    DELAY_US(1);
}

void configHighPower(void)
{
    /*  SNSL_configLowPower resets all pins to be analog inputs in
     *  order to save power. As such, we have to re-define all of
     *  the digital I/O pins when we come out of low power mode
     */
    _LATB7 = 0;		//enable power to VDIP
    CONFIG_SLEEP_INPUT();
    CONFIG_TEST_SWITCH();
    CONFIG_VDIP_POWER();
    CONFIG_SLEEP_TIME();

    CONFIG_INT1_TO_RP(14);
}

/****************************TIMER CONFIGURATION****************************/
void configTimer23(void)
{
    T2CON = T2_OFF | T2_IDLE_CON | T2_GATE_OFF | T2_32BIT_MODE_ON
            | T2_SOURCE_INT | T2_PS_1_1;
}

void startTimer23(void)
{
    timer_max_val.u32 = usToU32Ticks(((u32_hopTimeout*u8_numHops)*1000), getTimerPrescale(T2CONbits)) - 1;

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

/****************************POLLING FUNCTIONS****************************/
uint8 blocking_inChar(void)
{
    uint8 u8_char = '~';
    startTimer23();
    // Wait for character
    while(!isCharReady2() && !u8_stopPolling) {}
    if(!u8_stopPolling)
    {
        T2CONbits.TON = 0;
        u8_char = inChar2();
    }

    //printf("char input: %c | %x\n", u8_char, u8_char);
    return u8_char;
}

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
            return 0x01;
        }
    }
    return 0x0;
}

void sendStayAwake(void)
{
    //Packet Format: 0x1E + length + groupID LSB + grpID MSB + TTL + 0x02 + 'sleepyNodeFalse'
    const char sz_data[] = "sleepyModeFalse";

    SendPacketHeader();  //0x1E
    outChar2(0x10);	     //packet length = 1 + string length
    outChar2(0x01);	     //LSB of group ID is 0001
    outChar2(0x00);      //MSB of group ID
    outChar2(0x05);	     //TTL
    outChar2(0x02);      //MRPC packet type
    outString2(sz_data);
}


uint8 doPoll(char c_ad1, char c_ad2, char c_ad3, uint8 hr, uint8 min, uint8 sec)
{
    u8_stopPolling = 0;

    printf("Polling Node %02X%02X%02X\n", c_ad1, c_ad2, c_ad3);

    SendPacketHeader();
    outChar2(0x05);		//packet length
    outChar2(c_ad1);
    outChar2(c_ad2);
    outChar2(c_ad3);
    outChar2(0x03);	//Appdata packet (forward data directly to PIC)
    outChar2(MONITOR_REQUEST_DATA_STATUS);
    outChar2(hr);
    outChar2(min);
    outChar2(sec);
    //puts("---> Sent");
    WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();

    uint8 size = 2 + 6 + sizeof(float)*5 + sizeof(float)*4 + sizeof(float) + 12;    // == 60
    uint8 poll_data[size];
    uint8 i = 0;
    for(i = 0; i < size; ++i)
    {
        poll_data[i] = 0x00;
    }

    volatile uint8 tmp,
                   packet_length = 0,
                   remaining_polls = 0,
                   u8_char;

    //puts("---> Parsing Header");
    for(tmp = 0; tmp < 7; tmp++)    //full packet header == 7 bytes (header (1) | node address (3 bytes) | packet type (1)
    {   //                               | size (1) | remaining polls (1)
        u8_char = blocking_inChar();
        if (u8_char == 0 && tmp == 0) {
            u8_char = blocking_inChar();
        }
        //printf("char input: %c | %x\n", u8_char, u8_char);
        if(u8_char == '~')
        {
            u8_stopPolling = 1;
            break;
        }
        else
        {
            if(tmp == 4)
            {
                packet_length = u8_char - 2;    //subtract 2 to remove packet header and packet type from length

            }
            else if(tmp == 6)
            {
                remaining_polls = u8_char;
            }
        }
    }

    //printf("Remaining polls: `%u`\n", remaining_polls);
    //puts("---> Parse Packet");
    for(tmp = 0; tmp < packet_length; ++tmp)    //packet message == packet_length (max 58 == size-2)
    {   //this loop reads in remainder of packet message
        u8_char = blocking_inChar();
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

    if(!u8_stopPolling)
    {
        //puts("---> Format and Print");
        poll_data[packet_length] = '\0';

        // Print out the packet in ascii hex
        outString("RAW PACKET:");
        tmp = 0;
        for(tmp = 0; tmp < size-1; ++tmp)
        {
            outUint8(poll_data[tmp]);
            outChar(' ');
        }
        outString("\n");

        // Print out the packet in binary
        /*for(tmp = 0; tmp < size-1; ++tmp)
        {
            outChar(poll_data[tmp]);
        }*/

        uint8 nameStart = size - 12 - 2;    //size - 2 == max packet message - max node name size == start of node name
        // Variable-length node name, so put it in a null-terminated string for easy output
        uint8 psz_node_name[MAX_NODE_NAME_LEN];
        for(i = nameStart; i < packet_length; ++i)
        {
            psz_node_name[i-nameStart] = poll_data[i];
        }
        psz_node_name[i-nameStart] = 0x0;

        printf("NODE NAME: `%s`\n", psz_node_name);

        FLOAT probes[10];
        int x, offset = 6;
        for(x = 0; x < 10; offset+=sizeof(float),++x)
        {
            memcpy(probes[x].s, poll_data + offset, sizeof(float));
            //printf("\nP[%d].s = `%s`; .f=`%f`; data[%d]=%0X\n", x, probes[x].s, probes[x].f, offset, poll_data[offset]);
        }

        //date(MM/DD/YYY),time(HH:MM:SS),1.250,vref,sp1,sp2,sp3,sp4,temp1,temp2,temp3,temp4,temp5,nodeName
        uint8 psz_fmt[] =
            "%02x/%02x/%02x,"                // timestamp [MM/DD/YY]
            "%02x:%02x:%02x,"                // timestamp [HH:MM:SS]
            "1.250,"                         // some voltage reference (constant value)
            "%3.3f,"                         // battery voltage
            "%3.3f,%3.3f,%3.3f,%3.3f,"       // redox samples
            "%3.3f,%3.3f,%3.3f,%3.3f,%3.3f," // temperature samples
            "%s\n";                          // node name

        uint8* p = poll_data;
        sprintf(psz_out, psz_fmt,
                p[0],p[1],p[2],p[3],p[4],p[5], // Timestamp
                probes[0].f,                   // battery voltage
                probes[1].f, probes[2].f, probes[3].f, probes[4].f, // redox samples
                probes[5].f, probes[6].f, probes[7].f, probes[8].f, probes[9].f, // temperature samples
                psz_node_name); // node name
        psz_out[127] = 0x0;

        printf("FORMATTED PACKET:%s", psz_out);
        //outString(psz_out);

        // puts("---> Writing File");
        VDIP_WriteFile(filename, psz_out);
        //puts("---> Done writing polls");
        if (remaining_polls == 0x01 && hr != 0xff) {
            // send ACK packet
             SendPacketHeader();
             outChar2(0x05);		//packet length
             outChar2(c_ad1);
             outChar2(c_ad2);
             outChar2(c_ad3);
             outChar2(0x03);	//Appdata packet (forward data directly to PIC)
             outChar2(ACK_PACKET);
             outChar2(p[3]);
             outChar2(p[4]);
             outChar2(p[5]);
             //puts("---> Sent");
             WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();
        }    
        else if(remaining_polls != 0x00)
        {
            doPoll(c_ad1, c_ad2, c_ad3, p[3], p[4], p[5]);
        }
        //puts("---> Success");
        return 0x01;
    }
    else if(SLEEP_TIME)
    {
        //puts("---> Fail and record");
        //record node response failure
        //do not record failure if in mesh setup mode (SLEEP_PIN == HIGH)
        return 0x00;
    }
    else {
        //puts("---> Fail and don't record");
        return 0x02;
    }
}

void sendEndPoll(void)
{
    const char sz_data[] = "pollingStopped";

    SendPacketHeader(); //0x1E
    outChar2(0x0F);	//packet length
    outChar2(0x01);
    outChar2(0x00);
    outChar2(0x05);	//TTL
    outChar2(0x02);	//MRPC packet type
    outString2(sz_data);
}

/****************************INTERRUPT HANDLERS****************************/
/*
*   This interrupt exists only to wake the PIC from sleep mode. It is attached
*   to the SLEEP_INPUT pin and is configured to fire whenever on the rising
*   edge of that pin. As such, it's only duty is to clear it's own flag.
*/
void _ISRFAST _INT1Interrupt(void) {
    _INT1IF = 0;        //clear interrupt flag
}

void _ISRFAST _T3Interrupt(void)
{
    u8_stopPolling = 1;
    T2CONbits.TON  = 0;             //stop the timer
    outString("Timer Expired\n");
    _T3IF = 0;	                    //clear interrupt flag
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

    while (!RCFGCALbits.RTCSYNC) {
    }
    RTCC_Read(&u_RTCCatStartup);
    //RTCC_Print(&u_RTCC);
    //printf("Month: %2x Day: %2x Year: %2x\n", (uint16)u_RTCCatStartup.u8.month,
    //        (uint16)u_RTCCatStartup.u8.date, (uint16)u_RTCCatStartup.u8.yr);

    while (1) {

        if(!TEST_SWITCH)
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

            /*if (u8_menuIn != '6') {
                outString("\n\nInitilizing Setup. Please wait....\n\n");
            }  */
            //VDIP_Init();

            SNSL_ParseConfigHeader(&u8_numHops, &u32_hopTimeout, &u8_failureLimit);
            POLL *polls = SNSL_MergeConfig();

            if(u8_menuIn == '1')
            {
                outString("\n\nSetting Clock:\n");
                RTCC_GetDateFromUserNoWday(&u_RTCC);
                //outString("\n\nInitializing clock....");
                RTCC_Set(&u_RTCC);
                //outString("\nTesting clock (press any key to end):\n");
                /*while(!isCharReady())
                {
                    if(RCFGCALbits.RTCSYNC)
                    {
                        RTCC_Read(&u_RTCC);
                        RTCC_Print(&u_RTCC);
                        DELAY_MS(700);
                    }
                }*/
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
                u8_numHops = (uint8)SNSL_Atoi(hops_buff);
                outString("\nNew number of hops saved. Returning to menu....\n");
                WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            }
            else if(u8_menuIn == '3')
            {
                outString("\n\nSetting timeout value:\n");
                outString("\nThis value is the timeout per network hop (default is 300ms).\n");
                printf("\nCurrent timeout value: %lu\n", u32_hopTimeout);

                outString("New timeout value (milliseconds)[0300-9999]: ");
                uint8 timeout_buff[5];
                inStringEcho(timeout_buff, 4);
                u32_hopTimeout = SNSL_Atoi(timeout_buff);
                /***NOTE**
                for some reason, the SNSL_Atoi does not work for > 6 digits. The
                return value seems to overflow to the max int size. This isn't a
                problem due to the constraints set to 4 digits, so it's being
                ignored*/
                //printf("Timeout: %lu\n", u32_hopTimeout);
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
                u8_failureLimit = (uint8)SNSL_Atoi(failure_buff);
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
            else if(u8_menuIn == '6') {
                outString("\n\nExiting setup......\n");
                outString("\nPlease move SETUP swtich from 'MENU' to 'NORM' to exit.");
                while (!TEST_SWITCH) {}
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

            while (1)
            {
                SLEEP();
                NOP();      //insert nop after wake from sleep to solve stabiity issues

                //do code from interrupt here
                configHighPower();


                uint8 u8_pollReturn = 0;
                POLL *polls;
                u8_pollsCompleted = u8_pollsIgnored = u8_pollsFailed = 0;

                while(SLEEP_INPUT)
                {
                    /*if(!isCharReady2() && isMeshUp() != 0x01)
                    {
                        continue;
                    }*/
                    if(isCharReady2())
                    {
                        if(isMeshUp() == 0x01)
                        {
                    sendStayAwake();
                    WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();

                    VDIP_Init();

                    //outString("Checking disk: \n");
                    if(!VDIP_DiskExists())
                    {
                        outString("VDIP does NOT exist\n");
                        sendEndPoll();
                        continue;
                    }

                    uint8 fileCount = 1;
                    if(u8_newFileName)
                    {
                        sprintf(filename, "%02x%02x%02d.TXT", u_RTCCatStartup.u8.month, u_RTCCatStartup.u8.date, fileCount);
                        printf("Evaluating: `%s`\n", filename);
                        while(VDIP_FileExists(filename))
                        {
                            sprintf(filename, "%02x%02x%02d.TXT", u_RTCCatStartup.u8.month, u_RTCCatStartup.u8.date, ++fileCount);
                            printf("Evaluating: `%s`\n", filename);
                        }
                        //outString("out of while\n");
                        printf("New filename: `%s`\n", filename);
                        u8_newFileName = 0;
                        VDIP_Sync();
                    }
                    //sprintf(filename, "data.txt");

                    polls = SNSL_MergeConfig();
                    //puts("--- Printing MergeConfig");
                    //SNSL_PrintPolls(polls);

                    SNSL_ParseConfigHeader(&u8_numHops, &u32_hopTimeout, &u8_failureLimit);
                    //u32_hopTimeout = 300;
                    uint8 u8_i = 0;

                    RTCC_Read(&u_RTCC);
                    SNSL_LogPollEvent(0x00, &u_RTCC);  //log polling started
                    while(polls[u8_i].attempts != LAST_POLL_FLAG)
                    {
                        if(polls[u8_i].attempts <= u8_failureLimit)
                        {
                            u8_pollReturn = doPoll(polls[u8_i].name[0],
                                                   polls[u8_i].name[1],
                                                   polls[u8_i].name[2],
                                                   0xff, 0xff, 0xff);

                            if(u8_pollReturn == 0x01)
                            {
                                polls[u8_i].attempts = 0;
                                u8_pollsCompleted++;
                            }
                            else if(u8_pollReturn == 0x00)
                            {
                                ++polls[u8_i].attempts;
                                RTCC_Read(&u_RTCC);

                                SNSL_LogResponseFailure(polls[u8_i].attempts, polls[u8_i].name[0],
                                                        polls[u8_i].name[1], polls[u8_i].name[2],
                                                        &u_RTCC);
                                u8_pollsFailed++;
                            }
                            else if (u8_pollReturn == 0x02) {
                                RTCC_Read(&u_RTCC);
                                polls[u8_i].attempts = 0;
                                SNSL_LogResponseFailure(polls[u8_i].attempts, polls[u8_i].name[0],
                                                        polls[u8_i].name[1], polls[u8_i].name[2],
                                                        &u_RTCC);
                            }
                        }
                        else
                        {
                            //log node was ignored
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
                    SNSL_WriteConfig(u8_numHops, u32_hopTimeout, u8_failureLimit, polls);
                    free(polls);

                    // Log the time at which we stopped polling the sensor nodes
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
