#ifndef _VDIP1_SPI_H_
#define _VDIP1_SPI_H_

#define XFER_OK    0
#define XFER_RETRY 1

void SPI_Init(void);

int  SPI_Read(char *);
char SPI_ReadWait(void);
void SPI_Write(char);
void SPI_WriteStr(const char *);

int  SPI_Xfer(int, char*);

#endif
