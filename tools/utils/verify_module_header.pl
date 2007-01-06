#!/usr/bin/perl

if( @ARGV != 1) {
	print "usage: verify_module_header.pl [filename.sos]\n";
	exit 1;
}

my $fin = shift @ARGV;
my $header;
my $mod_id;
my $state_size;
my $num_timers;
my $num_sub_func;
my $num_prov_func;
my $version;
my $processor_type;
my $platform_type;
my $code_id;

open(SOS_FILE, "$fin") or die("Cannot read from $fin");
binmode SOS_FILE;
read(SOS_FILE, $header, 10);

($mod_id, $state_size, $num_timers, 
		$num_sub_func, $num_prov_func, $version, $processor_type, 
	   $platform_type,  $code_id) =
unpack("CCCCCCCCv", $header);

$mod_id += 0;
$state_size += 0;
$num_timers += 0;
$num_sub_func += 0;
$num_prov_func += 0;
$version += 0;
$processor_type += 0;
$platform_type += 0;  
$code_id += 0;
$error = 0;
$warning = 0;

print "Module ID = $mod_id\n";
print "State Size = $state_size\n";
print "\n";

if( $platform_type == 0 ) {
	$error ++;
	print "ERROR: platform ID is not defined in module header\n";
	print "  Please add following line to module header\n\n";
	print ".platform_type  = HW_TYPE /* or PLATFORM_ANY */,\n";
	print "\n";
}

if( $processor_type == 0 ) {
	$error ++;
	print "ERROR: processor ID is not defined in module header\n";
	print "  Please add following line to module header\n\n";
	print ".processor_type = MCU_TYPE,\n";
	print "\n";
}

if( $code_id == 0 ) {
	$warning ++;;
	print "WARNING: module code ID is not defined in module header\n";
	print "  Please add following line to module header\n\n";
	print ".code_id        = ehtons(<Code ID you define>),\n\n";
	print "  If not, code_id will be assumed to be mod_id\n";
	print "\n";
}

close(SOS_FILE);
if( $error > 0 ) {
	exit 1;
}

if( $error == 0 && $warning == 0) {
	print "====  Module Header Verified ====\n";
}

exit 0;
#print "$platform_type\n";
#print "$processor_type\n";
#print "$code_id\n";



