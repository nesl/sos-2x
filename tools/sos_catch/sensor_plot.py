#! /usr/bin/env python

import wx
import wx.lib.plot as plot
import pysos
import sys
from threading import Lock
import time
import struct
import thread

sys.stderr = open("stderr", "w")
EVT_RESULT_ID = wx.NewEventType()

class ResultEvent(wx.PyEvent):
    """Simple event to carry arbitrary result data."""
    def __init__(self, data):
        """Init Result Event."""
        wx.PyEvent.__init__(self)
        self.SetEventType(EVT_RESULT_ID)
        self.data = data

class MyFrame(wx.Frame):
	def __init__(self, parent, id, title):
		wx.Frame.__init__(self, parent, id, title, size=(510, 340))
		self.panel = wx.Panel(self)
		self.Center()
		# Initialize a Box to hold all the graph plots
		self.box = wx.BoxSizer(wx.VERTICAL)
		# Initialize dictionary of graph plots along with corresponding sensor id
		# {sensor id 0 : plot 0, ...}
		self.plotter = {}
		# Initialize dictionary of data along with corresponding sensor id
		# {sensor id 0 : data 0, ...}
		self.data = {}
		# Configuration variables
		# Header size of sensor data structure in new API
		self.SENSOR_DATA_HDR_SIZE = 4
		self.sequence = 0
		self.value = 0
		self._rcvLock = Lock()
		self.Connect(-1, -1, EVT_RESULT_ID, self.PlotGraph)

	def CreateGraphs(self, sensorid):
		"""Add all graphs for displaying sensor data.
		sensorid: list of sensor id's corresponding to each plot"""
		for i in range(len(sensorid)):
			# Add a graph plot to list
			self.plotter[sensorid[i]] = plot.PlotCanvas(self.panel)
			#self.plotter.append((sensorid[i], plot.PlotCanvas(self.panel)))
			# Initialize data array for each sensor
			self.data[sensorid[i]] = []
			# Add the above graph plot to the Box
			self.box.Add(self.plotter[sensorid[i]], 1, wx.EXPAND)
		# Add the Box to the panel
		self.panel.SetSizer(self.box)

	def ConnectSos(self, host='localhost', port=7915):
		"""Connect to a running SOS server using PySOS and
		   register a listener function for sensor data."""
		# Connect to SOS server
		try:
			self.srv = pysos.sossrv(host, port)
		except IOError:
			print('Could not connect to SOS server..')
			self.Close()
		# Register the listener function
		self.srv.register_trigger(self.GetData, type=0x81)

	def GetData(self, msg):
		"""Update the graph on receiving sensor data."""
		self._rcvLock.acquire()
		print ('Sequence %d received.'%(self.sequence))
		# unpack data to get sensor id
		(status, sensor, num_samples) = struct.unpack("<BBH", msg['data'][:self.SENSOR_DATA_HDR_SIZE]) 
		print('Status: %d Sensor: %d Number of Samples: %d' % (status, sensor, num_samples))
		# add data to its respective array
		# clip data to adjust to the window size
		# list of (x,y) data point tuples
		data = [(1,2), (2,3), (3,5), (4,6), (5,8), (6,8), (12,10), (13,4)]
		self.value = (self.value + 1) % 15
		data[0] = (1, self.value)
		# Post data to the plotting function
		event = ResultEvent([sensor, data])
		wx.PostEvent(self, event)
		print ('Sequence %d processed.'%(self.sequence))
		self.sequence = self.sequence + 1
		self._rcvLock.release()
		
	def PlotGraph(self, event):
		"""Plot the graph for corresponding sensor data"""
		line = plot.PolyLine(event.data[1], colour='red', width=1)
		# also draw markers, default colour is black and size is 2
		# other shapes 'circle', 'cross', 'square', 'dot', 'plus'
		marker = plot.PolyMarker(event.data[1], marker='triangle')
		# set up text, axis and draw
		gc = plot.PlotGraphics([line, marker], 'Line/Marker Graph', 'x axis', 'y axis')
		# Draw graphs for each plot
		#for id, graph in self.plotter.iteritems():
		self.plotter[event.data[0]].Draw(gc, xAxis=(0,15), yAxis=(0,15))

	def __del__(self):
		self.Close()

	def Close(self):
		"""Cleanup before exit."""
		self.srv.disconnect()
		pass


def __test():
	app = wx.App()
	f = MyFrame(None, -1, "Aceelerometer Data")
	sensorid = [0, 1]
	f.CreateGraphs(sensorid)
	f.ConnectSos()
	f.Show(True)
	app.MainLoop()



if (__name__ == '__main__'):
	__test()


