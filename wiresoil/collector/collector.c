#include "pic24_all.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "snsl.h"
#include "packet.h"

uint8 blocking_inChar()
{
    // Wait for character
    while(!isCharReady()){}
    
    return inChar();
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

    // Throw away the Start of Header
    blocking_inChar();
    
    // Throw away the next three address bytes
    blocking_inChar();
    blocking_inChar();
    blocking_inChar();
    
    // Store the size of the incoming payload
    uint8 size = blocking_inChar();
    
    // Allocate memory for message + newline + null
    uint8 *poll_data = (uint8*)malloc(sizeof(uint8)*(size+2));
    poll_data[size]   = '\n';
    poll_data[size+1] = 0x0;

    u8_i = 0;
    while(u8_i < size)
    {
        poll_data[u8_i++] = blocking_inChar();
    }
    
    /*
    // Store the date strings
    uint8 tm_day  = blocking_inChar(),
          tm_mon  = blocking_inChar(),
          tm_year = blocking_inChar(),
          tm_hour = blocking_inChar(),
          tm_min  = blocking_inChar(),
          tm_sec  = blocking_inChar(),
          test = snprintf(NULL, 0, "%d", tm_min);
          
    // Store the five temperature measurements
    */

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

#define SLEEP_INPUT _RB14
#define TEST_SWITCH _RB8

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

int main(void) {
	configClock();
	//configPinsForLowPower();
	configHeartbeat();
	configDefaultUART(DEFAULT_BAUDRATE); //uart2 >> SNAP Node
	configUART1(DEFAULT_BAUDRATE); //uart1 >> debug output
	
	CONFIG_SLEEP_INPUT();
	CONFIG_TEST_SWITCH();

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
