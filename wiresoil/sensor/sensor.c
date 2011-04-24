#include "pic24_all.h"
#include "packet.h"
#include "snsl.h"
#include "rtcc.h"
#include "adc_sample.h"

#include <stdio.h>  // printf
#include <string.h> // malloc


/***********************************************************
 * Macro Definitions
 **********************************************************/
#define SLEEP_INPUT   _RB9
#define TEST_SWITCH   _RB8
#define ANALOG_POWER  _RB7

/***********************************************************
 * Global Variables
 **********************************************************/
// 
uint8 u8_polled = 0;

RTCC u_RTCC;

STORED_SAMPLE pollData[MAX_STORED_SAMPLES];
STORED_SAMPLE poll_wake,
              poll_loop;


/***********************************************************
 * Pin Configurations
 **********************************************************/
/// Sleep Input from wireless module
inline void CONFIG_SLEEP_INPUT()
{
    CONFIG_RB9_AS_DIG_INPUT();     //use RB9 as sleep input
    DISABLE_RB9_PULLUP();
    DELAY_US(1);
}

// Test Mode Switch
inline void CONFIG_TEST_SWITCH()
{
    CONFIG_RB8_AS_DIG_INPUT();
    ENABLE_RB8_PULLUP();
    DELAY_US(1);
}

// Analog Power pin configuration
inline void CONFIG_ANALOG_POWER()
{
    CONFIG_RB7_AS_DIG_OD_OUTPUT();
    DELAY_US(1);
}

//Analog input pin configuration
inline void CONFIG_ANALOG_INPUTS()
{
    //configure ADC pins
    CONFIG_AN0_AS_ANALOG();
    CONFIG_AN1_AS_ANALOG();
    CONFIG_AN2_AS_ANALOG();
    CONFIG_AN3_AS_ANALOG();
    CONFIG_AN4_AS_ANALOG();
    CONFIG_AN5_AS_ANALOG();
    CONFIG_AN9_AS_ANALOG();
    CONFIG_AN10_AS_ANALOG();
    CONFIG_AN11_AS_ANALOG();
    CONFIG_AN12_AS_ANALOG();
    DELAY_US(1);
}

void configHighPower(void)
{
    CONFIG_SLEEP_INPUT();
    CONFIG_TEST_SWITCH();
    CONFIG_INT1_TO_RP(9);   //connect interrupt to sleep input
    CONFIG_ANALOG_POWER();
    CONFIG_ANALOG_INPUTS();
    _LATB7 = 0;     //enable power to analog circuitry
}

/****************************POLLING FUNCTIONS****************************/
void parseInput(void)
{
    DELAY_MS(5);
    outString("In Parse Input");
    SNSL_PrintSamples(pollData);
    uint8 u8_c,
          x, y;
    RTCC ack_time;
    UFDATA fdata;
    SNSL_GetNodeName(&fdata);
    u8_c = inChar2();
    ack_time.u8.hour = inChar2();
    ack_time.u8.min  = inChar2();
    ack_time.u8.sec  = inChar2();
    uint8 tmp = SNSL_NewestSample(pollData);
    STORED_SAMPLE *to_send = &pollData[tmp];
    printf("parseInput; Newest index: %d", tmp);

    if(u8_c != MONITOR_REQUEST_DATA_STATUS)
    {
        if(u8_c != ACK_PACKET)
        {
            outString("REQUEST FAILURE\n");
            return;
        }
    }

    // Request for new data
    if(ack_time.u8.hour == 0xff && ack_time.u8.min == 0xff && ack_time.u8.sec == 0xff)
    {
        RTCC_Read(&poll_loop.ts);
        sampleProbes((FLOAT *)&poll_loop.samples);   //sample ADC for redox data, store in data buffer
        to_send = &poll_loop;

        // If difference is greater than wake cycle, then push current
        // poll onto the buffer.
        if(SNSL_TimeDiff(poll_wake.ts, poll_loop.ts) > MESH_SLEEP_MINS * 60)
        {
            SNSL_InsertSample(pollData, poll_wake);
        }
    }
    // An ACK for the poll data with the given timestamp
    else
    {
        SNSL_ACKSample(pollData, ack_time);
        SNSL_PrintSamples(pollData);
        tmp = SNSL_NewestSample(pollData);
        to_send = &pollData[tmp];
        printf("ACKd; Newest index: %d", tmp);
    }

    //packet structure: 0x1E(1)|Packet Length(1)|Packet Type(1)|Remaining polls(1)|Timestamp(6)|Temp data(10)|Redox data(8)|
    //                  Reference Voltage(2)|Node Name(12 max)
    SendPacketHeader();
    uint8 length = 2 + 6 + sizeof(float) * 5 + sizeof(float) * 4 + sizeof(float) + strlen((char *)fdata.dat.node_name);
    outChar2(length);
    printf("TX Packet Size: 0x%2x\n", length);
    outChar2(APP_SMALL_DATA);
    outChar2(SNSL_TotalSamplesInUse(pollData));  //remaining polls
    //outString("MDYHMS");
    outChar2(to_send->ts.u8.month);
    outChar2(to_send->ts.u8.date);
    outChar2(to_send->ts.u8.yr);
    outChar2(to_send->ts.u8.hour);
    outChar2(to_send->ts.u8.min);
    outChar2(to_send->ts.u8.sec);

    for(x = 0; x < NUM_ADC_PROBES; ++x)
    {
        for(y = 0; y < sizeof(float); ++y)
        {
            outChar2(to_send->samples[x].s[y]);
        }
    }

    //outString2("10tempData");
    //outString2("8redData");
    //outString2("rV");
    outString2((char *)fdata.dat.node_name);
    //outString2("12_node_test");
    u8_polled = 1;
    outString("Poll Response Complete\n\n");
}

