import pysos

# these two variables should be changed depending on the test drivers PID
# and the type of message it will be sending, If you are using the generic_test.c 
# then it is likely these two values can stay the same
TEST_MODULE = 0x80
MSG_TEST_DATA= 33

# variables holding new and old sensor values
# this can be replaces with whatever you want since this is specific to
# what the test driver expects for data
oldstate = -1
state = -1

# a signal handler that will go off for an alarm
# it is highly suggested that you use this since it is the easiest way to test if your
# node has entered panic mode via the script
def panic_handler(signum, frame):
    print "it is highly likely that your node has entered panic mode"
    print "please reset the node"
    sys.exit(1)

# message handler for messages of type MSG_DATA_READY
def accel_test(msg):
    """ Small example of test driver usage. It simulates a virtual
    dice and shows which side of the dice is up.
    """
    global oldstate
    global state

    print "message recieved"
    #unpack the values we are expecting, in this case it is the acclerometer id
    # and a value from the sensor
    (accelid, value) = pysos.unpack("<BH", msg['data'])
    print accelid
    print value
    # these are some simple calculations to test the sensor value we have gotten
    # this is the part which you need to fill in in order to verify that the driver is working
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

    # here we set up a connection to sossrv using the pysos module
    # and begin listening for messages
    # we also register our function above with the server so that it is called
    # when the appropriate message type is recieved
    srv = pysos.sossrv()
    msg = srv.listen()

    srv.register_trigger(generic_test, type=MSG_TEST_DATA)

    # register the signal handler and begin an alarm that will wait for 60 seconds before going off
    # other times for the alarm might be good, use your own judgement based on your test
    signal.signal(signum.SIGALRM, panic_handler)
    signal.alarm(60)

    # we do this so since the test_suite application has information regarding the amount of time
    # each test should be run.  after the amount of time specified in test.lst, test_suite will 
    # end this script and move to another test
    while(1):
        continue
