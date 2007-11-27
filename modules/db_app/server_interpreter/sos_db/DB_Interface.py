from xml.dom import minidom
import pysos
import re
import subprocess
import os
from struct import unpack
import readline

NEW_QUERY = 2
comp_ops = {'<':1, '>':2, '=':3, '<=':4, '>=':5, '!=':6}
rel_ops = {'and':1, 'or':2, 'and not':3, 'or not':4, 'not':5}

provided_triggers = {'led':1}
provided_values = {'RED_ON':1, 'GREEN_ON':2, 'YELLOW_ON':3,
	                  'RED_OFF':4, 'GREEN_OFF':5, 'YELLOW_OFF':6,
																			'RED_TOGGLE':7, 'GREEN_TOGGLE':8, 'YELLOW_TOGGLE':9}

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
		self.query_files = {}
		self.curr_board = ""
		self.curr_queries = {}
		self.curr_id = 1
		self.install_drivers(default_board)
		self.srv.register_trigger(self.response_handler, sid=128, type=34)
		self.out = self.standard_out

	def standard_out(self, hdr, values):
		qid = hdr[0]
		if qid in self.query_files.keys():
			f = self.query_files[qid]
			hdr_str = ";".join([str(e) for e in hdr])
			val_str = ";".join([str(e) for e in values])
			f.write("hdr: %s\tvalues: %s\n" %(hdr_str, val_str))
			f.flush()
		else:
			print hdr
			print values
	  
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
		trash = data[:30]
		hdrstr = data[30:36]
		valuesstr = data[36:]

		hdr = unpack('<HHBB', hdrstr)
		values = unpack('<'+hdr[2]*'BH', valuesstr)

		self.out(hdr, values)
		
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
	def insert_new_query(self,qid, total_samples, interval,  trig_list =[], sensor_list=[], qual_list= []):
		print qid
		print total_samples
		print interval
		print trig_list 
		print sensor_list
		print qual_list

		if qid in self.curr_queries.keys():
			print "qid: %d is already in use" %qid
			return False

		self.curr_queries[qid] = 1
		num_trigs = len(trig_list)
		num_sensor = len(sensor_list)
		num_qual = len(qual_list)

 		sub = []
 		for i in trig_list:
			sub.append(i[0])
 			sub.append(i[1])

 		qual = []
		for i in qual_list:
			qual.append(i[0])
 			qual.append(i[1])
 			qual.append(i[2])

 		data_list = [i for i in sub] + [i for i in sensor_list] + [i for i in qual]

		data = pysos.pack('<BHHIBBH' + num_trigs*'BB' + num_sensor*'B' + num_qual * 'BBH', 
			NEW_QUERY,
			qid,
			total_samples, 
			interval,
			num_sensor,
			num_qual,
			num_trigs,
			*(i for i in data_list))

		self.srv.post(daddr = 1, saddr = 0, did = 128,
		sid = 128, type = 33, data = data)
		return True
	
	def remove_queries(self):
	  data = pysos.pack('<BB',1, 0)
	  self.srv.post(daddr =1, saddr=0, did=128, sid=128, type=33,data=data )

	def parse_query(self, query):
			# first check for the remove query command
			words = re.match(r'remove all', query)
			if words:
					self.remove_queries()
					return

			# first get the values we want to read, and the board type
			print query
			words = re.match(r'select\s+(\S+)\s+from\s+([a-zA-Z0-9]+)\s*(.*)', query)
			if words:
				sensors = words.group(1)
				board = words.group(2)
