import sys
import os
sys.path.append(os.environ['SOSROOT'] + '/modules/unit_test/pysos/')
import pysos
import signal

# these two variables should be changed depending on the test drivers PID
# and the type of message it will be sending, If you are using the generic_test.c 
# then it is likely these two values can stay the same
TEST_MODULE = 0x80
MSG_TEST_DATA= 33
ALARM_LEN = 60

START_DATA = 100
FINAL_DATA = 200
TEST_FAIL = 155
TEST_PASS = 255

# a signal handler that will go off for an alarm
# it is highly suggested that you use this since it is the easiest way to test if your
# node has entered panic mode via the script
def panic_handler(signum, frame):
    print >> sys.stderr, "it is highly likely that your node has entered panic mode"
    print >> sys.stderr, "please reset the node"
    sys.exit(1)

# message handler for messages of type MSG_DATA_READY
def generic_test(msg):
    """ Small example of test driver usage. It simulates a virtual
    dice and shows which side of the dice is up.
    """

    print "message recieved"
    signal.alarm(ALARM_LEN)


if __name__ == "__main__":

    # here we set up a connection to sossrv using the pysos module
    # and begin listening for messages
    # we also register our function above with the server so that it is called
    # when the appropriate message type is recieved
    srv = pysos.sossrv()
    msg = srv.listen()

    srv.register_trigger(generic_test, sid=TEST_MODULE, type=MSG_TEST_DATA)

    # register the signal handler and begin an alarm that will wait for 60 seconds before going off
    # other times for the alarm might be good, use your own judgement based on your test
    signal.signal(signal.SIGALRM, panic_handler)
    signal.alarm(ALARM_LEN)

    # we do this so since the test_suite application has information regarding the amount of time
    # each test should be run.  after the amount of time specified in test.lst, test_suite will 
    # end this script and move to another test
    while(1):
        continue
