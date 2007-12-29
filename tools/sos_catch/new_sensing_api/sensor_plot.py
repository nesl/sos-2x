#! /usr/bin/env python

import wx
import wx.lib.plot as plot
import pysos
import sys
from threading import Lock
import time
import struct

sys.stderr = open("stderr", "w")
EVT_RESULT_ID = wx.NewEventType()

class ResultEvent(wx.PyEvent):
    """Simple event to carry arbitrary result data."""
    def __init__(self, data):
        """Init Result Event."""
        wx.PyEvent.__init__(self)
        self.SetEventType(EVT_RESULT_ID)
        self.data = data

class SensorDataFrame(wx.Frame):
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
		# {sensor id 0 : [(x1,y1), (x2,y2), ...], ...}
		self.data = {}
		# Initialize dictionary of timestamps of first data buffer
		# for each sensor
		# {sensor id 0: timestamp, ...}
		self.init_timestamp = {}
		# Configuration variables
		# Header size of sensor data structure in new API
		self.SENSOR_DATA_HDR_SIZE = 8
		# Window size for displaying current data
		self.WINDOW_SIZE = 1024
		# Sample rate in Hz
		self.SAMPLE_RATE = 250
		# Clock frequency of clock utlized for timestamps
		self.TIME_CLOCK_SOURCE = 32768
		# Set the range for y-axis as (lower bound, upper bound)
		self.Y_AXIS_RANGE = (2000, 3000)
		# Internal variables
		self._x_lower = 0
		self._x_upper = 15
		self._rcvLock = Lock()
		# Set event handler for posting data
		self.Connect(-1, -1, EVT_RESULT_ID, self._PlotGraph)

	def CreateGraphs(self, sensorid):
		"""Add all graphs for displaying sensor data.
		sensorid: list of sensor id's corresponding to each plot"""
		for i in range(len(sensorid)):
			# Add a graph plot to list
			self.plotter[sensorid[i]] = plot.PlotCanvas(self.panel)
			#self.plotter.append((sensorid[i], plot.PlotCanvas(self.panel)))
			# Initialize data array for each sensor
			self.data[sensorid[i]] = []
			# Initialize timestamp for each sensor data
			self.init_timestamp[sensorid[i]] = -1
			# Add the above graph plot to the Box
			self.box.Add(self.plotter[sensorid[i]], 1, wx.EXPAND)
		# Add the Box to the panel
		self.panel.SetSizer(self.box)

	def SetWindowSize(self, size):
		"""Set window size of current display"""
		self.WINDOW_SIZE = size

	def SetSampleRate(self, rate):
		"""Set sample rate of sensor data"""
		self.SAMPLE_RATE = rate

	def SetTimeClockSource(self, source):
		"""Set the frequency of clock used for timestamps"""
		self.TIME_CLOCK_SOURCE = source

	def SetYAxisRange(self, lower, upper):
		"""Set the range of Y-axis for display"""
		self.Y_AXIS_RANGE = (lower, upper)

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
		self.srv.register_trigger(self._GetData, type=0x81)

	def _GetData(self, msg):
		"""Update the graph on receiving sensor data."""
		self._rcvLock.acquire()
		# unpack data to get sensor id
		(status, sensor, num_samples, timestamp) = struct.unpack("<BBHL", msg['data'][:self.SENSOR_DATA_HDR_SIZE]) 
		sensordata = struct.unpack("<"+num_samples*'H', msg['data'][self.SENSOR_DATA_HDR_SIZE:])
		# Check if this is the first data packet for 'sensor'
		if self.init_timestamp[sensor] < 0:
			self.init_timestamp[sensor] = timestamp
		# Convert timestamp into ms
		t = ((timestamp - self.init_timestamp[sensor]) * 1000.0)/self.TIME_CLOCK_SOURCE
		# add data to its respective array
		for i in range(len(sensordata)):
			self.data[sensor].append((t, sensordata[i]))
			t += (1000.0/self.SAMPLE_RATE)
		# clip data to adjust to the window size
		self.data[sensor][0:max(-self.WINDOW_SIZE, -len(self.data[sensor]))] = []
		# Update the lower and upper bounds for x-axis
		self._x_lower = self.data[sensor][0][0]
		self._x_upper = self.data[sensor][-1][0]
		# Post data to the plotting function
		event = ResultEvent([sensor, self.data[sensor]])
		wx.PostEvent(self, event)
		self._rcvLock.release()
		
	def _PlotGraph(self, event):
		"""Plot the graph for corresponding sensor data"""
		self._rcvLock.acquire()
		line = plot.PolyLine(event.data[1], colour='red', width=1)
		# To draw markers: default colour = black, size = 2
		# shapes = 'circle', 'cross', 'square', 'dot', 'plus'
		#marker = plot.PolyMarker(event.data[1], marker='triangle')
		# set up text, axis and draw
		#gc = plot.PlotGraphics([line, marker], 'Acceleration (ADC units)', 'x axis', 'y axis')
		gc = plot.PlotGraphics([line], 'Acceleration (ADC units)', 'x axis', 'y axis')
		# Draw graphs for each plot
		self.plotter[event.data[0]].Draw(gc, xAxis=(self._x_lower, self._x_upper), yAxis=self.Y_AXIS_RANGE)
		self._rcvLock.release()

	def __del__(self):
		self._Close()

	def _Close(self):
		"""Cleanup before exit."""
		self.srv.disconnect()
		pass


def __test():
	"""Example usage for 1 axis accelerometer sampling at 250 hz"""
	app = wx.App()
	f = SensorDataFrame(None, -1, "Aceelerometer Data")
	sensorid = [0]
	f.CreateGraphs(sensorid)
	f.SetWindowSize(1024)
	f.SetSampleRate(250)
	f.SetYAxisRange(2250, 2470)
	f.ConnectSos()
	f.Show(True)
	app.MainLoop()



if (__name__ == '__main__'):
	__test()