#self.install_drivers(board)
				rest = words.group(3)
			else:
				print "invalid select statement"
				return

			# now split all the sensor types
			words = re.match(r'([a-zA-Z0-9\-_()]+)((,)([a-zA-Z0-9\-_()]+))*', sensors)
			s_list = []
			if words:
					s_list = [self.sensor[board][e].get_value() for e in words.groups() if e != None and e[0] != ',' and e in self.sensor[board].keys() ]
			else:
					print "invalid list of sensors"
					return
				
			qual_list = []
			# now get the qualifiers
			words = re.match(r'where\s+([a-zA-Z0-9\-_]+)\s+([<>=])\s+(\d+)\s+(.*)', rest)
			if words:
					new_qual = self.parse_qualifier(board, words.group(1), words.group(2), words.group(3))
					if new_qual[0] not in s_list:
						s_list.append(new_qual[0])
					qual_list.append(new_qual)
					rest = words.group(len(words.groups()))
 					print words.groups()
 					while (rest[0] != 'w'):
						words = re.match(r'(and|or)\s+([a-zA-Z0-9\-_]+)\s+([<>=])\s+(\d+)\s+(.*)', rest)
						if words:
							new_qual = self.parse_qualifier(board, words.group(2), words.group(3), words.group(4))
							print words.group(1)
							print words.group(2)
							print words.group(3)
							print words.group(4)
							if new_qual[0] not in s_list:
								s_list.append(new_qual[0])
							qual_list.append(new_qual)
							rest = words.group(len(words.groups()))


			# now get the interval and sample number
			words = re.match(r'with\s+sample_rate\s+(\d+)\s+number_samples\s+(\d+)\s*(.*)', rest)
			if words:
					sample_rate = int(words.group(1))
					num_samples = int(words.group(2))
					rest = words.group(len(words.groups()))
			else:
				print "incorrect declaration of query details"
				return

			trig_list = []
			# now get any triggers that exist, currntly we only support one trigger per query
			words = re.match(r'on\s+fire\s+(.*)', rest)
			if words:
					rest = words.group(1)
 					print rest
					words = re.match(r'trigger\s*\(\s*([a-zA-Z0-9_]+)\s*,\s*([a-zA-Z0-9_]+)\s*\)\s*(.*)', rest)
					while words:
						command = words.group(1)
						value = words.group(2)
						new_trig = [0,0]
						if command in provided_triggers.keys():
							new_trig[0] = provided_triggers[command]
						if value in provided_values.keys():
							new_trig[1] = provided_values[value]
						trig_list.append(new_trig)
						rest = words.group(3)
						words = re.match(r'trigger\s*\(\s*([a-zA-Z0-9_]+)\s*,\s*([a-zA-Z0-9_]+)\s*\)\s*(.*)', rest)

			words = re.match(r'to\s+file\s+(\S+)\s*(.*)', rest)
			if words:
				rest = words.group(2)
 				out_f_name = words.group(1)
 				out_f = open(out_f_name, 'w')
				self.query_files[self.curr_id] = out_f

 			ret = self.insert_new_query(self.curr_id, num_samples, sample_rate, trig_list, s_list, qual_list)
 			if ret:
 				print "your query id is: %d" %self.curr_id
 			self.curr_id += 1

	def parse_qualifier(self, board, sensor_name, compare_op, compare_val):
		if sensor_name in self.sensor[board].keys():
			sen = self.sensor[board][sensor_name]
		else:
			print "invalid sensor %s" %sensor_name
			return

		if compare_op in comp_ops.keys():
			op = comp_ops[compare_op] << 4
			op = op | rel_ops['and']
		else:
			print "invalid comparison operator %s" %compare_op
			return
		value = int(compare_val)
		return [sen.get_value(), op, value]

if __name__ == '__main__':
		db = DB_Interface()
		db.display_drivers()
 		
 		q = readline.get_line_buffer()
 		while (q != 'quit'):
			db.parse_query(q)
 			q = raw_input('--> ')

#		db.parse_query("select x-accel from mts310 where y-accel > 123 and x-accel < 1 with sample_rate 1024 number_samples 20")
#		db.parse_query("select mag-1 from mts310 with sample_rate 4024 number_samples 100")
#		db.insert_new_query(2, 20, 1024, [], [4], [])

