import pysos

def func(msg):
    (addr, t1, t2) = pysos.unpack('<HLL', msg['data'])
    print "addr: ", addr, "time: ", t1, "refreshed: ", t2

srv = pysos.sossrv()

data = pysos.pack('<B', 0)
srv.post(daddr = 0xFFFF, saddr = 7000, did = 170,
        sid = 170, type = 35, data = data)

srv.register_trigger(func, type=43)
raw_input("Hit enter to finish")
