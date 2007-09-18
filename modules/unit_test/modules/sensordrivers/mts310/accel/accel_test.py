import sys
import os
import socket
import signal
import struct
import pysos

TEST_MODULE = 0x80
MSG_DATA_READY = 33 
oldstate = -1
state = -1
count = {}
total = {}
centroid = [475, 709]

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
        
	accelid -= 4

	if abs(value-centroid[accelid]) > 100:
	    print >> sys.stderr, "the value is to far out of range for accel id %d, the value is %d" %(accelid+4, value)
	    print >> sys.stderr, "if the mote is not moving, then this is an error"
	else:
	    print "the value is acceptedable, for accelid %d, the value is %d" %(accelid+4, value)
if __name__ == "__main__":
    srv = pysos.sossrv()

    srv.register_trigger(accel_test, did=TEST_MODULE, type=MSG_DATA_READY)

    signal.signal(signal.SIGALRM, panic_handler)
    signal.alarm(60)
    while(1):
        continue
