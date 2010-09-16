#include "pic24_all.h"
#include "pic24_ports.h"
#include "vdip.h"
#include <stdio.h>

//**********************************************************
//
//  Pin definitions for SPI
//
//**********************************************************

#define PORT_SDI   _LATB2	// SDI (on VDIP1) is RB6
#define PORT_SCLK  _LATB1	// SCLK is RB5
#define PORT_SDO   _RB3		// SDO (on VDIP1) is RB7
#define PORT_CS    _LATB6	// CS is RB2

//**********************************************************
//
//  Constants and variables for SPI
//
//**********************************************************

#define DIR_SPIWRITE 0
#define DIR_SPIREAD  1

//**********************************************************
// Name: spiDelay
//
// Description: Short delay.
//
// Parameters: None.
//
// Returns: None.
//
// Comments: Uses a nop to create a very short delay.
//
//**********************************************************
//#define spiDelay

#define spiDelay() \
 asm("nop");\
 asm("nop");


int SPI_Xfer(int spiDirection, char *pSpiData)
{
	unsigned char retData,
				  bitData;

	// Clock 1 - Start State
	PORT_SDI = 1;
	PORT_CS = 1;

	spiDelay();
	PORT_SCLK = 1;
	spiDelay();
	PORT_SCLK = 0;

	// Clock 2 - Direction
	PORT_SDI = spiDirection;

	spiDelay();
	PORT_SCLK = 1;
	spiDelay();
	PORT_SCLK = 0;

	// Clock 3 - Address
	PORT_SDI = 0;

	spiDelay();
    PORT_SCLK = 1;
	spiDelay();
	PORT_SCLK = 0;

	// Clocks 4..11 - Data Phase
	bitData = 0x80;
	switch(spiDirection)
	{
		// read operation
		case DIR_SPIREAD:
			retData = 0;
			spiDelay();
			retData |= PORT_SDO?0x80:0;
			PORT_SCLK = 1;
			spiDelay();
			PORT_SCLK = 0;

			spiDelay();
			retData |= PORT_SDO?0x40:0;
			PORT_SCLK = 1;
			spiDelay();
			PORT_SCLK = 0;

			spiDelay();
			retData |= PORT_SDO?0x20:0;
			PORT_SCLK = 1;
			spiDelay();
			PORT_SCLK = 0;

			spiDelay();
			retData |= PORT_SDO?0x10:0;
			PORT_SCLK = 1;
			spiDelay();
			PORT_SCLK = 0;

			spiDelay();
			retData |= PORT_SDO?0x08:0;
			PORT_SCLK = 1;
			spiDelay();
			PORT_SCLK = 0;

			spiDelay();
			retData |= PORT_SDO?0x04:0;
			PORT_SCLK = 1;
			spiDelay();
			PORT_SCLK = 0;

			spiDelay();
			retData |= PORT_SDO?0x02:0;
			PORT_SCLK = 1;
			spiDelay();
			PORT_SCLK = 0;

			spiDelay();
			retData |= PORT_SDO?0x01:0;
			PORT_SCLK = 1;
			spiDelay();
			PORT_SCLK = 0;

			*pSpiData = retData;
			break;

		// write operation
		case DIR_SPIWRITE:
			retData = *pSpiData;

			while (bitData)
			{
				PORT_SDI = (retData & bitData)?1:0;
				spiDelay();
				PORT_SCLK = 1;
				spiDelay();
				PORT_SCLK = 0;
				bitData >>= 1;
			}
			break;
	}

	spiDelay();
	bitData = PORT_SDO;			//0 = new data read/data recieved
								//1 = old data read/data not received
	if(bitData == 1)
	{
		DELAY_US(10);
	}

	PORT_SCLK = 1;
	spiDelay();
	PORT_SCLK = 0;

	// CS goes low to disable SPI communications
	PORT_CS = 0;
	spiDelay();

	// Clock 13 - CS low
	spiDelay();
	PORT_SCLK = 1;
	spiDelay();
	PORT_SCLK = 0;

    return bitData;
}

//**********************************************************
//
// External Routines
//
//**********************************************************

//**********************************************************
// Name: spiInit
//
// Description: Initialise the SPI interface.
//
// Parameters: None.
//
// Returns: None.
//
// Comments: Sets up pins connecting to the SPI interface.
//
//**********************************************************
void SPI_Init(void)
{

//	TRIS_SDO = 1;			        // SDO input
//	TRIS_SDI = 0;					// SDI output
//	TRIS_SCLK = 0;				// SCLK output
//	TRIS_CS = 0;				// CS output

// Configure pin direction (1 for input, 0 for output)
	CONFIG_RB1_AS_DIG_OUTPUT();
	CONFIG_RB2_AS_DIG_OUTPUT();
	CONFIG_RB3_AS_DIG_INPUT();
	CONFIG_RB6_AS_DIG_OUTPUT();

	// Configure initial pin states
	// SDO starts low
	PORT_SDI = 0;
	// SCLK starts low
	PORT_SCLK = 0;
	// CS starts low
	PORT_CS = 0;
}
//**********************************************************
// Name: spiReadWait
//
// Description: Blocking read of character from SPI bus.
//
// Parameters: None.
//
// Returns: spiData - Byte received.
//
// Comments: Waits until a character is read from the SPI
//           bus and returns.
//
//**********************************************************
char SPI_ReadWait(void)
{
	char spiData;
	
	while (SPI_Xfer(DIR_SPIREAD, &spiData)){
		printf("%c",spiData);
	}

    // If the new line isn't added, then the
    // characters just overwrite each other, and
    // since a space is the last character before
    // the EOS, none of the output shows up.
    if(spiData == 0x0d)
    {
        //putchar('\n');
        spiData = '\n';
    }

    return spiData;
}
//**********************************************************
// Name: spiRead
//
// Description: Non-blocking read of character from SPI bus.
//
// Parameters: None.
//
// Returns: pSpiData - Byte received.
//          int XFER_OK if data received, XFER_RETRY if not.
//
// Comments: Check for a character on the SPI bus and return.
//
//**********************************************************
int SPI_Read(char *pSpiData)
{
	return SPI_Xfer(DIR_SPIREAD, pSpiData);
}

//**********************************************************
// Name: spiWrite
//
// Description: Blocking write of character to SPI bus.
//
// Parameters: spiData - Byte to be transmitted.
//
// Returns: None.
//
// Comments: Waits until a character is transmitted on the SPI bus.
//
//**********************************************************
void SPI_Write(char spiData)
{
	while(SPI_Xfer(DIR_SPIWRITE, &spiData));
}


void SPI_WriteStr(const char *spiData)
{
    while(*spiData)
    {
        SPI_Write(*(spiData++));
    }    
    
	// Carriage Return - every command needs one.
	SPI_Write(0x0d);
}
