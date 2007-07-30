import subprocess
import sys
import os

makefile_cont = ["#change the project name to reflect the test you have created\n",
		"",
		"ROOTDIR = $(SOSROOT)\n",
		"include $(ROOTDIR)/modules/Makerules\n"]

def setup_dir(loc):
    ''' Setup the direcotry structure for a test, if any of the folders in the pathname do not exist, reate them
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

    test_f.write("@generic_kernel_test:\n%s\n%s\n%s\n%s\n%s\n" %(sensor_name, sensor_loc, test_name, test_loc, test_time))

    test_f.close()

def create_new_test(test_name):
    ''' create the new files for a given test
        it is assumed that the cwd is already the directory which should recieve all the new files
	the .c and .py files are simply copied from the provided templates, whereas the Makefile is created
	from scratch with the correct value assigned to PROJ
	'''
    base_file = os.environ['SOSROOT'] + '/modules/unit_test/python/generic_kernel_test/'

    move_cmd = ['cp', base_file + 'generic_kernel_test.c', test_name+'.c']

    subprocess.call(move_cmd)

    move_cmd[1] = base_file + 'generic_kernel_test.py'
    move_cmd[2] = test_name + '.py'
    subprocess.call(move_cmd)

    make_f = open("Makefile", "w")

    makefile_cont[1] = "PROJ = %s\n" %test_name
    for line in makefile_cont:
        make_f.write(line)

    make_f.close()

if __name__ == "__main__":
    #get the arguements, depending on if there are 3, or 5
    if (len(sys.argv) == 4):
	test_name = sys.argv[1]
	test_loc = sys.argv[2]
	test_time = sys.argv[3]
	sensor_name = 'accel_sensor'
	sensor_loc = '/modules/sensordrivers/mts310/accel'
    elif (len(sys.argv) < 6):
	print "requires three or five arugments"
	sys.exit(1)
    else:
	sensor_name = sys.argv[1]
	sensor_loc = sys.argv[2]
	test_name = sys.argv[3]
	test_loc = sys.argv[4]
	test_time = sys.argv[5]

	os.chdir(os.environ['SOSROOT'] + "/modules/unit_test/modules")
	print "setting up the directories for test1"
	setup_dir(sensor_loc)

	print "copying the files for test1"
	create_new_test(sensor_name)

	sensor_loc = "/modules/unit_test" + sensor_loc

    #change the cwd
    os.chdir(os.environ['SOSROOT'] + "/modules/unit_test")

    print "modifing test.conf"
    modify_list(sensor_name, sensor_loc, test_name, test_loc, test_time)

    os.chdir('modules')

    print "setting up the drictories for test2"
    setup_dir(test_loc)

    print "copying the files for test2"
    create_new_test(test_name)
    

