#include "pic24_all.h"
#include "packet.h"
#include "snsl.h"
#include "rtcc.h"
#include "adc_sample.h"
#include <string.h>

#define SLEEP_INPUT _RB9
#define TEST_SWITCH _RB8
#define ANALOG_POWER _RB7

/****************************GLOBAL VARIABLES****************************/
volatile int current_poll = 0, poll_count = 0;
volatile uint8 u8_polled = 0, final_old_ack = 0;

volatile unionRTCC u_RTCC;

volatile STORED_SAMPLE pollData[MAX_STORED_SAMPLES];
volatile FLOAT poll[NUM_ADC_PROBES];

/****************************PIN CONFIGURATION****************************/
/// Sleep Input pin configuration
inline void CONFIG_SLEEP_INPUT() 
{
    CONFIG_RB9_AS_DIG_INPUT();     //use RB9 as sleep input
    DISABLE_RB9_PULLUP();
    DELAY_US(1);
}

// Test Mode Switch pin configuration
inline void CONFIG_TEST_SWITCH()
{
    CONFIG_RB8_AS_DIG_INPUT();
    ENABLE_RB8_PULLUP();
    DELAY_US(1);
}

// Analog Power pin configuration
inline void CONFIG_ANALOG_POWER() {
    CONFIG_RB7_AS_DIG_OD_OUTPUT();
    DELAY_US(1);
}

