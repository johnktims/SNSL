#include "pic24_all.h"
#include "packet.h"
#include "snsl.h"
#include "rtcc.h"
#include "adc_sample.h"

#include <stdio.h>  //for 'printf' | 'sprintf'
#include <string.h> //for 'memcpy'

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
 * Pin Assignment Macros
 **********************************************************/
#define SLEEP_INPUT   _RB9
#define TEST_SWITCH   _RB8
#define ANALOG_POWER  _RB7

/***********************************************************
 * Pin configuration functions
 **********************************************************/

 /**********************************************************
  *
  * @brief Sleep Input pin configuration
  *
  **********************************************************/
inline void CONFIG_SLEEP_INPUT()
{
    CONFIG_RB9_AS_DIG_INPUT();
    DISABLE_RB9_PULLUP();
    DELAY_US(1);
}

/**********************************************************
 *
 * @brief Test Mode Switch pin configuration
 *
 **********************************************************/
inline void CONFIG_TEST_SWITCH()
{
    CONFIG_RB8_AS_DIG_INPUT();
    ENABLE_RB8_PULLUP();
    DELAY_US(1);
}

/**********************************************************
 *
 * @brief Analog circuitry power control pin configuration
 *
 **********************************************************/
inline void CONFIG_ANALOG_POWER()
{
    CONFIG_RB7_AS_DIG_OD_OUTPUT();
    DELAY_US(1);
}

/**********************************************************
 *
 * @brief ADC analog input pin configurations
 *
 **********************************************************/
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
    CONFIG_SLEEP_INPUT();
    CONFIG_TEST_SWITCH();
    CONFIG_INT1_TO_RP(9);   //connect interrupt to sleep input
    CONFIG_ANALOG_POWER();
    CONFIG_ANALOG_INPUTS();
    _LATB7 = 0;     //enable power to analog circuitry
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
    _INT1IF = 0;        //clear interrupt flag before exiting
}


/**********************************************************
 * Polling Functions
 **********************************************************/
 
/**********************************************************
 *
 * @breif Respond to data poll request
 * @note  This function reads in the data request and
 *        responds to it with data.
 *
 **********************************************************/
void parseInput(void)
{
    DELAY_MS(5);

    outString("Stored samples at beginning of poll response:\n");
    SNSL_PrintSamples(pollData);    //print stored samples array
    uint8 u8_c,
          x, y;
    RTCC ack_time;
    UFDATA fdata;
    
    SNSL_GetNodeName(&fdata);   //read in node name from program memory
    
    u8_c = inChar2();   //first byte is always garbage
    ack_time.u8.hour = inChar2();   //read in the ACK timestamp from poll request
    ack_time.u8.min  = inChar2();
    ack_time.u8.sec  = inChar2();
    
    uint8 tmp = SNSL_NewestSample(pollData);    //get index of newest stored sample in array
    /* Initially set the data point to send out to be the newest stored sample (if any)*/
    STORED_SAMPLE *to_send = &pollData[tmp];
    //printf("Newest index: %d", tmp);

    /*There are only two packet types we want to respond to, a data request or a single ACK*/
    if(u8_c != MONITOR_REQUEST_DATA_STATUS)
    {
        if(u8_c != ACK_PACKET)
        {
            outString("REQUEST FAILURE\n");
            return;
        }
    }

    /* If the ACK timestamp is all 0xff, then we have received a request for new data. We need
    *  to read the ADC again, check its timestamp vs the read we took when we woke up from sleep, and
    *  either store the one taken at wake, or store it if we have missed a sleep message*/
    if(ack_time.u8.hour == 0xff && ack_time.u8.min == 0xff && ack_time.u8.sec == 0xff)
    {
        RTCC_Read(&poll_loop.ts);   //read current time into new ADC sample
        sampleProbes((FLOAT *)&poll_loop.samples);   //sample ADC for redox data, store in data buffer
        outChar('\n');
        to_send = &poll_loop;       //set data point to send out to the ADC read just taken

        /* Check the difference in timestamps between the wake read and the current read. If greater
        *  than the mesh sleep time, push the wake read into the storage array, otherwise discard it.*/
        if(SNSL_TimeDiff(poll_wake.ts, poll_loop.ts) > MESH_SLEEP_MINS * 60)
        {
            SNSL_InsertSample(pollData, poll_wake);
        }
    }
    /* Otherwise, we've recieved an ACK for a data point we have already sent out. We need to mark that data
    *  point as having been sent*/
    else
    {
        SNSL_ACKSample(pollData, ack_time); //find the sample in the array and mark it as having been ACK'd
        outString("Stored samples after ACK\n");
        SNSL_PrintSamples(pollData);
        tmp = SNSL_NewestSample(pollData);
        to_send = &pollData[tmp];   //set data point to send out to newest stored sample
        //printf("ACKd; Newest index: %d", tmp);
    }

    /*Build and send response to data poll based on the data point to be sent out (as determine above)
    * packet structure: 0x1E(1)|Packet Length(1)|Packet Type(1)|Remaining polls(1)|Timestamp(6)|Temp data(20)|Redox data(16)|
    *                   Reference Voltage(4)|Node Name(12 max)*/
    SendPacketHeader();
    /*Length = packet type and remaining polls (2B) + timestamp (6B) + temp (4B*5=20B) + redox (4B*4=16B) + ref (4B) + 
               node name (<= 12B)*/
    uint8 length = 2 + 6 + sizeof(float) * 5 + sizeof(float) * 4 + sizeof(float) + strlen((char *)fdata.dat.node_name);
    outChar2(length);
    //printf("TX Packet Size: 0x%2x\n", length);
    outChar2(APP_SMALL_DATA);   //send packet type
    outChar2(SNSL_TotalSamplesInUse(pollData));  //remaining polls
    //send timestamp
    outChar2(to_send->ts.u8.month);
    outChar2(to_send->ts.u8.date);
    outChar2(to_send->ts.u8.yr);
    outChar2(to_send->ts.u8.hour);
    outChar2(to_send->ts.u8.min);
    outChar2(to_send->ts.u8.sec);
    //send redox, temperature, and reference voltage values
    for(x = 0; x < NUM_ADC_PROBES; ++x)
    {
        for(y = 0; y < sizeof(float); ++y)
        {
            outChar2(to_send->samples[x].s[y]);
        }
    }
    //send node name from program memory
    outString2((char *)fdata.dat.node_name);
    u8_polled = 1;  //mark that we have been polled
    outString("Poll Response Complete\n\n");
}


