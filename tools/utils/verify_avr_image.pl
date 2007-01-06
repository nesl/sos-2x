#!/usr/bin/perl

if( @ARGV != 1) {
  print "usage: verify_image.pl [elf_file_name]\n";
  exit 0;
}
my $fin = shift @ARGV;

open(ADDR, "avr-nm $fin | grep ker_jumptable |" )
  or die "Cannot find jumptable entry $!\n";
while(<ADDR>) {
  #print $_;
  if(/^(\w+)\s+\w\s+(\w+)/) {
    close(ADDR);
    if(hex $1 != 268 || $2 != "ker_jumptable") {
	print "WARNING: kernel jumptable is at wrong address\n";
	exit 1;
    } else {
	print "The kernel jump table in $fin is verified correctly\n";
	exit 0;
    }
  }
}
close(ADDR);
print "WARNING: failed to find kernel jumptable!!!\n";
exit 1;
