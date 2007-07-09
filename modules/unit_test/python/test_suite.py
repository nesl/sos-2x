#!/usr/bin/python

import signal
import time
import os
import sys
import subprocess
import re

global number_of_nodes
prog = 'mib510'
install_port = ['/dev/ttyUSB0']
listen_port = '/dev/ttyUSB1'
sos_group = '0'
number_of_nodes = 1
number_of_prog = 1

class Test:
    def __init__(self, n, d, dl, t, tl, dur):
	self.name = n
	self.driver_name = d
	self.driver_location = dl
	self.test_name = t
	self.test_location = tl
	self.time = dur * 60 

def run_and_redirect(run_cmd, outfile):
    if outfile != '':
	out = open(outfile, 'w')
	out2 = os.dup(out.fileno())
	os.dup2(out.fileno(), 1)
	os.dup2(out2, 2)

    os.execvp(run_cmd[0], run_cmd)
    os._exit(1)

def clean(dest):
    null = open("/dev/null", "w")
    cmd_clean = ['make', '-C', dest, 'clean']
    subprocess.call(cmd_clean, stdout=null)
       
def configure_setup():
    config_f = open('config.sys', 'r')

    global listen_port
    global install_port
    global SOS_GROUP
    global prog
    global number_of_nodes
    global number_of_prog

    home = '/home/test'
    sos_root = home + '/sos-2x/trunk'
    sos_tool_dir = home + '/local'
    test_dir = sos_root + '/modules/unit_test'

    for line in config_f:
	words = re.match(r'number_of_nodes = (\d+)\n', line)
	if words:
            number_of_nodes = int( words.group(1) )
	    continue
	words = re.match(r'boards = (\d+)\n', line)
	if words:
	    number_of_prog = int( words.group(1) )
	    continue
	words = re.match(r'HOME = (\S+)\n', line)
	if words:
	    home = words.group(1)
	    continue
	words = re.match(r'SOSROOT = (\S+)\n', line)
	if words:
	    sos_root = words.group(1)
	    continue
        words = re.match(r'SOSTOOLDIR = (\S+)\n', line)
        if words:
	    sos_tool_dir = words.group(1)
            continue
        words = re.match(r'TESTS = (\S+)\n', line)
        if words:
	    test_dir = words.group(1)
	    continue
        words = re.match(r'PROG = (\S+)\n', line)
        if words:
	    prog = words.group(1)
	    continue
        words = re.match(r'install_port = (\S+)\n', line)
        if words:
	    install_port[0] = words.group(1)
	    continue
	words = re.match(r'install_port(\d+) = (\S+)\n', line)
	if words:
	    install_port.append(words.group(2))
	    continue
        words = re.match(r'listen_port = (\S+)\n', line)
        if words:
	    listen_port = words.group(1)
	    continue
        words = re.match(r'SOS_GROUP = (\d+)\n', line)
        if words:
	    sos_group = words.group(1)
	    continue

    number_of_prog = len(install_port)
    os.environ['SOSROOT'] = home + sos_root
    os.environ['SOSTOOLDIR'] = home + sos_tool_dir
    os.environ['SOSTESTDIR'] = home + sos_root + test_dir

def configure_tests():
    test_f = open("test.lst", "r")
    
    test_list = []
    line = test_f.readline()
    while (line != ''):
        if line[0] == '#':
	    driver_name = test_f.readline()[:-1]
	    driver_location = test_f.readline()[:-1]
	    test_name = test_f.readline()[:-1]
	    test_location = test_f.readline()[:-1]
	    time = int(test_f.readline()[:-1])
	    new_test = Test(line, driver_name, driver_location, test_name, test_location, time)
	    test_list.append(new_test)
	line = test_f.readline()

    return test_list

def make_kernel(platform):
    null = open("/dev/null", "w")

    clean("config/blank")
    if platform == 0:
        cmd_make = ["make", "-C", "config/blank", "micaz"]
    elif platform == 1:
        cmd_make = ["make", "-C", "config/blank", "mica2"]
    elif platform == 2:
        cmd_make =  ["make", "-C", "config/blank", "avrora"]

    subprocess.call(cmd_make, stdout=null)
    time.sleep(5)

def install_on_mica(platform, address, port):

    if platform == 0:
        cmd_install = ["make", "-C", "config/blank", "install", "PROG=" + prog, "PORT=" + install_port[port], "SOS_GROUP=" + sos_group, "ADDRESS=" + str(address)] 
        subprocess.call(cmd_install)
    elif platform == 1:
	cmd_install = ["make", "-C", "config/blank", "install", "PROG=" + prog, "PORT=" + install_port[port], "SOS_GROUP=" + sos_group, "ADDRESS=" + str(address)]
	subprocess.call(cmd_install)
    else:
        print "you shouldn't be doing this"
	os.exit(0)

