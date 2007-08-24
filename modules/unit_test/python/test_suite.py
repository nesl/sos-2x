#!/usr/bin/python
import getopt
import signal
import time
import os
import sys
import subprocess
import re
import stat

INF = -1.0
prog = 'mib510'
install_port = ['/dev/ttyUSB0']
listen_port = '/dev/ttyUSB1'
sos_group = '0'
number_of_nodes = 1
number_of_prog = 1
tests_to_run = 'test.conf'
depend_list = 'depend.conf'
kernel_mode = ''

class Test:
    ''' a small object to hold all the important information regarding a test
    '''
    def __init__(self, test_list, dep):
	self.name = test_list[0]
	self.driver_name = test_list[1]
	self.driver_location = test_list[2]
	self.test_name = test_list[3]
	self.test_location = test_list[4]
	if test_list[5] == 'inf':
	    self.time = INF
	else:
	    try: 
		self.time = float(test_list[5]) *60
		if self.time < 0.0:
		    self.time = 0.0
	    except ValueError:
		self.time = 0.0 
	self.dep_list = dep

    def to_string(self):
	result = "%s\n%s\n%s\n%s\n%s\n" %(self.name, self.driver_name, self.driver_location, 
		                              self.test_name, self.test_location)
	if self.time == INF:
	    result += "INF\n"
	else:
	    result += "%f\n" %(self.time/60)

	for dep in self.dep_list:
	    result += "%s\n" %(dep)

	return result
    
    def check_dir(self, loc):
	file_dirs = loc.split('/')
	for dir in file_dirs:
	    if dir == '':
		continue
	    try:
		os.listdir(dir)
	    except OSError:
		return False
	    os.chdir(dir)

        return True 

    def is_valid(self):
	ret = True

	old_cwd = os.getcwd()

	os.chdir(os.environ['SOSROOT'])

        if self.check_dir(self.driver_location) == False:
	    print "driver location %s is invalid please check it" %self.driver_location
	    ret = False
        file_list = os.listdir(os.getcwd())
	if "%s.c" %self.driver_name not in file_list or "Makefile" not in file_list:
	    ret = False

	os.chdir(os.environ['SOSTESTDIR'])
	if self.check_dir(self.test_location) == False:
	    print "test location %s is invalid, please check it" %self.test_location
	    ret = False
	file_list = os.listdir(os.getcwd())
	if "%s.c" %self.test_name not in file_list or "%s.py" %self.test_name not in file_list or "Makefile" not in file_list:
	    ret = False

        print "Test %s is valid" %self.name
	os.chdir(old_cwd)
	return ret

class Dependency:

    def __init__(self):
	self.name = ''
	self.source = ''
	self.sub_dep = []


def run_and_redirect(run_cmd, outfile='/dev/null', error='/dev/null'):
    ''' redirect the output to outfile, if a file is specified
        and create change the programming running to the one specified in run_cmd
	if for some reason that returns, we exit with status 1
	'''
    if outfile != '':
	out = open(outfile, 'w')
	os.dup2(out.fileno(), 1)

    err_f = open(error, 'w')
    os.dup2(err_f.fileno(), 2)

    os.execvp(run_cmd[0], run_cmd)
    os._exit(1)

def clean(dest):
    ''' call make clean on the specified directory 
    '''
    null = open("/dev/null", "w")
    cmd_clean = ['make', '-C', dest, 'clean']
    subprocess.call(cmd_clean, stdout=null)
       
def configure_setup(config_file='config.sys'):
    ''' read the config.sys file to set up the hardware and envrioment variables correctly
        a number of options can be set, and is detailed in the README file
	'''
    config_f = open(config_file, 'r')

    global prog
    global install_port
    global listen_port
    global sos_group
    global number_of_nodes
    global number_of_prog
    global tests_to_run
    global depend_list
    global kernel_mode

    home = '/home/test'
    sos_root = home + '/sos-2x/trunk'
    sos_tool_dir = home + '/local'
    test_dir = sos_root + '/modules/unit_test'

    for line in config_f:
	words = re.match(r'test_list = (\S+)(\s*)\n', line)
	if words:
	    tests_to_run = words.group(1)
	    continue
	words = re.match(r'depend_list= (\S+)(\s*)\n', line)
	if words:
	    depend_list = words.group(1)
	    continue
	words = re.match(r'number_of_nodes = (\d+)(\s*)\n', line)
	if words:
            number_of_nodes = int( words.group(1) )
	    continue
	words = re.match(r'boards = (\d+)(\s*)\n', line)
	if words:
	    number_of_prog = int( words.group(1) )
	    continue
	words = re.match(r'HOME = (\S+)(\s*)\n', line)
	if words:
	    home = words.group(1)
	    continue
	words = re.match(r'SOSROOT = HOME(\S+)(\s*)\n', line)
	if words:
	    sos_root = words.group(1)
	    continue
        words = re.match(r'SOSTOOLDIR = HOME(\S+)(\s*)\n', line)
        if words:
	    sos_tool_dir = words.group(1)
            continue
        words = re.match(r'TESTS = SOSROOT(\S+)(\s*)\n', line)
        if words:
	    test_dir = words.group(1)
	    continue
        words = re.match(r'PROG = (\S+)(\s*)\n', line)
        if words:
	    prog = words.group(1)
	    continue
        words = re.match(r'install_port = (\S+)(\s*)\n', line)
        if words:
	    install_port[0] = words.group(1)
	    continue
	words = re.match(r'install_port(\d+) = (\S+)(\s*)\n', line)
	if words:
	    install_port.append(words.group(2))
	    continue
        words = re.match(r'listen_port = (\S+)(\s*)\n', line)
        if words:
	    listen_port = words.group(1)
	    continue
        words = re.match(r'SOS_GROUP = (\S+)(\s*)\n', line)
        if words:
	    sos_group = words.group(1)
	    continue
	words = re.match(r'kernel_mode = (\S+)(\s*)\n', line)
	if words:
	    kernel_mode = words.group(1)
	    continue

    number_of_prog = len(install_port)
    os.environ['SOSROOT'] = home + sos_root
    os.environ['SOSTOOLDIR'] = home + sos_tool_dir
    os.environ['SOSTESTDIR'] = home + sos_root + test_dir
    