/****************************INTERRUPT HANDLERS****************************/
void _ISRFAST _INT1Interrupt(void)
{
    _INT1IF = 0;        //clear interrupt flag before exiting
}


/****************************MAIN******************************************/
int main(void)
{
    //SNSL_ConfigLowPower();
    configClock();
    configDefaultUART(DEFAULT_BAUDRATE);
    configUART2(DEFAULT_BAUDRATE);
    CONFIG_SLEEP_INPUT();
    CONFIG_TEST_SWITCH();
    CONFIG_ANALOG_POWER();
    CONFIG_ANALOG_INPUTS();
    // Connect interrupt to sleep pin
    CONFIG_INT1_TO_RP(9);
    // Use 32kHz external clock
    __builtin_write_OSCCONL(OSCCON | 0x02);
    // Initialize the Real-Time Clock Calendar
    RTCC_SetDefaultVals(&u_RTCC);
    RTCC_Set(&u_RTCC);
    //
    UFDATA fdata;
    SNSL_GetNodeName(&fdata);

    if(fdata.dat.node_name[0] == 0xFF)
    {
        SNSL_SetNodeName(DEFAULT_NODE_NAME);
    }

    SNSL_InitSamplesBuffer(pollData);

    while(1)
    {
        if(!TEST_SWITCH)
        {
            // Disable the sleep interrupt
            _INT1IE = 0;
            uint8 u8_menuIn;
            outString("\n\n\nSetup Mode:\n\n");
            outString("Choose an option -\n\n");
            outString("1. Configure Clock\n");
            outString("2. Set Node Name\n");
            outString("3. Exit Setup Mode\n");
            outString("--> ");
            u8_menuIn = inChar();
            DELAY_MS(100);

            if(u8_menuIn == '1')
            {
                outString("\n\nSetting Clock:\n");
                RTCC_GetDateFromUserNoWday(&u_RTCC);
                //outString("\n\nInitializing clock....");
                RTCC_Set(&u_RTCC);
                /*outString("\nTesting clock (press any key to end):\n");
                while (!isCharReady())
                {
                    if (RCFGCALbits.RTCSYNC)
                    {
                        RTCC_Read(&u_RTCC);
                        RTCC_Print(&u_RTCC);
                        DELAY_MS(700);
                    }
                }*/
                outString("\n\nClock set. Returning to menu....\n");
                WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            }

            if(u8_menuIn == '2')
            {
                outString("\n\nSetting node name:\n");
                outString("\nCurrent node name: ");
                SNSL_PrintNodeName();
                outString("\n\nEnter new node name [maximum 12 characters]: ");
                uint8 name_buff[13];
                inStringEcho((char *)name_buff, 12);
                SNSL_SetNodeName(name_buff);
                outString("\nNew node name saved. Returning to menu....\n");
                //outString(name_buff);
                WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            }
            else if(u8_menuIn == '3')
            {
                outString("\n\nExiting setup......\n");
                outString("\nPlease move SETUP swtich from 'MENU' to 'NORM' to exit.");

                while(!TEST_SWITCH) {}

                outString("\n\nReturning to normal mode....\n\n");
            }
        }
        else
        {
            DELAY_MS(500);      // Wait for SLEEP_INPUT to stabilize before attaching interrupt
            _INT1IF = 0;        //clear interrupt flag
            _INT1IP = 2;        //set interrupt priority
            _INT1EP = 0;        //set rising edge (positive) trigger
            _INT1IE = 1;        //enable the interrupt
            WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            //now sleep until next MCLR
            WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();
            //disable UART2 to save power.
            U2MODEbits.UARTEN = 0;                    // disable UART RX/TX
            U2STAbits.UTXEN = 0;                      //disable the transmitter

            while(1)
            {
                SLEEP();         //macro for asm("pwrsav #0")
                NOP();          //insert nop after wake from sleep to solve stabiity issues
                //do code from interrupt here:
                configHighPower();
                U2MODEbits.UARTEN = 1;                    // enable UART RX/TX
                U2STAbits.UTXEN   = 1;                    //enable the transmitter

                if(SLEEP_INPUT)
                {
                    u8_polled = 0;
                    // MPLAB cries if doze constant isn't put in
                    // a variable. Casting doesn't seem
                    // to work either.
                    uint8 doze = 8;
                    _DOZE = doze; //chose divide by 32
                    outString("Awake....Reading ADC Data:\n");
                    sampleProbes((FLOAT *)&poll_wake.samples);   //sample ADC for redox data, store in data buffer
                    outChar('\n');
                    RTCC_Read(&poll_wake.ts);
                    outString("On wake\n");
                    SNSL_PrintSamples(pollData);

                    while(SLEEP_INPUT)
                    {
                        _DOZEN = 1; //enable doze mode, cut back on clock while waiting to be polled

                        //outString("Waiting for char\n");
                        if(isCharReady2())
                        {
                            _DOZEN = 0;
                            outString("Poll Request Received\n");
                            parseInput();  //satisfy the polling
                        }
                    }
                }

                if(u8_polled == 0)
                {
                    //node woke up but wasn't successfully polled | store data and increment current_poll
                    SNSL_InsertSample(pollData, poll_wake);
                    outString("ERROR: Poll Did Not Complete\n");
                    printf("Total Missed Polls: %d\n", SNSL_TotalSamplesInUse(pollData));
                }

                U2MODEbits.UARTEN = 0;                    // disable UART RX/TX
                U2STAbits.UTXEN = 0;                      //disable the transmitter
                SNSL_ConfigLowPower();
                _LATB7 = 1;     //cut power to analog circuitry
            }
        }
    }

    return 0;
}
