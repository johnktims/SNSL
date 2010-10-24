"""
Example "sleepy mesh" script. A "sleepy mesh" tries to spend most of it's time
sleeping. (This is done to conserve battery power). Sleeping is synchronized by
use of a leader node. A distributed election process is used to dynamically
determine which node is the leader.

NOTE! - This script must be imported by a customized user script, which provides
the following (required) helper routines:

atStartup() - user specified actions to be taken at startup
beforeSleep(dur) - user specified actions to be taken right before going to sleep
afterSleep() - user specified actions to be taken right after waking back up
every100ms() - user specified actions to be taken every 100 milliseconds
everySecond() - user specified actions to be taken every secondCounter
showElection() - user supplied method of showing that an election is in progress
showLeadership(isLeader) - user supplied method to show leadership

"""


def startupEvent():
    """run automatically after unit startup or download"""
    global secondCounter
    global runFlag, canLead, isLeader
    global sleepTicks, wakeTime, graceTime, electTime
    global wakeTimer, electTimer
    global sleepMode
    global sleepCookie
    global pollStopped
    global monitorNode
    global lowAddr
    

    # Default to being "sleepy", but support an override capability
    # When runFlag is False, stay awake regardless of the timers
    runFlag = True

    canLead = False # Nodes have physical switch to determine leadership eligibility
    
    isLeader = False # The leader will be chosen dynamically

    sleepMode = 1 # high resolution, so the mesh will stay synchronized

    #sleepTicks = 600 # this one represents "ticks", as used by sleep(), 10 minutes
    #sleepTicks = 300 # this one represents "ticks", as used by sleep(), 5 minutes
    #sleepTicks = 60 # for testing....
    sleepTicks = 10

    # The rest of these are in tenths of seconds
    wakeTime = 100      # 10 seconds
    graceTime = 10
    electTime = 20

    wakeTimer = 0 # this one counts up
    electTimer = 0 # this one counts down

    secondCounter = 0
    sleepCookie = 0
    pollStopped = False
    monitorNode = False
    lowAddr = "\xFF\xFF\xFF"
    

    atStartup() # invoke user customizations
    computeSleepTime() # convert sleep ticks to sleep seconds

#
# Command routines
#
def pollingStopped():
    global runFlag
    global pollStopped
    runFlag = True
    pollStopped = True;

def sleepyModeFalse():
    global runFlag   

    runFlag = False
    
def sleepyModeTrue():
    global runFlag
    runFlag = True
    
def forceElection():
    election()        

def sleepyMode(value):
    """Use this to turn the "sleepy mesh" on (True) or off (False)"""
    global runFlag
    runFlag = value

def extendWakeInterval(offset):
    """Extends wake period temporarily (milliseconds).
       0 just restarts the countdown,
       > 0 restarts it plus adds *additional time*"""
    global wakeTimer
    wakeTimer = -offset

#
# Internal routines
#

def computeSleepTime():
    """compute sleepTime from sleepTicks
       Needed because time per tick depends upon sleep mode"""
    global sleepMode, sleepTicks, sleepTime
    if sleepMode == 0:
        #only an approximation
        sleepTime = (sleepTicks * 10) + (sleepTicks*24)/100
    else:
        sleepTime = (sleepTicks * 10)


def timer100msEvent(currentMs):
    """background polling for sleepy mesh"""
    global secondCounter
    global runFlag, canLead, isLeader
    global wakeTime, graceTime
    global wakeTimer, electTimer
    global sleepTime
    global pollStopped

    every100ms() # invoke user customizations

    secondCounter += 1
    if secondCounter >= 10:
        everySecond() # invoke user customizations
        secondCounter = 0

    if electTimer > 0 : # election in progress, only do this if in run mode
        electTimer -= 1
        if electTimer <= 0: # time's up!
            showLeadership(isLeader)
            if isLeader:
                if runFlag:
                    sendSleep()
    elif not runFlag:
        return
    
    elif runFlag:
        wakeTimer += 1
        if pollStopped and isLeader and wakeTimer <= wakeTime:
            sendSleep()
        elif wakeTimer > wakeTime:
            if isLeader:
                sendSleep()
            elif canLead:
                # The following timeout may seem long, but is necessary
                # to prevent "independent sleepy meshes" from forming
                if wakeTimer > (sleepTime+wakeTime+graceTime):
                    election()

def election():
    """Called to re-trigger an election"""
    global isLeader
    global electTime, electTimer
    global monitorNode

    showElection() # invoke user customizations
    electTimer = electTime
    isLeader = True
    if monitorNode:
        mcastRpc(1,5,"nominate","\x00\x00\x01")   #monitorNode must win election!
    else:      
        mcastRpc(1,5,"nominate",localAddr())
    
def rpcsentEvent(cookie):
    global sleepCookie

    if cookie == sleepCookie and runFlag:
        doSleep(sleepTicks)
        

def sendSleep():
    """tell everyone else to go to sleep, and go to sleep ourself"""
    global sleepTicks
    global sleepCookie
    global runFlag
    
    if (runFlag):
        mcastRpc(1,5,'gotoSleep',sleepTicks)
        sleepCookie = getInfo(9)  #cookie of this RPC
    

def doSleep(dur):
    """this is the actual "going to sleep" code"""
    global isLeader, wakeTimer, electTimer, sleepMode
    global pollStopped
    global monitorNode
    global lowAddr
    
    pollStopped = False
    lowAddr = "\xFF\xFF\xFF"    #reset low address
    
    beforeSleep(dur)
    sleep(sleepMode, dur)
    afterSleep()

    # now we are back awake
    showLeadership(isLeader)

    # Let outsiders know the mesh is awake
    if isLeader:
        if monitorNode:
            monitorMeshUp()     #helper routine supplied by monitor node
        else:
            mcastRpc(1,5, 'meshUp', localAddr())

    wakeTimer = 0
    electTimer = 0

#
# Call-in routines, for other scripts to invoke
#



def nominate(addr):
    """Broadcast by nodes nominating themselves as the new leader"""
    global canLead, isLeader, electTimer, electTime
    global monitorNode
    global lowAddr
    # this is sometimes called the "bully" algorithm
    # highest priority (AKA lowest address) eligible node wins the election
    
    if (addr < lowAddr):
        lowAddr = addr      #remember the low address seen to avoid race conditions
    if canLead and (monitorNode or (localAddr() < lowAddr)):
        lowAddr = localAddr()
        election()
    else:
        isLeader = False
    showElection()
    electTimer = electTime

def meshUp():
    """This function does nothing in nodes that are part of the "sleepy mesh"."""
        
    return

def gotoSleep(dur):
    """commands nodes to go to sleep"""
    global isLeader
   
        
    if rpcSourceAddr() != localAddr():
        isLeader = False
    
    gotoSleep2(dur)

def gotoSleep2(dur):
    """commands nodes to go to sleep, but leaves leadership intact"""
    global runFlag
 
    if runFlag:   #if not leader, do not sleep until we see the meshup RPC
        doSleep(dur)

#
# Test/demo routines
#

def setSleepMode(mode):
    """override default sleep mode"""
    global sleepMode
    sleepMode = mode
    computeSleepTime()

