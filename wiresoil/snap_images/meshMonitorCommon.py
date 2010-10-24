"""
functions common to both meshMonitor.py and meshMonitorLeader.py

"""


import inspect

from synapse.evalBase import *
from synapse.switchboard import *

packetLen = 0;
recCnt = 0;
groupId = 0;
ttl = 0;

XNODE_RPC_PACKET = 0x01
XNODE_MRPC_PACKET = 0x02
XNODE_APPDATA_PACKET = 0x03


def printPacketHeader():
    print "\x1E",localAddr(),
    
def outString(s):
    print s,
    
def monitorMeshUp():
    mcastRpc(1,5, 'meshUp', localAddr())
    #printPacketHeader()
    print "\x1E\x04\x01",localAddr(),   # 4 bytes in packet, \x01 == 'meshUp' notice followed by node id
    

packetType = 0;
       
def stdinEvent(buf):
    """Receive handler for character input on UART0.
       The parameter 'buf' will contain one or more received characters. 
    """
    global forwardStr
    global remoteAddr
    global goTransparentFlag
    global recCnt
    global packetLen
    global groupId
    global ttl
    global packetType


    n = len(buf)
    i = 0
    while (i < n):
        #packetType = 0x02
        if (ord(buf[i]) == 0x1E and (recCnt == 0)):
            recCnt = 1
            remoteAddr = ''
            forwardStr = ''
        elif (recCnt == 1):
            packetLen = ord(buf[i])
            recCnt = recCnt + 1
        elif ((recCnt > 1) and (recCnt < 5)):
            if (recCnt == 2):
                groupId = ord(buf[i])
            if (recCnt == 3):
                groupId = ord(buf[i])*256 + groupId
            if (recCnt == 4):
                ttl = ord(buf[i])
            remoteAddr += buf[i]
            recCnt += 1
        elif (recCnt == 5):
            recCnt += 1
            packetType = ord(buf[i])
#            if (ord(buf[i]) == XNODE_RPC_PACKET):
#                rpcName = buf[i+1:]
#                rpc(remoteAddr,rpcName)   
#                rpcName = ''
#                remoteAddr = ''
#                recCnt = 0;
#            elif (ord(buf[i]) == XNODE_MRPC_PACKET):
#                rpcName = buf[i+1:]
#                mcastRpc(groupId, ttl, rpcName)
#                rpcName = ''
#                recCnt = 0;
        elif (recCnt >= 6):
            forwardStr += buf[i]
            recCnt += 1
            if ((recCnt-5) == packetLen):
                recCnt = 0
                #we are done, forward to node
                if (packetType == XNODE_RPC_PACKET):
                    rpc(remoteAddr,forwardStr)
                elif (packetType == XNODE_MRPC_PACKET):
                    mcastRpc(groupId, ttl, forwardStr)
                    #eval('%s()'%forwardStr)
                    if (forwardStr == 'sleepyModeFalse'):
                        sleepyModeFalse()
                    if (forwardStr == 'pollingStopped'):
                        pollingStopped()
                else:
                    rpc(remoteAddr,'outString',forwardStr)
                    #rpc(remoteAddr, 'outString', "\x33")
                remoteAddr = ''
                forwardStr = ''
                
                 
 
        i += 1
        

