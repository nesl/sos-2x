import sys
import os
sys.path.append(os.environ['SOSROOT'] + '/modules/unit_test/pysos/')
import socket
import signal
import struct
import pysos

TEST_MODULE = 0x80
CENTROID = 512
MSG_DATA_READY = 33 
oldstate = -1
state = -1
count = {}
total = {}

def panic_handler(signum, frame):
    print "it is likely that your base station node has entered panic mode"
    print "please reset the node"
    sys.exit(1)

def accel_test(msg):
    """ Small example of accelerometer usage. It simulates a virtual
    dice and shows which side of the dice is up.
    """
    global oldstate
    global state

    signal.alarm(60)

    (nodeid, change_freq, packetid, accelid, value) = pysos.unpack("<BBHBH", msg['data'])

    if nodeid not in count.keys():
	count[nodeid] = packetid
	total[nodeid] = 0
    else:

	if ( packetid % 1000 == 0):
	    print "total packets lost for node %d after 1000 messages is %d" %(nodeid, total[nodeid])
	    total[nodeid] = 0
        if (packetid - count[nodeid] ) > 1 :
	    total[nodeid] = total[nodeid] + 1

	if (change_freq == 2):
	    print "node %d changing frequency to 50hz" %nodeid
	elif (change_freq == 1):
	    print "node %d changing frequency to 100hz" %nodeid

	count[nodeid] = packetid
    g = (value-512)/1024.0*3000/333.0
    if abs(g)>0.8:
	if g > 0:
	    state = accelid
	else:
	    state = 7-accelid
    if oldstate != state:
	print "Side %d is up"%(state,)
	oldstate = state

if __name__ == "__main__":
    srv = pysos.sossrv()
    msg = srv.listen()


    srv.register_trigger(accel_test, did=TEST_MODULE, type=MSG_DATA_READY)

    signal.signal(signal.SIGALRM, panic_handler)
    signal.alarm(60)
    while(1):
        continue