//Analog input pin configuration
inline void CONFIG_ANALOG_INPUTS() {
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

void configHighPower(void) {
    CONFIG_SLEEP_INPUT();
    CONFIG_TEST_SWITCH();
    
    CONFIG_INT1_TO_RP(9);   //connect interrupt to sleep input
    
    CONFIG_ANALOG_POWER();
    CONFIG_ANALOG_INPUTS();
    _LATB7 = 0;     //enable power to analog circuitry
}    

/****************************POLLING FUNCTIONS****************************/
void parseInput(void){
    DELAY_MS(5);
    outString("In Parse Input");
    uint8 u8_c,
          u8_hour,
          u8_min,
          u8_sec,
          x, y;
          
    UFDATA fdata;
    SNSL_GetNodeName(&fdata);

    u8_c = inChar2();
    outChar(u8_c);
    u8_hour = inChar2();
    outChar(u8_hour);
    u8_min  = inChar2();
    outChar(u8_min);
    u8_sec  = inChar2();
    outChar(u8_sec);

    //outChar(u8_c);
    //outChar('\n');
    if (u8_c != MONITOR_REQUEST_DATA_STATUS) {
        if (u8_c != ACK_PACKET) {
            outString("REQUEST FAILURE\n");
            return;
        }    
    }
    
    // Request for new data
    if(u8_hour == 0xff && u8_min == 0xff && u8_sec == 0xff)
    {
        RTCC_Read(&u_RTCC);
        sampleProbes(poll);    //sample ADC for redox data, store in data buffer
        
        // If difference between the poll taken at wake and
        // the current poll is less than the time it takes for
        // the mesh to cycle, replace the newest poll in the buffer
        // with the current poll.
        if(((u_RTCC.u8.hour*360+u_RTCC.u8.min*60+u_RTCC.u8.sec) -
           (pollData[current_poll].ts.u8.hour*360 +
            pollData[current_poll].ts.u8.min*60 +
            pollData[current_poll].ts.u8.sec)) < MESH_SLEEP_MINS*60)
        {
            for(x = 0; x < NUM_ADC_PROBES; ++x)
            {
                pollData[current_poll].samples[x].f = poll[x].f;
            }
            pollData[current_poll].ts = u_RTCC;
        }
        
        // If difference is greater than wake cycle, then push current
        // poll onto the buffer.
        else
        {
            current_poll = ++current_poll % MAX_STORED_SAMPLES;    
            ++poll_count;
            if (poll_count > MAX_STORED_SAMPLES) {
                poll_count = MAX_STORED_SAMPLES;
            }
            for(x = 0; x < NUM_ADC_PROBES; ++x)
            {
                pollData[current_poll].samples[x].f = poll[x].f;
            }
            pollData[current_poll].ts = u_RTCC;
        }
    }
    
    // An ACK for the poll data with the given timestamp
    else
    {
        if(((u8_hour*360+u8_min*60+u8_sec) -
           (pollData[current_poll].ts.u8.hour*360 +
            pollData[current_poll].ts.u8.min*60 +
            pollData[current_poll].ts.u8.sec)) == 0)
        {
            if (poll_count != 0x00){
               if (final_old_ack != 1) {
                   --current_poll;
                   if (current_poll < 0) {
                       current_poll = MAX_STORED_SAMPLES;
                   }    
                   --poll_count;
                   if (poll_count == 0x00) {
                        //We have been ACK'd the 2nd to last old data packet and are sending the final
                        //packet. Set a flag so we will know if the final data packet isn't ACK'd
                        final_old_ack = 1;
                   } 
               }
               else {
                   --poll_count;
               }    
            }            
            else if (u8_c == ACK_PACKET) {
                final_old_ack = 0;
            } 
        }   
    }
        
    //packet structure: 0x1E(1)|Packet Length(1)|Packet Type(1)|Remaining polls(1)|Timestamp(6)|Temp data(10)|Redox data(8)|
    //					Reference Voltage(2)|Node Name(12 max)
    SendPacketHeader();
    uint8 length = 2 + 6 + sizeof(float)*5 + sizeof(float)*4 + sizeof(float) + strlen(fdata.dat.node_name);
    outChar2(length);
    printf("TX Packet Size: 0x%2x\n", length);
    outChar2(APP_SMALL_DATA);
    outChar2(poll_count);	//remaining polls
    //outString("MDYHMS");
    outChar2(pollData[current_poll].ts.u8.month);
    outChar2(pollData[current_poll].ts.u8.date);
    outChar2(pollData[current_poll].ts.u8.yr);
    outChar2(pollData[current_poll].ts.u8.hour);
    outChar2(pollData[current_poll].ts.u8.min);
    outChar2(pollData[current_poll].ts.u8.sec);
    
    for(x = 0; x < NUM_ADC_PROBES; ++x)
    {
        for(y = 0; y < sizeof(float); ++y)
        {
            outChar2(pollData[current_poll].samples[x].s[y]);
        }    
    }    
    //outString2("10tempData");
    //outString2("8redData");
    //outString2("rV");
    outString2(fdata.dat.node_name);
    //outString2("12_node_test");
    
    u8_polled = 1;
    outString("Poll Response Complete\n\n");
}

/****************************INTERRUPT HANDLERS****************************/
void _ISRFAST _INT1Interrupt (void) {
    _INT1IF = 0;		//clear interrupt flag before exiting
}
    

/****************************MAIN******************************************/
int main(void)
{
    //SNSL_ConfigLowPower();
    configClock();
    configDefaultUART(DEFAULT_BAUDRATE); //this is UART2
    configUART2(DEFAULT_BAUDRATE);
    
    CONFIG_SLEEP_INPUT();
    CONFIG_TEST_SWITCH();
    CONFIG_ANALOG_POWER();
    CONFIG_ANALOG_INPUTS();

    CONFIG_INT1_TO_RP(9);   //connect interrupt to sleep pin
    
    __builtin_write_OSCCONL(OSCCON | 0x02);
    RTCC_SetDefaultVals(&u_RTCC);
    RTCC_Set(&u_RTCC);
    
    UFDATA fdata;
    SNSL_GetNodeName(&fdata);
    if (fdata.dat.node_name[0] == 0xFF) {
        char defaultName[] = "default";
        SNSL_SetNodeName(defaultName);
    }    
    
    current_poll = 0x00;
    
    //outString("Hello World\n");
    //printResetCause();

    while (1) {
        if (!TEST_SWITCH)
        {
            _INT1IE = 0;		//make sure the interrupt is disabled
            uint8 u8_menuIn;
    
            outString("\n\n\nSetup Mode:\n\n");
            outString("Choose an option -\n\n");
            outString("1. Configure Clock\n");
            outString("2. Set Node Name\n");
            outString("3. Exit Setup Mode\n");
            outString("--> ");
            u8_menuIn = inChar();
            DELAY_MS(100);
    
            if (u8_menuIn == '1')
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
    
            if (u8_menuIn == '2')
            {
                outString("\n\nSetting node name:\n");
                outString("\nCurrent node name: ");
                SNSL_PrintNodeName();
                outString("\n\nEnter new node name [maximum 12 characters]: ");
                char name_buff[13];
                inStringEcho(name_buff, 12);
                SNSL_SetNodeName(name_buff);
                outString("\nNew node name saved. Returning to menu....\n");
                //outString(name_buff);
                WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            }
            
            else if(u8_menuIn == '3') {
                outString("\n\nExiting setup......\n");
                outString("\nPlease move SETUP swtich from 'MENU' to 'NORM' to exit.");
                while (!TEST_SWITCH) {}
                outString("\n\nReturning to normal mode....\n\n");
            }
        }
        else
        {
            DELAY_MS(500);      // Wait for SLEEP_INPUT to stabilize before attaching interrupt
    
            _INT1IF = 0;		//clear interrupt flag
            _INT1IP = 2;		//set interrupt priority
            _INT1EP = 0;		//set rising edge (positive) trigger
            _INT1IE = 1;		//enable the interrupt
    
            WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            //now sleep until next MCLR
            WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();
            //disable UART2 to save power.
            U2MODEbits.UARTEN = 0;                    // disable UART RX/TX
            U2STAbits.UTXEN = 0;                      //disable the transmitter
            
            while (1) {
                SLEEP();         //macro for asm("pwrsav #0")
                NOP();          //insert nop after wake from sleep to solve stabiity issues
                
                //do code from interrupt here:
                configHighPower();
                
                U2MODEbits.UARTEN = 1;                    // enable UART RX/TX
                U2STAbits.UTXEN = 1;                      //enable the transmitter
            
                if (SLEEP_INPUT)
                {
                    u8_polled = 0;
                    _DOZE = 8; //chose divide by 32
                    outString("Awake....Reading ADC Data:\n");
                    sampleProbes(poll);    //sample ADC for redox data, store in data buffer
                    outChar('\n');
                    
                    RTCC_Read(&pollData[current_poll].ts);
            
                    int x;
                    for(x = 0; x < NUM_ADC_PROBES; ++x)
                    {
                        pollData[current_poll].samples[x].f = poll[x].f;
                    }
            
                    while (SLEEP_INPUT)
                    {
                        _DOZEN = 1; //enable doze mode, cut back on clock while waiting to be polled
                        //outString("Waiting for char\n");
                        if (isCharReady2())
                        {
                            _DOZEN = 0;
                            outString("Poll Request Received\n");
                            parseInput();  //satisfy the polling
                        }
                    }
                    if (final_old_ack == 1) {
                        poll_count = 1;
                    }    
                }
                if (u8_polled == 0) {
                    //node woke up but wasn't successfully polled | store data and increment current_poll
                    current_poll = ++current_poll % MAX_STORED_SAMPLES;    
                    ++poll_count;
                    if (poll_count > MAX_STORED_SAMPLES) {
                        poll_count = MAX_STORED_SAMPLES;
                    }
                    outString("ERROR: Poll Did Not Complete\n");
                    printf("Total Missed Polls: %d\n", poll_count);
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
