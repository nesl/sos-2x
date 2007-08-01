import subprocess
import sys
import os

make_cont = ['#change the project name to reflect the test you have created\n',
	     '',
	     'ROOTDIR = $(SOSROOT)\n',
	     'INCDIR += -I$(ROOTDIR)/modules/sensordrivers/mts310/include/\n'
	     'SB = mts310\n',
	     'include $(ROOTDIR)/modules/Makerules\n']
	     
def setup_dir(loc):
    ''' Setup the directory structure for a test, if any of the folders in the pathname do not exist, create them
        this will also change the current working directory to where the new files should be placed
    '''

    file_dirs = loc.split('/')
    for dir in file_dirs:
	if dir == '':
	    continue
	try:
            os.listdir(dir)
	except OSError:
	    os.mkdir(dir)
	os.chdir(dir)

def modify_list(sensor_name, sensor_loc, test_name, test_loc, test_time):
    ''' modify the test.conf file so that it has a new entry reflecting this test
	by default, all tests added using this script will have the name generic_kernel_test
    '''

    test_f = open ("python/test.conf", 'a')

    test_f.write("@generic_test:\n%s\n%s\n%s\n%s\n%s\n" %(sensor_name, sensor_loc, test_name, test_loc, test_time))

    test_f.close()

def create_new_test(test_name):
    ''' create the new files for a given test
	it is assumed that the cwd is already the directory which should recieve all the new files
	the .c and .py files are simply copied from the provided templates, whereas the Makefile is created
	from scratch with the correct value assigned to PROJ
    '''

    base_file = os.environ['SOSROOT'] + '/modules/unit_test/python/generic_test/'

    move_cmd = ['cp', base_file + 'generic_test.c', test_name+'.c']

    subprocess.call(move_cmd)

    move_cmd[1] = base_file + 'generic_test.py'
    move_cmd[2] = test_name + '.py'
    subprocess.call(move_cmd)

    make_f = open("Makefile", 'w')
    make_cont[1] = 'PROJ = %s\n' %test_name
    for line in make_cont:
	make_f.write(line)
    make_f.close()
   
if __name__ == "__main__":
    if (len(sys.argv) < 6):
	print "requires five arugments"
	sys.exit(1)
    
    os.chdir(os.environ['SOSROOT'] + "/modules/unit_test")

    sensor_name = sys.argv[1]
    sensor_loc = sys.argv[2]
    test_name = sys.argv[3]
    test_loc = sys.argv[4]
    test_time = sys.argv[5]

    modify_list(sensor_name, sensor_loc, test_name, test_loc, test_time)

    os.chdir('modules')

    print "setting up the directories for the test"
    setup_dir(test_loc)

    print "copying the files needed for the test"
    create_new_test(test_name)

    cwd = os.getcwd()
    print "a new test has been created in %s" %cwd
