#include "pic24_all.h"
#include <string.h>
#include <stdio.h>
#include "snsl.h"
#include "packet.h"

#define SLEEP_INPUT _RB14

/// Switch1 configuration
inline void CONFIG_SLEEP_INPUT()  {
  CONFIG_RB14_AS_DIG_INPUT();     //use RB14 for mode input
  DISABLE_RB14_PULLUP();
  DELAY_US(1);
}

uint8 isMeshUp(void) {
	uint8 u8_c;

	u8_c = inChar();
	if (u8_c == 0x04) {
		while (!isCharReady()) {
		}
		u8_c = inChar();
		if (u8_c == 0x01) {
			u8_c = inChar();
			u8_c = inChar();
			u8_c = inChar();
			return 0x01;
		}
		return 0x00;
	}
	else {
		return 0x00;
	}
}

//format: 0x1E + length + groupID LSB + grpID MSB + TTL + 0x02 + 'sleepyNodeFalse'
void sendStayAwake(void) {
	const char sz_data[] = "sleepyModeFalse";

	SendPacketHeader(); //0x1E
	outChar(0x10);	//packet length = 1 + string length
	outChar(0x01);	//LSB of group ID is 0001
	outChar(0x00);  //MSB of group ID
	outChar(0x05);	//TTL
	outChar(0x02);  //MRPC packet type
	outString(sz_data);
}

//packet format: 0x1E + packet length + address + 0x03 + message
uint8 doPoll(char c_ad1, char c_ad2, char c_ad3) {

	outChar1('P');
	outChar1(':');
	outChar1(c_ad1);
	outChar1(',');
	outChar1(c_ad2);
	outChar1(',');
	outChar1(c_ad3);
	outChar1('\n');

	SendPacketHeader();
	outChar(0x02);		//packet length
	outChar(c_ad1);
	outChar(c_ad2);
	outChar(c_ad3);
	outChar(0x03);	//Appdata packet (forward data directly to PIC)
	outChar(MONITOR_REQUEST_DATA_STATUS);

    uint8 u8_c;
	uint8 u8_i = 0;
    // 44 characters
    uint8 poll_data[] = "2w'12_node_testMDYHMS10tempData8redDatarVN";

    outString1("Test Print:'");
    outString1(poll_data);
    outString1("'(End Test Print\n");
	while (!isCharReady()){}
	outChar1('G');
	outChar1(':');
	//while (u8_i < strlen(poll_data)+5) { // 19 - 24
    while (u8_i < 44) { // 19 - 24
		while (!isCharReady()){}

		// Get rid of the leading special characters
		if (u8_i < 4)
		{
    		++u8_i;
    		inChar();
    		continue;
        }

		u8_c = inChar();
		poll_data[u8_i] = u8_c;
		outChar1(u8_c);

		WAIT_UNTIL_TRANSMIT_COMPLETE_UART1();
		++u8_i;
	}
	poll_data[u8_i] = '\n';
	outString1("\nPACKET:`");
	outString1(poll_data);
	outString1("`\n");

    VDIP_WriteFile("DATA.TXT", poll_data);
	outChar1('\n');
	return 0x01;
}

void sendEndPoll(void) {
	const char sz_data[] = "pollingStopped";

	SendPacketHeader(); //0x1E
	outChar(0x0F);	//packet length
	outChar(0x01);
	outChar(0x00);
	outChar(0x05);	//TTL
	outChar(0x02);	//MRPC packet type
	outString(sz_data);
}


int main(void) {
	configClock();
	//configPinsForLowPower();
	configHeartbeat();
	configDefaultUART(DEFAULT_BAUDRATE); //uart2 >> SNAP Node
	configUART1(DEFAULT_BAUDRATE); //uart1 >> debug output
	
	CONFIG_SLEEP_INPUT();

	outChar1('\n');

	//char data[2][3] = {{0x00, 0x32, 0x64},
	//			       {0x00, 0x32, 0x77}};

	//uint8 u8_returnVal = 0x00;
    uint32 u32_index = 0;
    //uint8 u8_index = 0;

	if (SLEEP_INPUT) {
		_DOZE = 8; //choose divide by 32
		while (SLEEP_INPUT) {
			_DOZEN = 1; //reduce clock speed while waiting for mesh to come up
				_DOZEN = 0;
			if (isCharReady()) {
				if (isMeshUp() == 0x01) {
					sendStayAwake();
					WAIT_UNTIL_TRANSMIT_COMPLETE_UART2();


					//DELAY_MS(20000);
					//doPoll('\x00', '\x32', '\x64');
					VDIP_Init();
					char **data = SNSL_ParseNodeNames();
					// Poll each of the nodes
                    while(data[u32_index] != '\0')
                    {
                        doPoll(data[u32_index][0],
                               data[u32_index][1],
                               data[u32_index][2]);
                        ++u32_index;
                    }
					sendEndPoll();

                    VDIP_CleanupDirList(data);
					VDIP_Reset();
				}
			}
		}
	}

	U2MODEbits.UARTEN = 0;
	U2STAbits.UTXEN = 0;
	while (1) {
		SLEEP();
	}

	return 0;
}
