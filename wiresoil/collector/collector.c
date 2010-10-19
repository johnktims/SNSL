#include "pic24_all.h"
#include "packet.h"
#include "snsl.h"

uint8 u8_pollingTimeout;
uint8 u8_failureLimit;

uint8 blocking_inChar()
{
    // Wait for character
    while(!isCharReady2()){}
    return inChar2();
}

uint8 isMeshUp(void) {
	uint8 u8_c = inChar2();
	if (u8_c == 0x04) {
		u8_c = inChar2();
		if (u8_c == 0x1) {
			u8_c = inChar2();
			u8_c = inChar2();
			u8_c = inChar2();
			return 0x01;
		}
	}
	return 0x0;
}

//format: 0x1E + length + groupID LSB + grpID MSB + TTL + 0x02 + 'sleepyNodeFalse'
void sendStayAwake(void) {
	const char sz_data[] = "sleepyModeFalse";

	SendPacketHeader(); //0x1E
	outChar2(0x10);	//packet length = 1 + string length
	outChar2(0x01);	//LSB of group ID is 0001
	outChar2(0x00);  //MSB of group ID
	outChar2(0x05);	//TTL
	outChar2(0x02);  //MRPC packet type
	outString2(sz_data);
}

//packet format: 0x1E + packet length + address + 0x03 + message
uint8 doPoll(char c_ad1, char c_ad2, char c_ad3) {

	outChar('P');
	outChar(':');
	outChar(c_ad1);
	outChar(',');
	outChar(c_ad2);
	outChar(',');
	outChar(c_ad3);
	outChar('\n');

	SendPacketHeader();
	outChar2(0x02);		//packet length
	outChar2(c_ad1);
	outChar2(c_ad2);
	outChar2(c_ad3);
	outChar2(0x03);	//Appdata packet (forward data directly to PIC)
	outChar2(MONITOR_REQUEST_DATA_STATUS);

	WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();

	uint8 poll_data[40];
	uint8 i = 0;
	for (i = 0; i<40; i++) {
		poll_data[i] = 0x00;
	}
	//poll_data[38] = '\n';
	//poll_data[39] = '\0';

	uint8 tmp = 0;
	uint8 packet_length = 0;
	uint8 remaining_polls = 0;
	for(tmp = 0; tmp < 7; tmp++)
	{
		if (tmp == 4) packet_length = blocking_inChar() - 2;
		else if (tmp == 6) remaining_polls = blocking_inChar();
		else blocking_inChar();
	}

	for(tmp = 0; tmp < packet_length; tmp++) {
		poll_data[tmp] = blocking_inChar();
	}
	
	poll_data[packet_length] = '\0';
	
	outString("PACKET:`");
	tmp = 0;
	for(tmp = 0; tmp < 39; tmp++)
	{
		outUint8(poll_data[tmp]);
		outChar(' ');
	}
	outString("`\n");
	for(tmp = 0; tmp < 39; tmp++)
	{
		outChar(poll_data[tmp]);
	}

	//uint8 *pui_node_name = &poll_data[26];
	uint8 psz_node_name[MAX_NODE_NAME_LEN];
	for(i = 26; i < packet_length; ++i)
	{
		psz_node_name[i-26] = poll_data[i];
	}
	psz_node_name[i-26] = 0x0;
	

    uint8 psz_fmt[] = "%02X%02X%02X,"                   // node address
                      /*"%c%c%c%c%c%c%c%c%c%c%c%c,"   	// node name*/
					  "%s,"
                      "%02x/%02x/%02x %02x:%02x:%02x,"  // timestamp [MM/DD/YY HH:MM:SS]
                      "%c%c%c%c%c%c%c%c%c%c,"           // temp samples
                      "%c%c%c%c%c%c%c%c,"               // redox samples
                      "%c%c\n";                         // ref voltage

    uint8* p = poll_data;
    uint8 psz_out[70];
    sprintf(psz_out, psz_fmt,
        c_ad1, c_ad2, c_ad3,
        /*p[26],p[27],p[28],p[29],p[30],p[31],p[32],p[33],p[34],p[35],p[36],p[37],*/
		psz_node_name,
        p[0],p[1],p[2],p[3],p[4],p[5],
        p[6],p[7],p[8],p[9],p[10],p[11],p[12],p[13],p[14],p[15],
        p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
        p[24],p[25]);

	psz_out[69] = 0x0;

	outString(psz_out);

	VDIP_WriteFile("DATA.TXT", psz_out);

	outChar('\n');

	if (remaining_polls != 0) {
		doPoll(c_ad1, c_ad2, c_ad3);
	}

	return 0x01;
}

