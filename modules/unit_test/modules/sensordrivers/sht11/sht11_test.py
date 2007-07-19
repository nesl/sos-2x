#!/usr/bin/env python

import pysos
    
SHT11_TEMPERATURE = 34
SHT11_HUMIDITY = 35
SHT11_MOD = 130

def sht11_temperature(msg):
    # Note that pysos.unpack returns a tuple.  The comma after the lval
    # notes that we want to unpack the singleton tuple.
    temperature, = pysos.unpack('<H', msg['data'])
    cal_temp = -39.60 + 0.01 * temperature 
    print("Node %d has temperature %f" % (msg['saddr'], cal_temp))
    return


def sht11_humidity(msg):
    # Note that pysos.unpack returns a tuple.  The comma after the lval
    # notes that we want to unpack the singleton tuple.
    humidity, = pysos.unpack('<H', msg['data'])
    cal_hum = -4 + 0.0405 * humidity + -2.8 * 10**(-6) * (humidity ** 2)
    print("Node %d has humidity %f" % (msg['saddr'], cal_hum))
    return


if __name__ == "__main__":

    srv = pysos.sossrv()
    #msg = srv.listen()

    srv.register_trigger(sht11_temperature, did=SHT11_MOD, type=SHT11_TEMPERATURE)
    srv.register_trigger(sht11_humidity, did=SHT11_MOD, type=SHT11_HUMIDITY)


    raw_input("Press Any Key To Quit\n")
    
