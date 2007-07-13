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
tests_to_run = 'test.lst'

class Test:
    ''' a small object to hold all the important information regarding a test
    '''
    def __init__(self, n, d, dl, t, tl, dur, dep):
	self.name = n
	self.driver_name = d
	self.driver_location = dl
	self.test_name = t
	self.test_location = tl
	self.time = dur * 60 
	self.dep_list = dep

class Dependency:

    def __init__(self):
	self.name = ''
	self.source = ''
	self.sub_dep = []


def run_and_redirect(run_cmd, outfile):
    ''' redirect the output to outfile, if a file is specified
        and create change the programming running to the one specified in run_cmd
	if for some reason that returns, we exit with status 1
	'''
    if outfile != '':
	out = open(outfile, 'w')
	out2 = os.dup(out.fileno())
	os.dup2(out.fileno(), 1)
	os.dup2(out2, 2)

    os.execvp(run_cmd[0], run_cmd)
    os._exit(1)

def clean(dest):
    ''' call make clean on the specified directory 
    '''
    null = open("/dev/null", "w")
    cmd_clean = ['make', '-C', dest, 'clean']
    subprocess.call(cmd_clean, stdout=null)
       
def configure_setup():
    ''' read the config.sys file to set up the hardware and envrioment variables correctly
        a number of options can be set, and is detailed in the README file
	'''
    config_f = open('config.sys', 'r')

    global listen_port
    global install_port
    global SOS_GROUP
    global prog
    global number_of_nodes
    global number_of_prog
    global tests_to_run

    home = '/home/test'
    sos_root = home + '/sos-2x/trunk'
    sos_tool_dir = home + '/local'
    test_dir = sos_root + '/modules/unit_test'

    for line in config_f:
	words = re.match(r'test_list = (\S+)\n', line)
	if words:
	    tests_to_run = words.group(1)
	    continue
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
    
def gather_dependencies(dep_list_name):
    dep_f = open(dep_list_name, 'r')

    current_name = ''
    current_dep = Dependency()
    dep_dict = {}

    for line in dep_f:
	if line[0] == '#':
	    if current_name != '':
		dep_dict[current_name] = current_dep
	    current_dep = Dependency()
	    current_dep.name = line[1:-1]
     	    current_name = line[1:-1]
	elif line[0] == '/':
	    current_dep.source = line[:-1]
	else:
	    current_dep.sub_dep.append(line[:-1])

    if current_name != '':
	dep_dict[current_name] = current_dep
	
    return dep_dict

def configure_tests(test_list_name):
    ''' read the test.lst file to build our set of tests.
        each test should conform to the following layout:
	  test name
	  sensor driver file name
	  sensor driver location
	  test driver file name
	  test driver location
	  amount of time you want the test script to run
        '''
    test_f = open(test_list_name, "r")
    
    name = ''
    driver_name = ''
    driver_location = ''
    test_name = ''
    test_location = ''
    time = 0
    dep_list = []
    test_list = []

    line = test_f.readline()
    while (line != ''):
        if line[0] == '#':
	    if name != '': 
	        new_test = Test(name, driver_name, driver_location, test_name, test_location, time, dep_list)
	        test_list.append(new_test)
		dep_list = []
	    name = line[:-1]
	    driver_name = test_f.readline()[:-1]
	    driver_location = test_f.readline()[:-1]
	    test_name = test_f.readline()[:-1]
	    test_location = test_f.readline()[:-1]
	    time = int(test_f.readline()[:-1])
	else:
	    dep_list.append(line[:-1])
	line = test_f.readline()

    if name != '':
	new_test = Test(name, driver_name, driver_location, test_name, test_location, time, dep_list)
	test_list.append(new_test)

    return test_list

def make_kernel(platform):
    ''' attempts to compiler the kernel for the given platform.  either micaz, mica2, or avrora.
        if there are any comipilation issues, it informs the user and exits
	all output from compiliation is saved in $SOSROOT/modules/unit_test/python/kernel.log
	'''
    kernel_f = open(os.environ['SOSTESTDIR'] + "/../python/kernel.log", "w")

    clean("config/blank")
    if platform == 0:
        cmd_make = ["make", "-C", "config/blank", "micaz"]
    elif platform == 1:
        cmd_make = ["make", "-C", "config/blank", "mica2"]
    elif platform == 2:
        cmd_make =  ["make", "-C", "config/blank", "avrora"]

    try:
      subprocess.check_call(cmd_make, stderr=kernel_f, stdout=kernel_f)
    except subprocess.CalledProcessError:
      print "compiling the kernel ran into some issues, please check the kernel.log file to see the error"
      sys.exit(1)

    time.sleep(5)

def install_on_mica(platform, address, port):
    ''' assuming the kernel has been properly compiled, this will load a blank kernel onto the programming board
        specified by the value in install_port[port].  the group id of the node will be sos_group, and the address
	of the port is of course port.
	if the installation fails for any reason, a message is displayed to the user, urging them to check the connection
	and then retry installing the kernel again.
	'''

    if platform == 0:
        cmd_install = ["make", "-C", "config/blank", "install", "PROG=" + prog, "PORT=" + install_port[port], "SOS_GROUP=" + sos_group, "ADDRESS=" + str(address)] 
    elif platform == 1:
	cmd_install = ["make", "-C", "config/blank", "install", "PROG=" + prog, "PORT=" + install_port[port], "SOS_GROUP=" + sos_group, "ADDRESS=" + str(address)]
    else:
        print "you shouldn't be doing this"
	os.exit(0)
    
    try:
      subprocess.check_call(cmd_install)
    except subprocess.CalledProcessError:
	print "problem installing on board: %s, please be sure that the board is connected propperly" %install_port[port]
	print "please press any key when ready"

	raw_input()
	install_on_mica(platform, address, port)


