"""
PySOS version 1.51 - March 22 2007
Authors: Thiago Teixeira and Neeraj Singh.

This module allows you to connect to a SOS server on any host
and port, as well as send and receive SOS messages.


INITIALIZATION:
===============

Make sure you are running the SOS server, then create a new instance:

>>> import pysos
>>> srv = pysos.sossrv()    # you may enter host=... and port=... here
>>>                         # localhost and 7915 are default


SENDING MESSAGES:
=================

There are 3 different ways to send messages. They aren't too 
different, and it's up to your personal preference which one to use:


1ST METHOD:
-----------

>>> data = pysos.pack('<BBBB', 0, 1, 2, 3)
>>> 
>>> srv.post(daddr = 5,   saddr = 3,  did  = 128, 
...          sid   = 128, type  = 32, data = data)

Any of these can be omitted, in which case the defaults specified 
with set_message_defaults() are utilized.


2ND METHOD:
-----------

This method is largely the same as the previous, but it separates
the message creation from the act of sending it:

>>> m = pysos.msg(daddr = 5,   saddr = 3,  did  = 128, 
...               sid   = 128, type  = 32, data = data)
>>>
>>> srv.post_msg(m)


3RD METHOD:
-----------

If you prefer to use SOS's post_net syntax, you may do so like this:

>>> srv.post_net(128, 128, 32, 4, data, 0, 5)

In this case, saddr is the one specified with set_message_defaults(),
or 0xFFFE by default. This is because post_net does not let you specify
your saddr in SOS.

Also note that the "length" and "flags" parameters are ignored.


RECEIVING MESSAGES:
===================

There are 2 different methods you can use. The first one is 
synchronous (blocking) and the 2nd asynchronous -- it allows you to 
register listeners and then run a non-blocking method to start listening 
for messages. You can use both of these methods with the same sossrv.
 

1ST METHOD (synchronous):
-------------------------

>>> msg = srv.listen(did   = 128,  sid   = 128, 
...                  daddr = 0x14, saddr = 0x16, 
...                  type  = 32,   nreplies = 5, timeout = 3.0)

This method returns the first matching messages. It returns the message as
a dictionary with keys 'did', 'sid', 'daddr', 'saddr', 'type, 'length',
and 'data'. To cast msg['data'] into a tuple, you may use the unpack()
method, as such:

>>> data = pysos.unpack('<LHB', msg['data'])

Where '<LHB' stands for a little endian ('<') struct composed of a
uint32 ('L'), uint16 ('H') and uint8 ('B'). For signed integers, use
lowercase letters instead. Of course, the string you feed into the unpack()
method depends on your particular data struct.


2ND METHOD (asynchronous):
--------------------------

For this method you register a trigger (i.e. listener). Then a thread
in the background will call a function of your choosing
when the trigger fires.

This is how you specify a trigger:

>>> srv.register_trigger(func,         did   = 128, 
...                      sid   = 128,  daddr = 0x14,
...                      saddr = 0x16, type  = 32)

Where you may omit any parameter (except func) to match all messages,
irrespective of that parameter's value. That is, None is a wildcard.

At any point, you may use the deregister_trigger() method to remove
triggers from the pysos instance. When deregistering a trigger, None
is once again used as wildcard.


RPC-STYLE COMMUNICATIONS:
=========================

You can also do an RPC-style call, which posts a message to the network 
and returns the response message(s):

>>> replylist = srv.post_rpc_msg(m, rtype=36, nreplies=10, timeout=5.0)

The above command creates a message dictionary (through sossrv.msg_dict)
which is sent to all the nodes. We collect up to 10 replies with message
type 36 in the variable msglist. The replies are in a list of message
dicts. If 5 seconds elapse, we just return the messages obtained thus 
far.

For those who do not wish to first create a message dict (the variable
called 'm' in the example above), there is the post_rpc() method:

>>> post_rpc(did    = 0x97, daddr  = 13, type   = 32,
...          rsid   = 0x97, rsaddr = 13, rtype  = 40,
...          timeout = 3, nreplies = 5)


MORE INFORMATION:
=================

Use each method's help function for more details.
"""

from threading import Thread, Lock, Condition
from struct import pack, unpack
import socket
from time import strftime

BCAST_ADDRESS = 0xFFFF  #move this to a higher scope, since it's useful

