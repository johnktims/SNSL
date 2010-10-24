"""
Script for monitor node of sleepy mesh. This script assumes the monitor
is awake all of the time and just listens for data.
Uses functions from meshMonitorCommon.py
"""


# Use Synapse Evaluation Board definitions

from synapse.evalBase import *
from synapse.switchboard import *
from meshMonitorCommon import *



@setHook(HOOK_STARTUP)
def startupEvent():

    
    """This is hooked into the HOOK_STARTUP event"""
    initUart(1,19200)
    flowControl(1,False)     # no flow control
    stdinMode(1,False)       #char mode (not line mode), no echo

    crossConnect(DS_STDIO,DS_UART1) 



    
def meshUp(whoReportedIt):
    """The "leader" node of the sleepy mesh calls this upon wakeup"""
    printPacketHeader()
    print "\x04\x01",whoReportedIt,   # 4 bytes in packet, \x01 == 'meshUp' notice followed by node id
               
            
            

snappyGen.setHook(SnapConstants.HOOK_STDIN, stdinEvent)