def install_on_avrora(node_count):
    ''' this will start up avrora as it's own child process with the options specified below.
        and it will install a blank kernel to run. '''

    cmd_install = ["java", "-server", "avrora/Main", "-banner-false", "-colors=true", "-platform=mica2", "-simulation=sensor-network", "-monitors=serial,real-time", "-sections=.data,.text, .sos_bls", "-update-node-id", "-nodecount="+ str(node_count), os.environ['SOSROOT']+ "/config/blank/blank.od"]
    os.chdir(os.environ['SOSROOT'] + '/avrora/bin')
    ret = os.fork()
    if ret == 0:
	run_and_redirect(cmd_install, os.environ['SOSTESTDIR'] + '/../python/avrora.log')
    else:
	return ret
    

def run_sossrv(target):
    ''' start up sossrv.  for targets mica2 or micaz, it will run sossrv on the listen port specified.  
        for avrora taget, it will set up on localhost:2390, which is the port avrora expects on default
	output from sossrv will be directed to $SOSROOT/modules/unit_test/python/sossrv.log '''
    if target == 0 or target == 1:
	cmd_run = ['sossrv.exe', '-s', listen_port]
    elif target == 2:
	cmd_run = ['sossrv.exe', '-n', '127.0.0.1:2390']

    print "starting sossrv"
    print cmd_run
    time.sleep(10)

    ret = os.fork()
    if ret == 0:
	run_and_redirect(cmd_run, os.environ['SOSTESTDIR'] + '/../python/sossrv.log')

    time.sleep(10)
    return ret

def install_dependency(dep_list, dep_dict, target):
    if len(dep_list) == 0:
	return
    if len(dep_dict) == 0:
	return

    if target == 0:
	plat = 'micaz'
    elif target == 1:
	plat = 'mica2'

    for dep in dep_list:
	print "installing dependency: %s" %dep
	current_dep = dep_dict[dep]

	if len(current_dep.sub_dep) > 0:
	    install_dependency(current_dep.sub_dep, dep_dict,target)
	 
	 
        cmd_make = ['make', '-C', os.environ['SOSROOT'] + current_dep.source, plat]
	cmd_install = ['sos_tool.exe', '--insmod=' + os.environ['SOSROOT'] + current_dep.source+'/' + current_dep.name + '.mlf']

	print cmd_make
	print cmd_install
	clean(os.environ['SOSROOT'] + current_dep.source)
        subprocess.call(cmd_make)
        subprocess.call(cmd_install)
	time.sleep(5)

def run_tests(test_list, target, dep_dict):
    ''' given a list of tests, and the target node, compile, dynamically load, and test the output for each
        test.  before each test, all modules will be removed from the network to prevent any conflicts.
	Also, the python test script, which is used to verify the output of the tests, and runs that tests 
	for the number of minutes specified by each test
	the standard output and error for the python script will be saved in in a log file located in the test
	driver's location
	'''
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
	
        #first install all the depenedencies
	install_dependency(test.dep_list, dep_dict,target)

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

    dep_dict = gather_dependencies("depend.lst")

    test_list = configure_tests(tests_to_run)
    
    os.chdir(os.environ['SOSROOT'])
    
    #get the arguement for the node type, if not available, assume it is for micaz
    if (len(sys.argv) == 2):
	target = int(sys.argv[1])
    else:
	target = 0

    if target > 2:
	print "invalid target type, exiting"
	os.exit(0)
    
    make_kernel(target)

    if target == 2:
	#install on avrora with the approriate number of nodes
	avrora_child = install_on_avrora(number_of_nodes)
    elif number_of_prog == 1: 
	# installing several nodes via the same programming board
	# it will install one node at a time, and wait for the user to switch nodes before continuing
	print "installing several nodes via the same board, please pay attention"
	while (number_of_nodes > 1):
	    #install_on_mica(target, number_of_nodes - 1, 0)

	    print "this current nodes address is: %d" %(number_of_nodes - 1)
	    print "please remove the current node and place another on the programming board"
	    print "press any key when ready to install on the next node"
	    
	    raw_input()

	    number_of_nodes -= 1

	#install_on_mica(target, 0, 0)
	print "this is the base station node, please leave it connected to the programming board"
    else:
	# installing on several programming boards, serially
	# we assume that for each port assigned, there is a programming board and node connected
	print "installing through multiple programming boards, your reaction is not required"

	while (number_of_prog > 0):
	    #install_on_mica(target, number_of_prog -1, number_of_prog-1)

	    print "this nodes address is: %d" %(number_of_prog-1)
	    print "the next node will be installed automatically"

	    number_of_prog -= 1
	    
    sos_child = run_sossrv(target)

    run_tests(test_list, target, dep_dict)

    # killing any child processes that are running
    if (sos_child > 0):
        print "killing sossrv"
        os.kill(sos_child, signal.SIGTERM)
    if (avrora_child > 0):
	print "killing avrora"
	os.kill(avrora_child, signal.SIGTERM)

    os.waitpid(avrora_child, 0)
    print "test suite complete"
