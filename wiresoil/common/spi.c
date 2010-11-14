#include "pic24_all.h"
#include "pic24_ports.h"
#include "vdip.h"
//#include <stdio.h>

/***********************************************************
 * Pin Mappings
 **********************************************************/

#define PORT_SCLK  _LATB1	// AD0(SCLK) -> RB1
#define PORT_SDI   _LATB2	// AD1(SDI)  -> RB2
#define PORT_SDO   _RB3		// AD2(SDO)  -> RB3
#define PORT_CS    _LATB12	// AD3(CS)   -> RB12

#define CONFIG_SCLK() CONFIG_RB1_AS_DIG_OUTPUT()
#define CONFIG_SDI()  CONFIG_RB2_AS_DIG_OUTPUT()
#define CONFIG_SDO()  CONFIG_RB3_AS_DIG_INPUT()
#define CONFIG_CS()   CONFIG_RB12_AS_DIG_OUTPUT()


/***********************************************************
 * Common Characters
 **********************************************************/

#define DIR_SPIWRITE 0
#define DIR_SPIREAD  1


//**********************************************************
/**
 * @brief Uses a nop to create a very short delay.
 */
//**********************************************************
#define spiDelay() \
 asm("nop");\
 asm("nop");


//**********************************************************
/**
 * @brief Transfer data to/from the VDIP
 * @param[in] int The direction of the transfer
 * @param[out] uint8* The data to send or a buffer to fill
 */
//**********************************************************
int SPI_Xfer(int spiDirection, uint8 *pSpiData)
{
	uint8 retData,
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
/**
 * @brief Initialize the pins for the SPI interface
 */
//**********************************************************
void SPI_Init(void)
{

    CONFIG_SCLK();
    CONFIG_SDI();
    CONFIG_SDO();
    CONFIG_CS();


	// Configure initial pin states
	PORT_SDI = 0;
	PORT_SCLK = 0;
	PORT_CS = 0;
}


//**********************************************************
/**
 * @brief Wait till one character is read and then return it
 * @return uint8 The received character
 */
//**********************************************************
uint8 SPI_ReadWait(void)
{
	uint8 spiData;

	while (SPI_Xfer(DIR_SPIREAD, &spiData))
    {
    	//outChar(spiData);
    }

    // If the new line isn't added, then the
    // uint8acters just overwrite each other, and
    // since a space is the last uint8acter before
    // the EOS, none of the output shows up.
    /*if(spiData == 0x0d)
    {
        spiData = '\n';
    } TED == IDIOT*/

    // printf("`%c`:`%x`\n", spiData, spiData);
    return spiData;
}


//**********************************************************
/**
 * @brief Non-blocking read of one uint8acter from SPI bus
 * @return uint8 The received uint8acter
 */
//**********************************************************
uint8 SPI_Read(uint8 *pSpiData)
{
	return SPI_Xfer(DIR_SPIREAD, pSpiData);
}

//**********************************************************
/**
 * @brief Blocking write of uint8acter to SPI bus
 * @param[in] uint8 The byte to be transmitted
 */
//**********************************************************
void SPI_Write(uint8 spiData)
{
	while(SPI_Xfer(DIR_SPIWRITE, &spiData));
}


//**********************************************************
/**
 * @brief Send a string of uint8acters to the SPI bus
 * @param[in] const uint8* The string to send
 */
//**********************************************************
void SPI_WriteStr(const uint8 *spiData)
{
    while(*spiData)
    {
        SPI_Write(*(spiData++));
    }

	// Carriage Return - every command needs one.
	SPI_Write(0x0d);
}


//**********************************************************
/**
 * @brief Send a string of uint8acters to the SPI bus
 * @param[in] const uint8* The string to send
 */
//**********************************************************
void SPI_WriteStrN(const uint8 *spiData, uint32 u32_size)
{
    uint32 u32_i;
    for(u32_i = 0; u32_i < u32_size; ++u32_i)
    {
        SPI_Write(spiData[u32_i]);
    }

	// Carriage Return - every command needs one.
	SPI_Write(0x0d);
}
