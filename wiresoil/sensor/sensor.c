#include "pic24_all.h"
#include "packet.h"
#include "snsl.h"
#include "rtcc.h"
#include <string.h>

#define SLEEP_INPUT _RB9
#define TEST_SWITCH _RB8

/****************************GLOBAL VARIABLES****************************/
uint8 remaining_polls;

unionRTCC u_RTCC;

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

void configHighPower(void) {
    CONFIG_SLEEP_INPUT();
    CONFIG_TEST_SWITCH();
    
    CONFIG_INT1_TO_RP(9);   //connect interrupt to sleep input
}    

/****************************POLLING FUNCTIONS****************************/
void parseInput(void){
    outString("Polled\n");
    uint8 u8_c;
    UFDATA fdata;
    SNSL_GetNodeName(&fdata);

    u8_c = inChar2();
    outChar(u8_c);
    outChar('\n');
    if (u8_c != MONITOR_REQUEST_DATA_STATUS) return;

    while (!RCFGCALbits.RTCSYNC)
    {
    }
    RTCC_Read(&u_RTCC);
    //packet structure: 0x1E(1)|Packet Length(1)|Packet Type(1)|Remaining polls(1)|Timestamp(6)|Temp data(10)|Redox data(8)|
    //					Reference Voltage(2)|Node Name(12 max)
    SendPacketHeader();
    outChar2(28+strlen(fdata.dat.node_name));
    outChar2(APP_SMALL_DATA);
    outChar2(remaining_polls);	//remaining polls
    //outString("MDYHMS");
    outChar2(u_RTCC.u8.month);
    outChar2(u_RTCC.u8.date);
    outChar2(u_RTCC.u8.yr);
    outChar2(u_RTCC.u8.hour);
    outChar2(u_RTCC.u8.min);
    outChar2(u_RTCC.u8.sec);
    outString2("10tempData");
    outString2("8redData");
    outString2("rV");
    outString2(fdata.dat.node_name);
    //outString2("12_node_test");

    if (remaining_polls != 0x00)
    {
        remaining_polls--;
    }
}

/****************************INTERRUPT HANDLERS****************************/
void _ISRFAST _INT1Interrupt (void) {
    configHighPower();
    
    U2MODEbits.UARTEN = 1;                    // enable UART RX/TX
    U2STAbits.UTXEN = 1;                      //enable the transmitter

    if (SLEEP_INPUT)
    {
        _DOZE = 8; //chose divide by 32
        while (SLEEP_INPUT)
        {
            _DOZEN = 1; //enable doze mode, cut back on clock while waiting to be polled
            outString("Waiting for char\n");
            if (isCharReady2())
            {
                _DOZEN = 0;
                outString("Responding...\n");
                parseInput();  //satisfy the polling
            }
        }
    }

    U2MODEbits.UARTEN = 0;                    // disable UART RX/TX
    U2STAbits.UTXEN = 0;                      //disable the transmitter
    SNSL_ConfigLowPower();
    _INT1IF = 0;		//clear interrupt flag before exiting
}

/****************************MAIN******************************************/
int main(void)
{
    SNSL_ConfigLowPower();
    configClock();
    configDefaultUART(DEFAULT_BAUDRATE); //this is UART2
    configUART2(DEFAULT_BAUDRATE);

    CONFIG_SLEEP_INPUT();
    CONFIG_TEST_SWITCH();

    CONFIG_INT1_TO_RP(9);   //connect interrupt to sleep pin

    __builtin_write_OSCCONL(OSCCON | 0x02);
    RTCC_SetDefaultVals(&u_RTCC);
    RTCC_Set(&u_RTCC);
    
    char defaultName[] = "default";
    SNSL_SetNodeName(defaultName);
    
    remaining_polls = 0x00;
    
    outString("Hello World\n");

    while (1) {
        if (!TEST_SWITCH)
        {
            uint8 u8_menuIn;
    
            outString("Setup Mode:\n\n");
            outString("Choose an option -\n\n");
            outString("1. Configure Clock\n");
            outString("2. Set Node Name\n");
            outString("--> ");
            u8_menuIn = inChar();
            DELAY_MS(100);
    
            if (u8_menuIn == '1')
            {
                RTCC_GetDateFromUser(&u_RTCC);
                outString("\n\nInitializing clock....");
                RTCC_Set(&u_RTCC);
                outString("\nTesting clock (press any key to end):\n");
                while (!isCharReady())
                {
                    if (RCFGCALbits.RTCSYNC)
                    {
                        RTCC_Read(&u_RTCC);
                        RTCC_Print(&u_RTCC);
                        DELAY_MS(700);
                    }
                }
            }
    
            if (u8_menuIn == '2')
            {
                outString("\nCurrent node name: ");
                SNSL_PrintNodeName();
                outString("\n\nEnter new node name [maximum 12 characters]: ");
                char name_buff[13];
                inStringEcho(name_buff, 12);
                outString("\n\nSaving new node name........");
                SNSL_SetNodeName(name_buff);
                outString("\nNew node name: ");
                SNSL_PrintNodeName();
                //outString(name_buff);
                WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
            }
        }
        else
        {
            DELAY_MS(500);
    
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
            while (1)
            {
                SLEEP();         //macro for asm("pwrsav #0")
            }
        }
    }    
    return 0;
}
