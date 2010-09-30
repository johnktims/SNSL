#include "pic24_all.h"
#include "packet.h"

#define SLEEP_INPUT	_RB14

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

uint8 parseVal(char *sz_1) {
	uint16 u16_bin;
	uint8 u8_bcd;
	sscanf(sz_1, "%d", (int *)&u16_bin);
	u8_bcd = u16_bin/10;
	u8_bcd = u8_bcd << 4;
	u8_bcd = u8_bcd | (u16_bin % 10);
	return(u8_bcd);
}

void getDateFromUser(void) {
	/*u_RTCC.u8.yr = getBCDvalue("Enter year (0-99): ");
	u_RTCC.u8.month = getBCDvalue("Enter month (1-12): ");
	u_RTCC.u8.date = getBCDvalue("Enter day of month (1-31): ");
	u_RTCC.u8.wday = getBCDvalue("Enter week day (0-6): ");
	u_RTCC.u8.hour = getBCDvalue("Enter hour (0-23): ");
	u_RTCC.u8.min = getBCDvalue("Enter min (0-59): ");
	u_RTCC.u8.sec = getBCDvalue("Enter sec(0-59): ");*/

	u_RTCC.u8.yr = parseVal("90");
	u_RTCC.u8.month = parseVal("10");
	u_RTCC.u8.date = parseVal("5");
	u_RTCC.u8.wday = parseVal("5");
	u_RTCC.u8.hour = parseVal("10");
	u_RTCC.u8.min = parseVal("10");
	u_RTCC.u8.sec = parseVal("0");
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

void parseInput(void) {
	uint8 u8_c;

	u8_c = inChar();
	outChar1(u8_c);
	if (u8_c != MONITOR_REQUEST_DATA_STATUS) return;
	
	//while (!RCFGCALbits.RTCSYNC) {
	//}
	readRTCC();

	SendPacketHeader();
	outChar(39);
	outChar(APP_SMALL_DATA);
	outString("12_node_test");
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
}

inline void CONFIG_SLEEP_INPUT()  {
  CONFIG_RB14_AS_DIG_INPUT();     //use RB14 for mode input
  DISABLE_RB14_PULLUP();
  DELAY_US(1);                    
}

int main(void) {
	configClock();
	configDefaultUART(DEFAULT_BAUDRATE);

	CONFIG_SLEEP_INPUT();

	__builtin_write_OSCCONL(OSCCON | 0x02);
	getDateFromUser();
	setRTCC();

	if (SLEEP_INPUT) {
		_DOZE = 8;	//choose divide by 32
		while (SLEEP_INPUT) {
			_DOZEN = 1;	//enable doze mode while waiting on poll
			if (isCharReady()) {
				_DOZEN = 0;	//ramp clock back up to normal
				parseInput();	//satisfy the polling request
			}
		}
	}

	WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();
	U2MODEbits.UARTEN = 0;
	U2STAbits.UTXEN = 0;
	while (1) {
		SLEEP();
	}
	
	return 0;
}
