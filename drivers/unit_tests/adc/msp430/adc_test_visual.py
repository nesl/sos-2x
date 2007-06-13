import socket
import struct
import sys
import wx
import wx.lib.plot as plot
import thread

ACCELEROMETER_MODULE = 0x80

EVT_RESULT_ID = wx.NewId()

def EVT_RESULT(win, func):
    """Define Result Event."""
    win.Connect(-1, -1, EVT_RESULT_ID, func)

class ResultEvent(wx.PyEvent):
    """Simple event to carry arbitrary result data."""

    def __init__(self, data):
        """Init Result Event."""
        wx.PyEvent.__init__(self)
        self.SetEventType(EVT_RESULT_ID)
        self.data = data
                                                
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

class accel_test(wx.Frame):
    """ Small example of accelerometer usage. It simulates a virtual
    dice and shows which side of the dice is up.
    """
    def __init__(self, parent, id, title):
        wx.Frame.__init__(self, parent, id, title, size=(180, 280))
        sizer = wx.GridSizer(1, 1);
        self.SetSizer(sizer)

        self.g = plot.PlotCanvas(self)
        self.g.SetEnableLegend(True)
        sizer.Add(self.g, 0, wx.EXPAND)
        self.index = 0  
        self.store = []
        EVT_RESULT(self, self.OnResult)

        try:
            thread.start_new_thread(self.output_thread, ())
        except thread.error:
            print error
                                                        
    def OnResult(self, event):
        self.store.append((self.index, 100))
        self.index += 1
        for d in event.data:
            self.store.append((self.index, d))
            self.index += 1
        self.store = self.store[-2000: -1]
        print self.index
        if self.index%1054 == 0:
            line = plot.PolyLine(self.store, width=1)
        
            gc = plot.PlotGraphics((line,), 'Sound', 'Time', 'Sound')
            self.g.Draw(gc, yAxis = (0, 4096))

    def output_thread(self):
        self.sc = SocketClient("127.0.0.1", 7915)
        state = -1
        oldstate = -1
        while 1:
            data = ord(self.sc.s.recv(1))
            if data == ACCELEROMETER_MODULE:
                s = self.sc.s.recv(7)
                (src_mod, dst_addr, src_addr, msg_type, msg_length) = struct.unpack("<BHHBB", s)
                s = self.sc.s.recv(msg_length)
                data = struct.unpack("<"+msg_length/2*"H", s)
                wx.PostEvent(self, ResultEvent(data))

class MyApp(wx.App):
    def OnInit(self):
        frame = accel_test(None, -1, 'Plotting')
        frame.Show(True)
        self.SetTopWindow(frame)
                                   
        return True
                                            
if(__name__ == "__main__"):
                                               
    app = MyApp(0)
    app.MainLoop()
                                                        
