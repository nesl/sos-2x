#!/usr/bin/env python

import pysos
    
SHT15_TEMPERATURE = 34
SHT15_HUMIDITY = 35

def sht15_temperature(msg):
    # Note that pysos.unpack returns a tuple.  The comma after the lval
    # notes that we want to unpack the singleton tuple.
    temperature, = pysos.unpack('>h', msg['data'])
    cal_temp = -39.60 + 0.01 * temperature 
    print("Node %d has temperature %f" % (msg['sid'], cal_temp))
    return


def sht15_humidity(msg):
    # Note that pysos.unpack returns a tuple.  The comma after the lval
    # notes that we want to unpack the singleton tuple.
    humidity, = pysos.unpack('>h', msg['data'])
    cal_hum = -4 + 0.0405 * humidity + -2.8 * 10**(-6) * (humidity ** 2)
    print("Node %d has humidity %f" % (msg['sid'], cal_hum))
    return


if __name__ == "__main__":

    srv = pysos.sossrv()
    msg = srv.listen()

    srv.register_trigger(sht15_temperature, type=SHT15_TEMPERATURE)
    srv.register_trigger(sht15_humidity, type=SHT15_HUMIDITY)


    raw_input("Press Any Key To Quit\n")
    
