#!/usr/bin/python

#
# Copyright (c) 2005 The Regents of the University of California.  All rights
# reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. All advertising materials mentioning features or use of this software
# must display the following acknowledgment: This product includes software
# developed by Networked & Embedded Systems Lab at UCLA
#
# 4. Neither the name of the University nor that of the Laboratory may be used
# to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# @brief build_test: Functions to help automated build tests of SOS 
# @author Roy Shea (roy@cs.ucla.edu) 
# @date Ported to SOS 2.x Spring 2007
#

# TODO: Separate out white list functionality

import os
import sys
import subprocess

#
# Assume that this is run from the SOS directory
#
def set_environment(home = "/home/test"):
    os.environ['SOSROOT'] = home + "/sos-2x/trunk"  # this path might need to be modified on a user basis
    os.environ['SOSTOOLDIR'] = home + "/local"      # as might this path
    os.environ['SOSMSPTOOLDIR'] = os.environ['SOSTOOLDIR']
    os.environ['PATH'] = home + os.environ['SOSTOOLDIR'] + "/bin:" + os.environ['PATH']
    os.chdir(os.environ['SOSROOT'])


#
# Helper function to log the output of a make
#
def build_and_log(dir, command="", out="/dev/null", err="/dev/null"):
    out_f = open(out, "w")
    err_f = open(err, "w")
    cmd_make = ["make", "-C", dir, command]
    print cmd_make
    subprocess.call(cmd_make, stdout=out_f, stderr=err_f)


#
# Helper function to clean up the source tree
#
def clean(dir):
    cmd_clean = ["make", "-C", "clean", dir]
    null = open("/dev/null", "w")
    print cmd_clean
    subprocess.call(cmd_clean, stdout=null, stderr=null)


#
# Find all modules in the 'base_dir' subtree.
# 
# This function assumes that any directory located under base_dir containing a
# Makefile is a valid module build location.  Output from the build is stored
# into a file named based on the directory containing the Makefile (so modules
# with in directories the same end name may become hidden).
#
def find_modules(base_dir):
    
    module_dirs = []
    
    # Find the candidate module locations
    for root, dirs, files in os.walk(base_dir):
        if 'Makefile' in files:
            module_dirs.append(root)

    return module_dirs
   
#
# Build modules
#
def build_modules(target, module_dirs):
    
    # Build the candidate modules
    for module_dir in module_dirs:
        
        module_name = os.path.basename(module_dir)
        stdout = module_name + ".good.tmp"
        stderr = module_name + ".bad.tmp"
        clean(module_dir)
        build_and_log(module_dir, target, stdout, stderr)
        clean(module_dir)


#
# Builds specific tools used by SOS.  Currently builds:
#   sos_server
#   sos_tool
#
def build_tools():
    
    # Build sos_server
    path = "tools/sos_server/bin"
    command = "x86"
    stdout = "sos_server.good.tmp"
    stderr = "sos_server.bad.tmp"
    clean(path)
    build_and_log(path, command, stdout, stderr)
    clean(path)
    
    # Build sos_tool
    path = "tools/sos_tool"
    command = "emu"
    stdout = "sos_tool.good.tmp"
    stderr = "sos_tool.bad.tmp"
    clean(path)
    build_and_log(path, command, stdout, stderr)
    clean(path)
    

#
# Build SOS kernels that are commonly deployed on embedded nodes.  Currently
# builds:
#   blank
#
def build_embedded(target):
    
    # Build base
    path = "config/blank"
    command = target
    stdout = target + ".good.tmp"
    stderr = target + ".bad.tmp"
    clean(path)
    build_and_log(path, command, stdout, stderr)
    clean(path)
    
    

#
# Function to ignore bogus errors in the build logs.  Returns true if the line
# can be safely ignored.
#
def ignore_line(line, whitelist):
    
    # Check if the line can be ignored
    for ignore in whitelist:
        if ignore in line:
            return True

    # Note that the line can not be ignored
    return False


#
# Combine related log files into a single large log file.  Empty files are
# ignored.
#
def combine_files(dir, pattern, out_file, file_title=""):
    import fnmatch
    
    # Error messages that we know are okay
    whitelist = ['dos2unix', 'make: Entering directory', 'avr-gcc -E', \
            'cilly.asm.exe --', 'make: Leaving directory']

    
    out = open(out_file, 'w')
    files = []

    # Make list of files
    for file in os.listdir(dir):
        if fnmatch.fnmatch(file, pattern):
            files.append(os.path.join(dir, file))

    # Concatenate files
    for file in files:
        
        # Read file into a buffer and then to file if buffer is not empty
        buff = []
        tmp_file = open(file)
        
        for line in tmp_file:
            if(ignore_line(line, whitelist) or line == '\n'):
                continue
            else:
                buff.append(line)
        
        if len(buff) > 0:
            out.write(file.split('.')[1][1:] + ": " + file_title + '\n')
            for line in buff:
                out.write(line)
            out.write('\n')

    out.close()


#
# Example usage
#
if __name__ == '__main__':

    #
    # Set environment for testing
    #
    set_environment(sys.argv[1])


    # 
    # Find modules that we are interested in
    #
    module_dirs = find_modules("modules")


    #
    # Filter the modules we want to test
    #
    skip_list = [\
            'module_template',\
            'VM',\
            'vire',\
            ];
    
    modules = []
    for module_dir in module_dirs:
        if not ignore_line(module_dir, skip_list):
            modules.append(module_dir)
        else:
            print "Ignoring module located in: " + module_dir
            
    build_tools()
    build_embedded('mica2')
    build_embedded('micaz')
    build_embedded('sim')
    build_modules('mica2', modules)
    
    # Generate diagnostic files
    combine_files('.', '*.bad.tmp', 'build_fail.txt', 'Build Errors')

    # Send diagnostics to stdout
    error = open('build_fail.txt', 'r')
    print("SOS BUILD STATUS:\n")
    for line in error:
        print line,
    error.close()

    # Clean up the mess
    for word in os.listdir("."):
        if word.endswith("bad.tmp") \
                or word.endswith("good.tmp") \
                or word.endswith("check.tmp"):
            subprocess.call(["rm", word])

