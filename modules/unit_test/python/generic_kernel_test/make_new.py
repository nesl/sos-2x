import subprocess
import sys
import os

def setup_dir(loc):
    
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
    test_f = open ("python/test.lst", 'a')

    test_f.write("#generic_kernel_test:\n%s\n/modules/unit_test%s\n%s\n%s\n%s\n" %(sensor_name, sensor_loc, test_name, test_loc, test_time))

    test_f.close()

def create_new_test(test_name):
    base_file = os.environ['SOSROOT'] + '/modules/unit_test/python/generic_kernel_test/'

    move_cmd = ['cp', base_file + 'generic_kernel_test.c', test_name+'.c']

    subprocess.call(move_cmd)

    move_cmd[1] = base_file + 'generic_kernel_test.py'
    move_cmd[2] = test_name + '.py'
    subprocess.call(move_cmd)

    move_cmd[1] = base_file +'Makefile'
    move_cmd[2] = 'Makefile'
    subprocess.call(move_cmd)

if __name__ == "__main__":
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
	setup_dir(sensor_loc)
	create_new_test(sensor_name)

    os.chdir(os.environ['SOSROOT'] + "/modules/unit_test")

    modify_list(sensor_name, sensor_loc, test_name, test_loc, test_time)

    os.chdir('modules')
    setup_dir(test_loc)
    create_new_test(test_name)
    