class sossrv:
    
	SOS_HEADER_SIZE = 8
	BCAST_ADDRESS   = 0xFFFF
	MOD_MSG_START   = 32

	def __init__(self, host=None, port=None, nid=0xFFFE, pid=128, verbose=False):

		if host: self.host = host
		else: self.host = 'localhost'

		if port: self.port = port 
		else: self.port = 7915

		self.daddr     = self.BCAST_ADDRESS
		self.saddr     = nid
		self.did       = pid
		self.sid       = pid
		self.type      = self.MOD_MSG_START
		self.verbose   = verbose 

		self._retries  = 0

		self._listening     = False
		self._listenthread  = None
		self._leftoverbytes = ''

		self._syncPkt  = []           # variable for passing packets to a sync listener
		self._syncTrig = None
		self._syncCV   = Condition()  # condition variable for sync receive

		self._triggers = []
		self._trgLock  = Lock()       # synchronizes reg/dereg against matching

		self._sock = None

		if self.verbose: print('Connecting to sossrv at %s:%d' % (self.host, self.port))
		self._connect()

		# necessary when running in fast systems to ensure that
		# pysos is connected to sossrv when the constructor returns
		while True:
			if self._listening: break


	def __del__(self):

		self.disconnect()


	def disconnect(self):

		if self.verbose: print('Disconnecting from sossrv...')

		if self._listening:
			self._sock.shutdown(socket.SHUT_RDWR)
			self._listening = False
			self._sock.close()

		if self._listenthread: self._listenthread.join()
	
	
	def reconnect(self, host=None, port=None):
		"""
		Run this if you restart sossrv and need to reconnect.
		"""
	
		if host == None : host = self.host
		if port == None : port = self.port
	
		self._connect(host=host, port=port, retries=self.retries)


	def set_msg_defaults(self, did=None, sid=None, daddr=None, saddr=None, type=None):
		"""
		Use this method to specify the default parameters for sending
		messages. This allows you to type less arguments when calling
		methods such as post() or msg().
		"""
	
		if did   == None : did   = self.did
		if sid   == None : sid   = self.sid
		if daddr == None : daddr = self.daddr
		if saddr == None : saddr = self.saddr
		if type  == None : type  = self.type


	def post(self, daddr=None, saddr=None, did=None, sid=None, type=None, data=None):
		"""
		Sends a message. Unspecified fields will be sent using the 
		defaults set when instantiating sossrv or by use of 
		set_message_defaults() method.

		The data argument must be a str (use the pack() method to 
		build strings from tuples).
		"""

		if did   == None : did   = self.did
		if sid   == None : sid   = self.sid
		if daddr == None : daddr = self.daddr
		if saddr == None : saddr = self.saddr
		if type  == None : type  = self.type
		if data  == None : data  = ''
		
		self._sock.send(pack('<BBHHBB', did, sid, daddr, saddr, type, len(data)) + data) 				


	def post_msg(self, msg):
			"""
			Sends a message dictionary, which can be produced by
			sossrv.msg()
			"""
			self._sock.send(pack('<BBHHBB', msg['did'] , msg['sid'], msg['daddr'],
			                msg['saddr']  , msg['type'], msg['len']) + msg['data'])


	def post_net(self, did, sid, type, length, data, flags, daddr):
		"""
		Sends a message where saddr and sid are the ones set when 
		initializing this instance or with set_msg_defaults().
		If none was specified in either of those times, saddr (nid) 
		defaults to 0xFFFE, and sid (pid) defaults to 128.

		The data argument must be a str (use the pack() method to 
		build strings from tuples).

		The flags argument is currently not used, and is only there
		for keeping the same syntax as SOS.
		"""

		if daddr == None: daddr = BCAST_ADDRESS
		self._sock.send(pack('<BBHHBB', did, sid, daddr, self.saddr, type, length) + data)


	# The r's are for "reply" to distinguish the trigger from the outgoing message
	def post_rpc(self, did    = None, sid    = None, daddr  = None, 
	                   saddr  = None, type   = None, data   = '',
	                   rdid   = None, rsid   = None, rdaddr = None, 
	                   rsaddr = None, rtype  = None,
	                   timeout = None, nreplies = 1, **etc):
		"""
		post_rpc(did    = 0x97, sid    = None, daddr  = None, 
		         saddr  = None, type   = None, data   = '',
		         rdid   = None, rsid   = None, rdaddr = None, 
		         rsaddr = None, rtype  = None,
				 timeout = 3, nreplies = 5)

		returns [msg0, msg1, ...]

		This method allows you to send a message and wait for the
		replies. The parameters starting with the letter 'r' describe
		what the expected reply looks like. The timeout parameter is 
		required when nreplies != 1.

		Although this method has an aweful lot of parameters, in 
		real-world scenarios only a handful will be used at a time.
		"""

		# We do the post_net inside the condition lock to prevent the (unlikely) race condition
		# that the packet comes back before we get a chance to read the reply packet

		if not self._listening:
			raise Exception, 'The sossrv instance is disconnected.'

		if nreplies > 1 and (not timeout):
			raise Exception, 'You should provide a timeout if you want multiple replies.'

		# Only one thread can be synchronously listening at a time
		assert self._syncTrig == None and self._syncPkt == [] 
		
		ret = []
		
		self._syncCV.acquire()
		self._syncTrig = (rdid, rsid, rdaddr, rsaddr, rtype, None)  # make a trigger packet

		self.post(daddr=daddr, saddr=saddr, did=did, sid=sid, type=type, data=data)

		while nreplies > 0:
			self._syncCV.wait(timeout)

			if len(self._syncPkt):
				# this is None on timeout or error. (hdr, data) otherwise
				nreplies -= len(self._syncPkt)
				msgdict = [_make_msg_dict(*pkt) for pkt in self._syncPkt]
				ret.extend(msgdict)
				self._syncPkt = []
			else: break

		self._syncTrig = None
		self._syncCV.release()

		return ret		
				

	def post_rpc_msg(self, msgdict,
	                 rdid   = None, rsid  = None, rdaddr  = None, 
	                 rsaddr = None, rtype = None, timeout = None, 
					 nreplies = 1):
		"""
		post_rpc_msg(msgdict,
	                 rdid   = None, rsid  = None, rdaddr  = None, 
	                 rsaddr = None, rtype = None, timeout = None,
					 nreplies = 1)

		returns [msg0, msg1, ...]

		This method allows you to send a message and wait for the
		replies. The parameters starting with the letter 'r' describe
		what the expected reply looks like. The timeout parameter is 
		required when nreplies != 1.

		msgdict is a dictionary rescribing the message and can be
		created with the msg() method.
		"""

		return self.post_rpc(
				did    = msgdict['did'], 
				sid    = msgdict['sid'],
				daddr  = msgdict['daddr'], 
				saddr  = msgdict['saddr'],
				type   = msgdict['type'],
				data   = msgdict['data'],
				rdid   = rdid,   rsid  = rsid,  rdaddr  = rdaddr, 
				rsaddr = rsaddr, rtype = rtype, timeout = timeout,
				nreplies = nreplies)


	def listen(self, did=None, sid=None, daddr=None, saddr=None, type=None, timeout=None, nreplies=1):
		"""
		listen(did=None, sid=None, daddr=None, saddr=None,
		       type=None, timeout=None, nreplies=1) 
			   
		returns [msg0, msg1, ...]

		This is a blocking method that returns the first matching message.
		Timeout is specified in floating point seconds.
		"""

		t = (did, sid, daddr, saddr, type)

		if not self._listening:
			raise Exception, 'The sossrv instance is disconnected.?'

		if nreplies > 1 and (not timeout):
			raise Exception, 'You should provide a timeout if you want multiple replies.'

		# Only one thread can be synchronously listening at a time
		assert self._syncTrig == None and self._syncPkt == []
		
		ret = []

		self._syncCV.acquire()
		self._syncTrig = t
		
		while nreplies > 0:
			self._syncCV.wait(timeout)
			nreplies -= len(self._syncPkt)
			if len(self._syncPkt):
				# this is None on timeout or error. (hdr, data) otherwise
				msgdict = [_make_msg_dict(*pkt) for pkt in self._syncPkt]
				ret.extend(msgdict)
				self._syncPkt = []

			else: break

		self._syncTrig = None
		self._syncCV.release()

		return ret
                
	
	def register_trigger(self, func, did=None, sid=None, daddr=None, saddr=None, type=None):
		"""
		Register a trigger (i.e. listener) to execute 'func(hdr, data)' upon
		receiving a matching message.'.

		NOTE: Because of the way Python works, if you change a function *after* registering 
		it, you must deregister then re-register it!
		"""
		
		self._trgLock.acquire()
		self._triggers.append((did, sid, daddr, saddr, type, func))
		self._trgLock.release()


	def deregister_trigger(self, func=None, did=None, sid=None, daddr=None, saddr=None, type=None):
		"""
		Deregisters ALL matching triggers, so if no arguments are provided
		all triggers are deregistered. 
		"""

		pattern = (did, sid, daddr, saddr, type, func)
		newtriggers = []

		self._trgLock.acquire()

		for t in self._triggers:
			if not _match_trigger(t, pattern): newtriggers.append(t)

		self._triggers = newtriggers
		self._trgLock.release()


	def msg(self, daddr=None, saddr=None, did=None, sid=None, type=None, data=''):
		"""
		Returns a properly-formatted message dictionary with default values 
		for any parameters not provided. Defaults can be set with set_defaults().
		"""

		if did == None   : did   = self.did
		if sid == None   : sid   = self.sid
		if daddr == None : daddr = self.daddr
		if saddr == None : saddr = self.saddr
		if type == None  : type  = self.type

		return {'did'   : did,     'sid'   : sid,  'daddr'  : daddr,
		        'saddr' : saddr,   'type'  : type, 'length' : len(data),
		        'data'  : data,    'flags' : 0}


	def _connect(self, host=None, port=None, retries=0):
		"""
		Internal: connects to the sossrv
		"""

		if host == None: host = self.host
		else: self.host = host
		
		if port == None: port = self.port
		else: self.port = port
		
		self.retries = retries

		self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		
		while retries >= 0:
			try:
				self._sock.connect((host, port))
				retries = -1
			except Exception, e:
				if retries <= 0: raise IOError, "Can't connect to SOS server: " + str(e)
				retries -= 1
				continue

		self._start_listening()



	def _start_listening(self):
		"""
		Internal: Starts a thread to listen to all incomming messages. This must 
		be called in order to receive any data.
		"""
		if (self._listenthread == None) or (not self._listenthread.isAlive()):
			self._listenthread = Thread(name='SOSSRVpktrecv', target=self._listen_for_triggers)
			self._listenthread.setDaemon(True)
			self._listenthread.start()


	def _receive_message_tuple(self):
		"""
		Internal: Returns the next packet received by the SOS server as a header
		tuple and data str. Use the unpack() method to cast the data 
		into the required format.
		"""
		
		hdrstr = self._leftoverbytes
		tmpbuf = ''
		
		while len(hdrstr) < self.SOS_HEADER_SIZE :
			tmpbuf = self._sock.recv(self.SOS_HEADER_SIZE - len(tmpbuf))
			if tmpbuf != '' : hdrstr += tmpbuf
			else : raise socket.error, 'Socket reset by peer.'
		
		# we do this in case we get a bit of the data string along with hdrstr
		hdrstr = hdrstr[:self.SOS_HEADER_SIZE]
		data   = hdrstr[self.SOS_HEADER_SIZE:]
		
		hdr = unpack('<BBHHBB', hdrstr)
		
		datalen = hdr[5]
		
		while len(data) < datalen:
			tmpbuf = self._sock.recv(datalen - len(data))
			if tmpbuf == '' : raise socket.error, 'Socket reset by peer.'
			data += tmpbuf 

		data = data[:datalen]
		self._leftoverbytes = data[datalen:]


		return hdr, data


	def _listen_for_triggers(self):
		"""
		Internal listener thread procedure.
		"""

		self._listening = True
		self._leftoverbytes = ''

		while self._listening:

			try: hdr, data = self._receive_message_tuple()

			except socket.error:
				if self.verbose: print('Connection to sossrv at %s:%d broken.' % (self.host, self.port))
				break

			self._trgLock.acquire()

			for t in self._triggers:
				if _match_packet(hdr, t):
					t[5](_make_msg_dict(hdr, data))

			self._trgLock.release()
			self._syncCV.acquire()

			if self._syncTrig != None:
				if _match_packet(hdr, self._syncTrig):
					self._syncPkt.append((hdr, data))
					self._syncCV.notify()

			self._syncCV.release()

		self._listening = False


def _make_msg_dict(hdr, data):
	"""
	Gets the header and data string as a dictionary for easier display and parsing.
	"""
	return {'did'   : hdr[0], 'sid'  : hdr[1], 'daddr'  : hdr[2],
	        'saddr' : hdr[3], 'type' : hdr[4], 'length' : hdr[5],
	        'data'  : data}


def _match_packet(hdr, pattern):
	"""
	Returns True if the hdr matches the pattern.
	"""
	for i in xrange(5):
		if (pattern[i]) and (hdr[i] != pattern[i]): return False

	return True


def _match_trigger(trigger, pattern):
	"""
	Returns True if the trigger matches the pattern.
	"""
	for i in xrange(6):
		if (pattern[i]) and (trigger[i] != pattern[i]): return False

	return True

