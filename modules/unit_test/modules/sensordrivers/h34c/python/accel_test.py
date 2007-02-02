import socket
import struct

ACCELEROMETER_MODULE = 0x80
CENTROID = 512

class SocketClient:
    """ This class connects to the sos server. """
    def __init__(self, host,port):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connected=1

        try:
            self.s.connect((host, port))
        except socket.error:
            self.connected=0
        self.data=""


    def close(self):
        if(self.connected):
            self.s.shutdown(2)
            self.s.close()

    def send(self, command):
        if(self.connected):
            self.s.send(command)
        else:
            print "Error: No connection to command server..."

class accel_test:
    """ Small example of accelerometer usage. It simulates a virtual
    dice and shows which side of the dice is up.
    """
    def __init__(self):

        self.sc = SocketClient("127.0.0.1", 7915)
        state = -1
        oldstate = -1
        while 1:
            data = ord(self.sc.s.recv(1))
            if data == ACCELEROMETER_MODULE:
                s = self.sc.s.recv(7)
                (src_mod, dst_addr, src_addr, msg_type, msg_length) = struct.unpack("<BHHBB", s)
                s = self.sc.s.recv(3)
                (accelid, value) = struct.unpack("<BH", s)
                g = (value-512)/1024.0*3000/333.0
                #print "saddr: %d, accelid: %d, value: %f"%(src_addr, accelid, (value-512)/1024.0*3000/333.0)
                if abs(g)>0.8:
                    if g > 0:
                        state = accelid
                    else:
                        state = 7-accelid
                if oldstate != state:
                    print "Side %d is up"%(state,)
                    oldstate = state


if __name__ == "__main__":
    at = accel_test()