def gather_dependencies(dep_list_name):
    ''' build the list of dependencies based off of the specified file
    	each dependency has a name, source location, and the any other sub dependencies
	'''

    dep_f = open(dep_list_name, 'r')

    current_name = ''
    current_dep = Dependency()
    dep_dict = {}

    for line in dep_f:
	if line[0] == '@':
	    if current_name != '':
		dep_dict[current_name] = current_dep
	    current_dep = Dependency()
	    current_dep.name = line[1:-1]
     	    current_name = line[1:-1]
	elif line[0] == '/':
	    current_dep.source = line[:-1]
	elif line[0] != '#':
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
	  any dependencies
        '''
    test_f = open(test_list_name, "r")
    
    req_count = 0
    new_test = ['','','','','','']
    dep_list = []
    test_list = []

    for line in test_f:
	if line == '':
	    continue
        if line[0] == '@':
	    if new_test[0] != '': 
	        test_to_add= Test(new_test,dep_list)
		if (test_to_add.is_valid()):
		    print "test %s added" %test_to_add.name
		    test_list.append(test_to_add)
		dep_list = []
	    new_test[0] = line[:-1]
            req_count = 1
        elif req_count < 6 and line[0] != '#':
	    new_test[req_count] = line[:-1]
	    req_count += 1
	elif line[0] != '#':
	    dep_list.append(line[:-1])

    if new_test[0] != '':
	new_test = Test(new_test, dep_list)
	if (new_test.is_valid()):
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
        cmd_make = ["make", "-C", "config/blank", "micaz", 'TEST_MODE=true', 'MODE=%s' %kernel_mode]
    elif platform == 1:
        cmd_make = ["make", "-C", "config/blank", "mica2", 'TEST_MODE=true', 'MODE=%s' %kernel_mode]
    elif platform == 2:
        cmd_make =  ["make", "-C", "config/blank", "avrora", 'TEST_MODE=true', 'MODE=%s' %kernel_mode]

    try:
      print cmd_make
      subprocess.check_call(cmd_make, stderr=kernel_f, stdout=kernel_f)
      kernel_f.close()

      if (_debug == 1):
	  kernel_f = open(os.environ['SOSTESTDIR'] + '/../python/kernel.log', 'r')
	  print "printing the output for making the kernel"
	  for line in kernel_f:
	      print line
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
    global prog
    global sos_group

    if platform == 0:
        cmd_install = ["make", "-C", "config/blank", "install", "PROG=%s" %prog, "PORT=%s" %install_port[port], "SOS_GROUP=%s" %sos_group, "ADDRESS=%d" %address] 
    elif platform == 1:
	cmd_install = ["make", "-C", "config/blank", "install", "PROG=%s" %prog, "PORT=%s" %install_port[port], "SOS_GROUP=%s" %sos_group, "ADDRESS=%s" %addres]
    else:
        print "you shouldn't be doing this"
	os.exit(0)
    
    print cmd_install
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

    print cmd_run
    time.sleep(10)

    ret = os.fork()
    if ret == 0:
	run_and_redirect(cmd_run, os.environ['SOSTESTDIR'] + '/../python/sossrv.log')

    time.sleep(10)
    return ret

def run_and_log(name, dir, platform, base_file='test_run'):
    out_f = open(base_file+'.good', 'w')
    err_f = open(base_file+'.bad', 'w')

    cmd_make = ['make', '-C', dir, platform]
    cmd_install = ['sos_tool.exe', '--insmod=' + dir + '/' + name + '.mlf']

    clean(dir)

    if (_debug == 1):
	subprocess.call(cmd_make)
	subprocess.call(cmd_install)
    else:
    	subprocess.call(cmd_make, stderr=err_f, stdout=out_f)
    	subprocess.call(cmd_install, stderr=err_f, stdout=out_f)

    out_f.close()
    err_f.close()

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
	 
        dep_loc = os.environ['SOSROOT'] + current_dep.source
	run_and_log(current_dep.name, dep_loc, plat, "%s/install_%s" %(dep_loc, current_dep.name))

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

    failed_tests = []

    cmd_erase = ["sos_tool.exe", "--rmmod=0"]
    for test in test_list:
	subprocess.call(cmd_erase)

	print "running test" + test.name
        driver_location = os.environ['SOSROOT'] + test.driver_location
	test_location = os.environ['SOSTESTDIR'] + test.test_location
	
        #first install all the depenedencies
	install_dependency(test.dep_list, dep_dict,target)

	#first install the sensor driver
	run_and_log(test.driver_name, driver_location, platform, "%s/install_%s" %(driver_location, test.driver_name))
	time.sleep(5)

        # next install the test driver for the sensor
	run_and_log(test.test_name, test_location, platform, '%s/install_%s' %(test_location, test.test_name))

        #now run the python script to verify the output 
	# and wait for the specified amount of time
	child = os.fork()
	if child == 0:
	    print "running python test"
	    cmd_test = ['python', test_location + '/' + test.test_name + '.py']
	    run_and_redirect(cmd_test, outfile="%s/%s.good" %(test_location, test.test_name),
			     error="%s/%s.bad" %(test_location, test.test_name)) 
	else:
	    if test.time != INF:
		print "finite run-time"
		time.sleep(test.time)
		os.kill(child, signal.SIGKILL)
	    os.waitpid(child, 0)
	    if (os.stat("%s/%s.bad" %(test_location, test.test_name))[stat.ST_SIZE] > 0):
		failed_tests.append(test)

	    if (_debug == 1):
		test_out_f = open("%s/%s.bad" %(test_location, test.test_name), 'r')
		print "printing the error output from the python script"
		for line in test_out_f:
		    print line
	    
    return failed_tests

def usage():
    print "test_suite options:\n\
    		-h, --help:	print this usage display\n\
		-n, --no_make:	do not build or reinstall a blank kernel on the node\n\
		-d, --debug:	print all output to log files, and consol\n\
		-p, --platform:	set the platform as either 0, 1, or 2 corresponding to micaz, mica2, and avrora respectively"

def process_args(argv):
    try:
	opts, args = getopt.getopt(argv, "hndp", ["help", "no_make", "debug", "platform"])
    except getopt.GetoptError:
	usage()
	sys.exit(2)
    global _debug 
    _debug=0
    build_kernel = True
    platform = 0

    for opt, arg in opts:
	if opt in ("-h", "--help"):
	    usage()
	    sys.exit()
	elif opt in ('-d', '--debug'):
	    _debug = 1
	elif opt in ('-n', '--no_make'):
	    build_kernel = False;
	elif opt in ('-p', '--platform'):
	    platform = arg

    return (build_kernel, platform)

if __name__ == '__main__':
    avrora_child = 0
    sossrv_child = 0
    build_kernel = True
       
    (build_kernel, target) = process_args(sys.argv[1:])
    if target > 2:
	print "invalid target type, exiting"
	sys.exit(1)

    print "reading the config file"
    configure_setup()

    print "gathering dependencies"
    dep_dict = gather_dependencies(depend_list)

    print "gathering all the tests to run"
    test_list = configure_tests(tests_to_run)
    
    os.chdir(os.environ['SOSROOT'])
    
    if (build_kernel):
	print "building the kernel"
	make_kernel(target)

	if target == 2:
	    #install on avrora with the approriate number of nodes
	    avrora_child = install_on_avrora(number_of_nodes)
	elif number_of_prog == 1: 
	    # installing several nodes via the same programming board
	    # it will install one node at a time, and wait for the user to switch nodes before continuing
	    print "installing several nodes via the same board, please pay attention"
	    while (number_of_nodes > 1):
		install_on_mica(target, number_of_nodes - 1, 0)

		print "this current nodes address is: %d" %(number_of_nodes - 1)
		print "please remove the current node and place another on the programming board"
		print "press any key when ready to install on the next node"
		
		raw_input()

		number_of_nodes -= 1

	    install_on_mica(target, 0, 0)
	    print "this is the base station node, please leave it connected to the programming board"
	else:
	    # installing on several programming boards, serially
	    # we assume that for each port assigned, there is a programming board and node connected
	    print "installing through multiple programming boards, your reaction is not required"

	    while (number_of_prog > 0):
		install_on_mica(target, number_of_prog -1, number_of_prog-1)

		print "this nodes address is: %d" %(number_of_prog-1)
		print "the next node will be installed automatically"

	    number_of_prog -= 1
	    
    print "starting sossrv"
    sos_child = run_sossrv(target)

    print "now running tests"
    failed_tests = run_tests(test_list, target, dep_dict)
    print "tests now completed"

    for test in failed_tests:
	print "test %s had failures, please check the .bad file for it" %test.name

    # killing any child processes that are running
    if (sos_child > 0):
	print "killing sossrv"
	os.kill(sos_child, signal.SIGTERM)
    if (avrora_child > 0):
	print "killing avrora"
	os.kill(avrora_child, signal.SIGTERM)

    os.waitpid(avrora_child, 0)
    print "test suite complete"