/****************************MAIN******************************************/
int main(void)
{
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

    UFDATA fdata;
    SNSL_GetNodeName(&fdata);

    /* After programming, all of the unused program memory is set to 0xff. If
    *  we haven't set a node name yet, and we try to send this out, everything breaks.
    *  So, we set a default node name to prevent everything from breaking.*/
    if(fdata.dat.node_name[0] == 0xFF)
    {
        SNSL_SetNodeName(DEFAULT_NODE_NAME);
    }

    SNSL_InitSamplesBuffer(pollData);

    while(1)
    {
        if(!TEST_SWITCH)    //in setup mode
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
                RTCC_Set(&u_RTCC);
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
                /* Now that we're awake, we need to respond to any data requests we receive*/
                configHighPower();
                U2MODEbits.UARTEN = 1;                    // enable UART RX/TX
                U2STAbits.UTXEN   = 1;                    //enable the transmitter

                if(SLEEP_INPUT)
                {
                    u8_polled = 0;  //mark that we're awake and we haven't been polled yet
                    // MPLAB cries if doze constant isn't put in
                    // a variable. Casting doesn't seem
                    // to work either.
                    uint8 doze = 8;
                    _DOZE = doze; //chose divide by 32
                    outString("Awake....Reading ADC Data:\n");
                    
                    /*We take an ADC sample at wake. This may or may not be used later*/
                    sampleProbes((FLOAT *)&poll_wake.samples);   //sample ADC for redox data, store in data buffer
                    outChar('\n');
                    RTCC_Read(&poll_wake.ts);
                    outString("Stored samples at wake:\n");
                    SNSL_PrintSamples(pollData);

                    while(SLEEP_INPUT)
                    {
                        _DOZEN = 1; //enable doze mode, cut back on clock while waiting to be polled

                        //Wait until we receive something from the SNAP node
                        if(isCharReady2())
                        {
                            _DOZEN = 0; //return to normal mode
                            outString("Poll Request Received\n");
                            parseInput();  //satisfy the polling
                        }
                    }
                }

                if(u8_polled == 0)
                {
                    /*We've woken up, but we were never polled. We have to save the data sample we took
                    * at wake, and increment the total number of times we've been missed.*/
                    SNSL_InsertSample(pollData, poll_wake);
                    outString("ERROR: Poll Did Not Complete\n");
                    printf("Total Missed Polls: %d\n", SNSL_TotalSamplesInUse(pollData));
                    WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
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
