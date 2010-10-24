"""
Script for remote data node of sleepy mesh. Uses functions from sleepyTemplate.py
"""


# Use Synapse Evaluation Board definitions

from synapse.evalBase import *
from synapse.switchboard import *
from sleepyTemplate import *



MCLR_PIN = GPIO_11
REQ_PIN = GPIO_10
SLEEP_PIN = GPIO_9
MODE_PIN = GPIO_12
CAN_LEAD_PIN = GPIO_13

STATE_NULL = 0
STATE_PACKET = 1
STATE_STRING = 2
MAX_STRING = 32

stdioState = STATE_NULL

homeAddr = ''
lastRecCnt = 0
forwardStr = ''


def atStartup():

    global stdioState  
    global lastRecCnt
    global recCnt
    global forwardStr
    global canLead
        
    
    
    #configure unused pins for low power
    setPinDir(GPIO_0,False)   #input
    setPinPullup(GPIO_0,True) #enable pullup
    setPinDir(GPIO_1,False)   #input
    setPinPullup(GPIO_1,True) #enable pullup
    setPinDir(GPIO_2,False)   #input
    setPinPullup(GPIO_2,True) #enable pullup
    setPinDir(GPIO_3,False)   #input
    setPinPullup(GPIO_3,True) #enable pullup
    setPinDir(GPIO_5,False)   #input
    setPinPullup(GPIO_5,True) #enable pullup
    setPinDir(GPIO_6,False)   #input
    setPinPullup(GPIO_6,True) #enable pullup
    #setPinDir(GPIO_12,False)   #input
    #setPinPullup(GPIO_12,True) #enable pullup
    setPinDir(GPIO_13,False)   #input
    #setPinPullup(GPIO_13,True) #enable pullup
    setPinDir(GPIO_14,False)   #input
    setPinPullup(GPIO_14,True) #enable pullup
    setPinDir(GPIO_15,False)   #input
    setPinPullup(GPIO_15,True) #enable pullup
    setPinDir(GPIO_16,False)   #input
    setPinPullup(GPIO_16,True) #enable pullup
    setPinDir(GPIO_17,False)   #input
    setPinPullup(GPIO_17,True) #enable pullup 
    setPinDir(GPIO_18,False)   #input
    setPinPullup(GPIO_18,True) #enable pullup 
   
   
    lastRecCnt = 0
    recCnt = 0
    forwardStr = ''
    stdioState = STATE_NULL
    
    
    setPinDir(MCLR_PIN, False)  #input 
    writePin(MCLR_PIN,True)    #set high for when configured as output   
    setPinDir(REQ_PIN,False)   #input tied to PIC24 for general use, not currently used
    setPinPullup(REQ_PIN,True) #pullup
    setPinDir(MODE_PIN, False) #input
        
    setPinDir(SLEEP_PIN, True)  #output
    writePin(SLEEP_PIN,True)   #set high, want to jump to app
     
    initUart(1,19200)
    flowControl(1,False)     # no flow control
    stdinMode(1,False)       #char mode (not line mode), no echo
    crossConnect(DS_UART1,DS_STDIO)
    
    if (readPin(CAN_LEAD_PIN)):
        canLead = True
    else:
        canLead = False
    
  
def beforeSleep(duration):
    """This helper routine is invoked right before going to sleep"""
    writePin(SLEEP_PIN,False)   #tell micro to go to sleep


def afterSleep():
    """This helper routine is invoked right after waking up"""
    #if (readPin(MODE_PIN)):
    writePin(SLEEP_PIN,True)   #tell micro to wakeup
        #doMCLR()                   #sleep pin high causes bootloader firmware to skip timer sequence
    #toggling MCLR assures that we get processor attention, even if it is hung by software error
    #only do MCLR if the processor is not in setup mode


def every100ms():
    """This helper routine is invoked every 100 milliseconds while awake"""
    pass


def everySecond():
    """This helper routine is invoked every second while awake"""
    pass

def showElection():
    """Show that an election is currently in progress
       This helper routine is just for demo purposes"""
    pass


def showLeadership(isLeader):
    """Show if this node is (or is not) the leader
       This helper routine is just for demo purposes"""

    pass

def monitorMeshUp():
    mcastRpc(1,5, 'meshUp', localAddr())


def doMCLR():
    setPinDir(MCLR_PIN, True)  #output
    writePin(MCLR_PIN,True)
    writePin(MCLR_PIN,False)
    writePin(MCLR_PIN,True)
    setPinDir(MCLR_PIN, False)  #input 
 
 
## DO NOT CHANGE THIS FUNCTION NAME, C CODE depends on it.
def doUcReset():
    global homeAddr
    homeAddr = rpcSourceAddr()
    writePin(SLEEP_PIN,False)   #set low to force bootloader firmware through normal sequence
    doMCLR()


def outString(s):
    global homeAddr
    homeAddr = rpcSourceAddr() #must remember who called us!
    print s,
 
    
@setHook(HOOK_STDIN)
def stdinEvent(buf):
    """Receive handler for character input on UART0.
       The parameter 'buf' will contain one or more received characters. 
    """
    global forwardStr
    global recCnt
    global packetLen
    global stdioState
  
    n = len(buf)
    i = 0
    while (i < n):
        if (stdioState == STATE_NULL):
            if (ord(buf[i]) == 0x1E):
                #check if entire packet is in buffer
                if ((i == 0) and (n > 2) and (n-2) == ord(buf[1])):
                    #entire packet here, send it.
                    forwardStr = buf[i]
                    forwardStr += localAddr() # insert our address
                    forwardStr += buf[1:] 
                    rpc(homeAddr,'outString',forwardStr)
                    forwardStr = ''  
                    return
                else:
                    #have to accumulate the packet
                    forwardStr = buf[i]
                    forwardStr += localAddr()
                    recCnt = 1
                    stdioState = STATE_PACKET
        elif (stdioState == STATE_PACKET):
            #accumulating a packet
            forwardStr += buf[i]
            recCnt += 1      
            if (recCnt == 2):
                 packetLen = ord(buf[i])               
            elif (recCnt > 2):
                if ((recCnt-2) == packetLen):
                    recCnt = 0
                    #we are done, forward to node
                    stdioState = STATE_NULL
                    rpc(homeAddr,'outString',forwardStr)
                    forwardStr = ''
                    return
                    
   
        i += 1
 
# hook up event handlers
snappyGen.setHook(SnapConstants.HOOK_STARTUP, startupEvent)
snappyGen.setHook(SnapConstants.HOOK_100MS, timer100msEvent)
snappyGen.setHook(SnapConstants.HOOK_RPC_SENT, rpcsentEvent)
    
            
