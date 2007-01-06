#!/usr/bin/perl

use strict;
use Env;
use File::Find;

## variables and args ##

## TODO ##
# allow multipule modules to be added to build
#
# find the files for all modules

my(@module_names);
if ($ARGV[0]) {
  @module_names = @ARGV;
} else {
  print "Usage: [ENV_OPT0=val] [ENV_OPT2=val2] ../wrap_module.pl module1 [module2] ...\n";
  print "\nExamples:\n";
  print "CFLAGS=\"-DUART_COMM\" ADDRESS=5 ./wrap_module.pl echo_uart\n";
  print "CFLAGS=\"-Werror -DDISABLE_RADIO -DDISABLE_WDT\" ADDRESS=5 ./wrap_module.pl loader nic\n";
  exit(0);
}

my($mfile) = "Makefile";
my($cfile) = "module_test_app.c";

my($rootdir);
$rootdir = "../.." unless ( $rootdir = $ENV{"SOSROOT"});

my(@searchdirs) = ( "$rootdir/modules/" );

# append user defined 
my($modpath) = $ENV{"SOSMODPATH"};
if ($modpath) {
  #if ($modpath =~ /${rootdir}\/(.*)\//) {
  #  @searchdirs = ( @searchdirs, "$1" );
  #}
  @searchdirs = (@searchdirs, $modpath);
}

my($mod_name);
my($module); # loop index
my(%modules); # list of full module names
my($moddir);
my(%moddirs);

foreach $module (@module_names) {
  find( sub { /^$module\.c$/ && -f && { ${mod_name} = $File::Find::name } && { ${moddir} = $File::Find::dir } }, @searchdirs);
  if ($mod_name) {
    $modules{$module} = $mod_name;
    $moddirs{$module} = $moddir;
    print "found $module at $mod_name\n";
  } else {
    print "failed to find $module.c\n";
    exit(-1);
  }
}

## Do cleanup ##
# if there is and existing make file do cleanup before starting
if (-e $mfile) {
  system("make -s clean2");
}

## Generate new Makefile ##
open(MK_FILE, ">$mfile") or die("Cannot write to $mfile");
print MK_FILE <<EOF;
# -*-Makefile-*-

PROJ = module_test_app
ROOTDIR = ${rootdir}

EOF

# env args interpereted by the kernel
# general env vars 
my @env_vars = qw(CFLAGS DEFS SRCS_FIRST SRCS OBJS);
# add any enviroment variables if passed in
foreach my $env_var (@env_vars) {
  my $var_val = $ENV{"$env_var"};
  if ($var_val) {
    print MK_FILE "$env_var += $var_val\n\n";
  }
}

# address sets/overrides
@env_vars = qw(ADDRESS UART_ADDRESS I2C_ADDRESS MAC);
# location related vars
@env_vars = (@env_vars, qw(X Y Z LOC_UNIT SOS_GROUP TX_POWER CHANNEL GPS_X_DIR GPS_X_DEG GPS_X_MIN GPS_X_SEC GPS_Y_DIR GPS_Y_DEG GPS_Y_MIN GPS_Y_SEC GPS_Z_UNIT GPS_Z));
# platform enviroment variable options (some options may not do anything on some platforms)
@env_vars = (@env_vars, qw(PROG PORT UISP IP));
# sim specific enviroment variable options
@env_vars = (@env_vars, qw(SIM_PORT_OFFSET SIM_DEBUG_PORT SIM_MAX_GROUP_ID SIM_MAX_MOTE_ID));

# add any enviroment variables if passed in
foreach my $env_var (@env_vars) {
  my $var_val = $ENV{"$env_var"};
  if ($var_val) {
    print MK_FILE "$env_var = $var_val\n\n";
  }
}

print MK_FILE "SRCS += ";
foreach $module (@module_names) {
  print MK_FILE "$module.c ";
}
print MK_FILE "\n\ninclude ../Makerules\n\n";

foreach $module (@module_names) {
  print MK_FILE "vpath ${module}.c $moddirs{$module}\/\n";
}

print MK_FILE <<EOF;

clean2:
	rm -f \*.o \*.srec \*.elf \*.lst \$(PROJ).map \$(PROJ).exe \$(PROJ).exe.out.\* \$(PROJ).od \$(PROJ).srec.out.\* .build.tmp
	rm -f \$(PROJ).c Makefile

EOF

close(MK_FILE);

# use new make file to cleanup any remaining junk
system("make -s clean");

# remove old test file
system("rm -f $cfile");

# Generate new testfile
open(C_FILE, ">$cfile") or die("Cannot write to $cfile");
print C_FILE <<EOF;

#include <sos.h>
/**
 * Must also include the header file that defines the 
EOF

foreach $module (@module_names) {
  print C_FILE " * ${module}_get_header()\n";
}

print C_FILE <<EOF;
 */

EOF

foreach $module (@module_names) {
  print C_FILE "mod_header_ptr ${module}_get_header();\n"
}

print C_FILE <<EOF;

/**
 * application start
 * This function is called once at the end od SOS initialization
 */
void sos_start(void)
{
EOF

foreach $module (@module_names) {
  print C_FILE "ker_register_module(${module}_get_header());\n"
}
print C_FILE "}\n";

print "\n$mfile and $cfile generated\n";

exit(0);