def install_on_avrora(node_count):

    cmd_install = ["java", "-server", "avrora/Main", "-banner-false", "-colors=true", "-platform=mica2", "-simulation=sensor-network", "-monitors=serial,real-time", "-sections=.data,.text, .sos_bls", "-update-node-id", "-nodecount="+ str(node_count), os.environ['SOSROOT']+ "/config/blank/blank.od"]
    os.chdir(os.environ['SOSROOT'] + '/avrora/bin')
    ret = os.fork()
    if ret == 0:
	run_and_redirect(cmd_install, os.environ['SOSTESTDIR'] + '/../python/avrora.log')
    else:
	return ret
    

def run_sossrv(target):
    if target == 0 or target == 1:
	cmd_run = ['sossrv.exe', '-s', listen_port]
    elif target == 2:
	cmd_run = ['sossrv.exe', '-n', '127.0.0.1:2390']

    print "starting sossrv"
    time.sleep(10)

    ret = os.fork()
    if ret == 0:
	run_and_redirect(cmd_run, os.environ['SOSTESTDIR'] + '/../python/sossrv.log')

    time.sleep(10)
    return ret


def run_tests(test_list, target):
    if target == 0:
	platform = 'micaz'
    elif target == 1 or target == 2:
	platform = 'mica2'

    print "starting tests"

    cmd_erase = ["sos_tool.exe", "--rmmod=0"]
    for test in test_list:
	subprocess.call(cmd_erase)

	print "running test" + test.name
        driver_location = os.environ['SOSROOT'] + test.driver_location
	test_location = os.environ['SOSTESTDIR'] + test.test_location
	
	#first install the sensor driver
        cmd_make = ["make", "-C",driver_location, platform]
	cmd_install = ["sos_tool.exe", "--insmod=" + driver_location +'/' + test.driver_name + ".mlf"]
	clean(driver_location)
	subprocess.call(cmd_make)
	subprocess.call(cmd_install)
	time.sleep(5)

        # next install the test driver for the sensor
	cmd_make = ['make', '-C', test_location, platform]
	cmd_install = ['sos_tool.exe', '--insmod=' + test_location + '/' + test.test_name + '.mlf']
	clean(test_location)
	subprocess.call(cmd_make)
	subprocess.call(cmd_install)

        #now run the python script to verify the output 
	# and wait for the specified amount of time
	child = os.fork()
	if child == 0:
	    print "running python test"
	    cmd_test = ['python', test_location + '/' + test.test_name + '.py']
	    run_and_redirect(cmd_test, '')
	else:
	    time.sleep(test.time)
	    os.kill(child, signal.SIGKILL)
	    os.waitpid(child, 0)

if __name__ == '__main__':
    avrora_child = 0
    sossrv_child = 0
    
    configure_setup()

    print number_of_nodes
    test_list = configure_tests()
    
    os.chdir(os.environ['SOSROOT'])
    if (len(sys.argv) == 2):
	target = int(sys.argv[1])
    else:
	target = 0

    if target > 2:
	print "invalid target type, exiting"
	os.exit(0)
    
    make_kernel(target)

    if target == 2:
	avrora_child = install_on_avrora(number_of_nodes)
    elif number_of_prog == 1: 
	print "installing several nodes via the same board, please pay attention"
	while (number_of_nodes > 1):
	    install_on_mica(target, number_of_nodes - 1, 0)

	    print "this current nodes address is: " + str(number_of_nodes - 1)
	    print "please remove the current node and place another on the programming board"
	    print "press any key when ready to install on the next node"
	    
	    raw_input()

	    number_of_nodes -= 1

	install_on_mica(target, 0, 0)
	print "this is the base station node, please leave it connected to the programming board"
    else:
	print "installing through multiple programming boards, your reaction is not required"

	while (number_of_prog > 0):
	    install_on_mica(target, number_of_prog -1, number_of_prog-1)

	    print "this nodes address is: " + str(number_of_prog-1)
	    print "the next node will be installed automatically"

	    number_of_prog -= 1
	    
    sos_child = run_sossrv(target)

    run_tests(test_list, target)

    if (sos_child > 0):
        print "killing sossrv"
        os.kill(sos_child, signal.SIGTERM)
    if (avrora_child > 0):
	print "killing avrora"
	os.kill(avrora_child, signal.SIGTERM)

    os.waitpid(avrora_child, 0)
    print "test suite complete"
