import pysos

class DB_Interface():
    def __init__(self):
	self.srv = pysos.sossrv()

	self.srv.register_trigger(self.response_handler, sid=128, type=34)
	self.out = 0

    def setup_output(self, output_method):
	self.out = output_method

    def response_handler(self, msg):
#o	(sid, qid, n_remain, sensor, value) = pysos.unpack('<BHHBH', msg['data'])
        trash = pysos.unpack('<'+10*'B', msg['data'])
	(qid, n_remain, n_results) = pysos.unpack('<HHB', msg['data'])
	results = pysos.unpack('<'+n_results*'BH', msg['data'])

        res_dict = {}
	for i in range(len(results) / 2):
	    res_dict[results[i*2]] = results[i*2+1]
	print "new response!"
	print qid,
	print n_remain
	print n_results
	print res_dict 
	self.out(qid, n_remain, res_dict)

    def insert_new_query(self,qid, total_samples, interval,  trigger_list, sensor_list, qual_list):
	print qid
	print total_samples
	print interval
	print trigger_list
	print sensor_list
	print qual_list

	num_sensor = len(sensor_list)
	num_qual = 0 #len(qual_list)

	data = pysos.pack('<HHIBBH' + num_sensor*'B' + num_qual * 'BBH', 
		qid,
		interval,
		total_samples, 
		num_sensor,
		num_qual,
		0,
		*(i for i in sensor_list))
	self.srv.post(daddr = 1, saddr = 0, did = 128,
		sid = 128, type = 33, data = data)
	

if __name__ == '__main__':
    db = DB_Interface()
    db.insert_new_query(2, 20, 1024, [], [4], [])

