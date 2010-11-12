#include "pic24_all.h"
#include "packet.h"
#include "snsl.h"
#include <string.h>

#define SLEEP_INPUT _RB14
#define TEST_SWITCH _RB8

uint8 remaining_polls;

typedef union _unionRTCC {
	struct { //four 16 bit registers
				uint8 yr;
				uint8 null;
				uint8 date;
				uint8 month;
				uint8 hour;
				uint8 wday;
				uint8 sec;
				uint8 min;
	}u8;
	uint16 regs[4];
}unionRTCC;

unionRTCC u_RTCC;

uint8 getBCDvalue(char *sz_1) {
	char sz_buff[8];
	uint16 u16_bin;
	uint8 u8_bcd;
	outString(sz_1);
	inStringEcho(sz_buff, 7);
	sscanf(sz_buff, "%d", (int *)&u16_bin);
	u8_bcd = u16_bin/10;
	u8_bcd = u8_bcd << 4;
	u8_bcd = u8_bcd | (u16_bin % 10);
	outChar1('\n');
	return(u8_bcd);
}

uint8 parseVal(char *sz_1) {
	uint16 u16_bin;
	uint8 u8_bcd;
	sscanf(sz_1, "%d", (int *)&u16_bin);
	u8_bcd = u16_bin/10;
	u8_bcd = u8_bcd << 4;
	u8_bcd = u8_bcd | (u16_bin % 10);
	return(u8_bcd);
}

void setRTCCVals(void) {
	u_RTCC.u8.yr = parseVal("90");
	u_RTCC.u8.month = parseVal("10");
	u_RTCC.u8.date = parseVal("5");
	u_RTCC.u8.wday = parseVal("5");
	u_RTCC.u8.hour = parseVal("10");
	u_RTCC.u8.min = parseVal("10");
	u_RTCC.u8.sec = parseVal("0");
}

void getDateFromUser(void) {
	u_RTCC.u8.yr = getBCDvalue("Enter year (0-99): ");
	u_RTCC.u8.month = getBCDvalue("Enter month (1-12): ");
	u_RTCC.u8.date = getBCDvalue("Enter day of month (1-31): ");
	u_RTCC.u8.wday = getBCDvalue("Enter week day (0-6): ");
	u_RTCC.u8.hour = getBCDvalue("Enter hour (0-23): ");
	u_RTCC.u8.min = getBCDvalue("Enter min (0-59): ");
	u_RTCC.u8.sec = getBCDvalue("Enter sec(0-59): ");
}

void setRTCC(void) {
	uint8 u8_i;
	__builtin_write_RTCWEN();
	RCFGCALbits.RTCEN = 0;
	RCFGCALbits.RTCPTR = 3;
	for (u8_i = 0; u8_i < 4; u8_i++) RTCVAL = u_RTCC.regs[u8_i];
	RCFGCALbits.RTCEN = 1;
	RCFGCALbits.RTCWREN = 0;
}

void readRTCC(void) {
	uint8 u8_i;
	RCFGCALbits.RTCPTR = 3;
	for (u8_i = 0; u8_i < 4; u8_i++) u_RTCC.regs[u8_i] = RTCVAL;
}

void parseInput(void){
	uint8 u8_c;
	UFDATA fdata;
	SNSL_GetNodeName(&fdata);
  
   	u8_c = inChar2();
   	if (u8_c != MONITOR_REQUEST_DATA_STATUS) return;  

	while (!RCFGCALbits.RTCSYNC) {
	}
	readRTCC();
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

	if (remaining_polls != 0x00) {
		remaining_polls--;
	}
}

void _ISRFAST _INT1Interrupt (void) {
 	U2MODEbits.UARTEN = 1;                    // enable UART RX/TX
 	U2STAbits.UTXEN = 1;                      //enable the transmitter

	if (SLEEP_INPUT) {
		_DOZE = 8; //chose divide by 32 
		while (SLEEP_INPUT) {
			_DOZEN = 1; //enable doze mode, cut back on clock while waiting to be polled
			if (isCharReady2()) {
				_DOZEN = 0;
				parseInput();  //satisfy the polling
			} 
		}
	}

	U2MODEbits.UARTEN = 0;                    // disable UART RX/TX
	U2STAbits.UTXEN = 0;                      //disable the transmitter
	_INT1IF = 0;		//clear interrupt flag before exiting
}

/// Sleep Input pin configuration
inline void CONFIG_SLEEP_INPUT()  {
	CONFIG_RA2_AS_DIG_INPUT();     //use RA2 for mode input
  	DISABLE_RA2_PULLUP();
  	DELAY_US(1);                    
}

// Test Mode Switch pin configuration
inline void CONFIG_TEST_SWITCH() {
	CONFIG_RB8_AS_DIG_INPUT();
	ENABLE_RB8_PULLUP();
	DELAY_US(1);
}

int main(void) {
  	configClock();
  	configDefaultUART(DEFAULT_BAUDRATE); //this is UART2
	configUART2(DEFAULT_BAUDRATE);

	if (_POR) {
		_POR = 0;    //clear POR bit, init any persistent variables
	}

	CONFIG_SLEEP_INPUT();
	CONFIG_TEST_SWITCH();

	CONFIG_INT1_TO_RP(14);

	remaining_polls = 0x05;

	__builtin_write_OSCCONL(OSCCON | 0x02);

	setRTCCVals();
	setRTCC();

	if (!TEST_SWITCH) {
		uint8 u8_menuIn;

		outString("Setup Mode:\n\n");
		outString("Choose an option -\n\n");
		outString("1. Configure Clock\n");
		outString("2. Set Node Name\n");
		outString("--> ");
		u8_menuIn = inChar();
		DELAY_MS(100);
		
		if (u8_menuIn == '1') {
			getDateFromUser();
			//setRTCCVals();
			outString("\n\nInitializing clock....");
			setRTCC();
			outString("\nTesting clock (press any key to end):\n");
			while (!isCharReady()) {
				if (RCFGCALbits.RTCSYNC) {
					readRTCC();
					char buff[8];
					sprintf(buff, "%2x\n", u_RTCC.u8.sec);
					outString(buff);
					DELAY_MS(700);
					/*uint16 tmp = (uint16) u_RTCC.u8.sec;
					outUint161(tmp);*/
				}
			}
		}

		if (u8_menuIn == '2') {
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
		
		while (1) {
			SLEEP();
		}
	}
	else {
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
	 	while (1) {
	    	SLEEP();         //macro for asm("pwrsav #0")
	 	} 
	}
 	return 0;
}