void sendEndPoll(void) {
	const char sz_data[] = "pollingStopped";

	SendPacketHeader(); //0x1E
	outChar2(0x0F);	//packet length
	outChar2(0x01);
	outChar2(0x00);
	outChar2(0x05);	//TTL
	outChar2(0x02);	//MRPC packet type
	outString2(sz_data);
}

#define SLEEP_INPUT _RB14
#define TEST_SWITCH _RB8
#define VDIP_POWER	_RB7

/// Sleep Input pin configuration
inline void CONFIG_SLEEP_INPUT()  {
  CONFIG_RB14_AS_DIG_INPUT();     //use RB14 for mode input
  DISABLE_RB14_PULLUP();
  DELAY_US(1);
}

//Test Mode Switch pin configuration
inline void CONFIG_TEST_SWITCH() {
	CONFIG_RB8_AS_DIG_INPUT();
	ENABLE_RB8_PULLUP();
	DELAY_US(1);
}

//VDIP power pin configuration
inline void CONFIG_VDIP_POWER() {
	CONFIG_RB7_AS_DIG_OD_OUTPUT();
	DELAY_US(1);
}

void _ISRFAST _INT1Interrupt(void) {
	uint32 u32_index = 0;

	_LATB7 = 0;		//enable power to VDIP

	while (SLEEP_INPUT) {
		if (isCharReady2()) {
			if (isMeshUp() == 0x01) {
				sendStayAwake();
				WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();


				VDIP_Init();
				char **data = SNSL_ParseNodeNames();

				while (data[u32_index] != '\0') {
					doPoll(data[u32_index][0],
							data[u32_index][1],
							data[u32_index][2]);
					++u32_index;
				}
				sendEndPoll();

				//doPoll('\x00', '\x32', '\x64');

				VDIP_CleanupDirList(data);
				VDIP_Reset();
			}
		}
	}
	_LATB7 = 1;					//cut power to VDIP

	_INT1IF = 0;
}	

int main(void) {
	configClock();
	configDefaultUART(DEFAULT_BAUDRATE);
	configUART2(DEFAULT_BAUDRATE);

	CONFIG_SLEEP_INPUT();
	CONFIG_TEST_SWITCH();
	CONFIG_VDIP_POWER();

	CONFIG_INT1_TO_RP(14);

	u8_pollingTimeout = 5;
	u8_failureLimit = 5;

	if (!TEST_SWITCH) {
		uint8 u8_menuIn;

		outString("\n\n\n");
		outString("Setup Mode:\n\n");
		outString("Choose an option -\n\n");
		outString("1. Configure clock\n");
		outString("2. Set polling timeout\n");
		outString("3. Set node failure limit\n");
		outString("4. Reset node ignore list\n");
		outString("--> ");
		u8_menuIn = inChar();
		DELAY_MS(100);

		if (u8_menuIn == '1') {
		}
		else if (u8_menuIn == '2') {
			outString("\nThis value is the ammount of time to wait during polling before skipping unresponsive node.\n");
			outString("\nCurrent timeout (seconds): ");
			outUint8Decimal(u8_pollingTimeout);
			outString("\nNew timeout (seconds, max 15): ");
			char timeout_buff[3];
			inStringEcho(timeout_buff, 2);
			outString("\nNew timeout value saved.");
			WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
		}
		else if (u8_menuIn == '3') {
			outString("\nThis value is the number of failures to respond a sensor node has before it will be ignored.\n");
			outString("\nCurrent failure limit: ");
			outUint8Decimal(u8_failureLimit);
			outString("\nNew failure limit: ");
			char failure_buff[3];
			inStringEcho(failure_buff, 2);
			outString("\nNew failure limit saved.");
			WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
		}
		else if (u8_menuIn == '4') {
			outString("Resetting node ignore list........");
			outString("\nNode ignore list reset completed.");
			WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
		}
	}
	else {
		DELAY_MS(500);		//wait for SLEEP_INPUT to stabilize before attaching interrupt
	
		_INT1IF = 0;		//clear interrupt flag
		_INT1IP = 2;		//set interrupt priority
		_INT1EP = 0;		//set rising edge (positive) trigger 
		_INT1IE = 1;		//enable the interrupt

		_LATB7 = 1;					//cut power to VDIP*/
		_LATB7 = 0;

		while (1) {
			SLEEP();
		}
	}
	return 0;
}
