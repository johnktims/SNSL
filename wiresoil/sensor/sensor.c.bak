#include "pic24_all.h"
#include "packet.h"

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
	outString1(sz_1);
	inStringEcho1(sz_buff, 7);
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
  
   	u8_c = inChar();
   	outChar1(u8_c);
   	if (u8_c != MONITOR_REQUEST_DATA_STATUS) return;  

	while (!RCFGCALbits.RTCSYNC) {
	}
	readRTCC();

	SendPacketHeader();
	outChar(39);
	outChar(APP_SMALL_DATA);
	//outString("MDYHMS");
	outChar(u_RTCC.u8.month);
	outChar(u_RTCC.u8.date);
	outChar(u_RTCC.u8.yr);
	outChar(u_RTCC.u8.hour);
	outChar(u_RTCC.u8.min);
	outChar(u_RTCC.u8.sec);
	outString("10tempData");
	outString("8redData");
	outString("rV");
	outString("12_node_test");
}

#define SLEEP_INPUT _RB14
#define TEST_SWITCH _RB8

/// Sleep Input pin configuration
inline void CONFIG_SLEEP_INPUT()  {
	CONFIG_RB14_AS_DIG_INPUT();     //use RB14 for mode input
  	DISABLE_RB14_PULLUP();
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
	configUART1(DEFAULT_BAUDRATE);

	if (_POR) {
		_POR = 0;    //clear POR bit, init any persistent variables
	}

	CONFIG_SLEEP_INPUT();
	CONFIG_TEST_SWITCH();

	__builtin_write_OSCCONL(OSCCON | 0x02);

	if (!TEST_SWITCH) {
		uint8 u8_menuIn;

		/*outString1("In test mode\n");
		WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();*/

		outString1("Setup Mode:\n\n");
		outString1("Choose an option -\n\n");
		outString1("1. Configure Clock\n");
		outString1("2. Set Node Name\n");
		outString1("--> ");
		u8_menuIn = inCharEcho1();
		DELAY_MS(100);
		
		if (u8_menuIn == '1') {
			outChar1('\n');
			getDateFromUser();
			//setRTCCVals();
			outString1("\n\nInitializing clock....");
			setRTCC();
			outString1("\nTesting clock (press any key to end):\n");
			while (1) {
				if (RCFGCALbits.RTCSYNC) {
					readRTCC();
					char buff[8];
					sprintf(buff, "%2x\n", u_RTCC.u8.sec);
					outString1(buff);
					DELAY_MS(700);
					/*uint16 tmp = (uint16) u_RTCC.u8.sec;
					outUint161(tmp);*/
				}
			}
		}
		
		while (1) {
			SLEEP();
		}
	}
	else {
		setRTCCVals();
		setRTCC();
	
	 	if (SLEEP_INPUT) {
	 		_DOZE = 8; //chose divide by 32 
	   		while (SLEEP_INPUT) {
	    		_DOZEN = 1; //enable doze mode, cut back on clock while waiting to be polled
	    		if (isCharReady()) {
	       			_DOZEN = 0;
					outChar1('*');
	       			parseInput();  //satisfy the polling
	    		} 
	   		}
	 	}

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
