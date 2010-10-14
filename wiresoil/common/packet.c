#include "pic24_all.h"
#include "packet.h"

void WriteBuffer(char * ptrData, unsigned int Size) {
  unsigned int DataCount;

  for (DataCount = 0; DataCount < Size; DataCount++) {
    outChar(ptrData[DataCount]);
  }
}

void SendPacketHeader(void) {
 
  outChar2(PACKET_START);
}

void sendPacketChar(char b) {
 SendPacketHeader();
 outChar(2);
 outChar(APP_STRING_DATA);
 outChar(b);
} 
