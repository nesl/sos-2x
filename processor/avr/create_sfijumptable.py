#! /usr/bin/env python
#########################################
# Verify the following:
# 1. Order of writing pages is same as in sfi_jump_table.h
# 2. Number of pages of every type matches with information in sfi_jump_table.h
# 3. Correct exception function is used
###########################################
import string
import re
import sys

# Create file to write
sfifile = open('./sfi_jumptable_proc.S', 'w');

# Write out file skeleton
sfifile.write('.section .sfijmptbl,"ax"\n');
sfifile.write('.global sfi_ker_table_init\n');
sfifile.write('sfi_kertable_init:\n');


numfns = 0
fnsperdomain = 64
# num_mod_domains = 7
# Kernel:2 Modules:7 Pad:1 
num_total_domains = 10


# Fill out the module jump table
#while (numfns < (num_mod_domains * fnsperdomain)):
#    wrline = 'jmp sfi_jmptbl_exception\n'
#    sfifile.write(wrline)
#    numfns = numfns + 1

# Open Kernel Table
kertablefile = open(sys.argv[1] ,'r');
startwrite = 0
pattobj = re.compile("[ ]*/\*[ 0-9]*\*/[ ]*\(void\*\)([a-zA-Z0-9_]*)");
for line in kertablefile:
    if line.find('SOS_KER_TABLE(...)') != -1:
	startwrite = 1
    elif startwrite == 1:
	if line.find('__VA_ARGS__') != -1:
	    startwrite = 0
	    continue
	else:
	    ## Do the regexp here
	    matchobj = pattobj.search(line)
	    if matchobj:
		wrline = 'jmp '
		wrline +=  matchobj.group(1)
		wrline += '\n'
		sfifile.write(wrline)
		numfns = numfns + 1
	    else:
		print "Parse Error"
kertablefile.close()

# Open Proc Table
proctablefile = open(sys.argv[2], 'r');
startwrite = 0
for line in proctablefile:
    if line.find('PROC_KER_TABLE') != -1:
	startwrite = 1
    elif startwrite == 1:
	matchobj = pattobj.search(line)
	if matchobj:
	    wrline = 'jmp '
	    wrline += matchobj.group(1)
	    wrline += '\n'
	    sfifile.write(wrline)
	    numfns = numfns + 1	
	else:
	    startwrite = 0
proctablefile.close()

# Open Plat Table
plattablefile = open(sys.argv[3], 'r');
startwrite = 0
for line in plattablefile:
    if line.find('PLAT_KER_TABLE') != -1:
	startwrite = 1
    elif startwrite == 1:
	matchobj = pattobj.search(line)
	if matchobj:
	    wrline = 'jmp '
	    wrline += matchobj.group(1)
	    wrline += '\n'
	    sfifile.write(wrline)
	    numfns = numfns + 1	
	else:
	    startwrite = 0
plattablefile.close()

# Fill out rest of pages
while (numfns < (num_total_domains * fnsperdomain)):
    wrline = 'jmp sfi_jmptbl_exception\n'
    sfifile.write(wrline)
    numfns = numfns + 1








# End of line
sfifile.write('\n');

# Close file to write
sfifile.close();
