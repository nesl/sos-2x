import socket
import struct
import pysos

ACCELEROMETER_MODULE = 0x80
CENTROID = 512
MSG_DATA_READY = 33 
oldstate = -1
state = -1

def accel_test(msg):
    """ Small example of accelerometer usage. It simulates a virtual
    dice and shows which side of the dice is up.
    """
    global oldstate
    global state

    print "message recieved"
    (accelid, value) = pysos.unpack("<BH", msg['data'])
    print accelid
    print value
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

    srv.register_trigger(accel_test, type=MSG_DATA_READY)

    while(1):
        continue
