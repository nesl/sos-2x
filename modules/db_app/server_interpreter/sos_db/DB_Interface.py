from xml.dom import minidom
import pysos
import re
import subprocess
import os
from struct import unpack
import readline

class Sensor_Driver():
  def __init__(self, n, v, l, f):
    self.name = n
    self.value = v
    self.loc = os.environ['SOSROOT'] + l
    self.file = f

  def get_name(self):
    return self.name

  def get_value(self):
		return self.value
  def install_driver(self, target='micaz'):
				cmd_clean = ['make', '-C', self.loc, 'clean']
				cmd_make = ['make', '-C', self.loc, target]
				cmd_install = ['sos_tool.exe', '--insmod=%s/%s.mlf' %(self.loc, self.file)]

				print cmd_install
				subprocess.call(cmd_clean)
				subprocess.call(cmd_make)
				subprocess.call(cmd_install)

class DB_Interface():
	def __init__(self):
		self.srv = pysos.sossrv()
		self.sensor ={} 
	#parse the sensor list
		xml_doc = minidom.parse(os.environ['SOSROOT']+'/modules/db_app/server_interpreter/sos_db/sensors.xml')
		boards= xml_doc.getElementsByTagName('board')
		for b in boards:
			board = b.getElementsByTagName('board-name')
			sensors = b.getElementsByTagName('sensor')

			b_name = board[0].childNodes[0].data
			s_list = {} 
			for s in sensors:
					pair = s.getElementsByTagName('pair')
					loc = s.getElementsByTagName('location')
					file = s.getElementsByTagName('file-name')

					l = loc[0].childNodes[0].data
					f = file[0].childNodes[0].data
					for n in pair:
							name = n.getElementsByTagName('name')
							s_name = name[0].childNodes[0].data
							s_value = int( n.getElementsByTagName('value')[0].childNodes[0].data )
							s_list[s_name] =  Sensor_Driver(s_name, s_value,l, f) 

			self.sensor[b_name] = s_list
	
		default_board = 'mts310'
		self.curr_board = ""
		self.curr_queries = {}
		self.curr_id = 1
		self.install_drivers(default_board)
		self.srv.register_trigger(self.response_handler, sid=128, type=34)
		self.out = 0

	def setup_output(self, output_method):
		self.out = output_method

	def install_drivers(self, board='mts310', target='micaz'):
		if (self.curr_board == board):
			return
		for s in self.sensor[board].keys():
			sen = self.sensor[board][s]
			sen.install_driver()
		self.curr_board = board

	def clear_drivers(self):
		cmd_clear = ['sos_tool.exe', '--rmmod=0']
		subprocess.call(cmd_clear)

	def display_drivers(self):
		for key in self.sensor.keys():
				print key
				for s in self.sensor[key].keys():
					print "\t" + str(self.sensor[key][s].get_name()) + " " + str(self.sensor[key][s].get_value())
	
				  
	def response_handler(self, msg):
		data = msg['data']
		trash = data[:10]
		hdrstr = data[10:15]
		valuesstr = data[15:]

		hdr = unpack('<HHB', hdrstr)
		values = unpack('<'+hdr[2]*'BH', valuesstr)

		print hdr
		print values
		
#		(qid, n_remain, n_results) = pysos.unpack('<HHB', msg['data'])
#		results = pysos.unpack('<'+n_results*'BH', msg['data'])

#		res_dict = {}
#		for i in range(len(results) / 2):
#				res_dict[results[i*2]] = results[i*2+1]
#		print "new response!"
#		print qid,
#		print n_remain
#		print n_results
#		print res_dict 
#		self.out(qid, n_remain, res_dict)

	#for arguements it takes:
	# qid = integer declaring the new qid number
	# total_samples = integer declaring the number of samples
	# interval = integer declaring the sample rate
	# trigger_list = leave this blank right now
	# sensor_list = list of integers which should exist in the list of installed drivers
	# qual_list = list of byte codes declaring the specific qualifiactions needed
	def insert_new_query(self,qid, total_samples, interval,  trigger_list, sensor_list, qual_list):
		print qid
		print total_samples
		print interval
		print trigger_list
		print sensor_list
		print qual_list

		if qid in self.curr_queries.keys():
			print "qid: %d is already in use" %qid
			return False

		self.curr_queries[qid] = 1
		num_sensor = len(sensor_list)
		num_qual = 0 #len(qual_list)

		data = pysos.pack('<HHIBBH' + num_sensor*'B' + num_qual * 'BBH', 
			qid,
			total_samples, 
			interval,
			num_sensor,
			num_qual,
			0,
			*(i for i in sensor_list))

		self.srv.post(daddr = 1, saddr = 0, did = 128,
		sid = 128, type = 33, data = data)
		return True
	
	def parse_query(self, query):
			# first get the values we want to read, and the board type
			print query
			words = re.match(r'select (\S+) from ([a-zA-Z0-9]+) (.+)+', query)
			if words:
				sensors = words.group(1)
				board = words.group(2)
#self.install_drivers(board)
				rest = words.group(3)
			else:
				return

			# now split all the sensor types
			words = re.match(r'([a-zA-Z0-9\-_()]+)((,)([a-zA-Z0-9\-_()]+))*', sensors)
			s_list = []
			if words:
					s_list = [e for e in words.groups() if e != None and e[0] != ',']
			else:
					return
				
			# now get the qualifiers
			words = re.match(r'where ([a-zA-Z0-9\-_]+) ([<>=]) (\d+) ((and|or) ([a-zA-Z0-9\-_]+) ([<>=]) (\d+))* (.*)', rest)
			if words:
					rest = words.group(len(words.groups()))

			# now get the interval and sample number
			words = re.match(r'with sample_rate (\d+) number_samples (\d+)(.*)', rest)
			if words:
					sample_rate = int(words.group(1))
					num_samples = int(words.group(2))
 			else:
				return

			q_list  = []
			for s in s_list:
				q_list.append( self.sensor[board][s].get_value())
 			ret = self.insert_new_query(self.curr_id, num_samples, sample_rate, [], q_list, [])
 			if ret:
 				print "your query id is: %d" %self.curr_id
 			self.curr_id += 1

if __name__ == '__main__':
		db = DB_Interface()
		db.display_drivers()
 		
 		q = readline.get_line_buffer()
 		while (q != 'quit'):
			db.parse_query(q)
 			q = raw_input('--> ')

#		db.parse_query("select x-accel from mts310 where y-accel > 123 and x-accel < 1 with sample_rate 1024 number_samples 20")
		db.parse_query("select mag-1 from mts310 with sample_rate 4024 number_samples 100")
#		db.insert_new_query(2, 20, 1024, [], [4], [])

