#!/usr/bin/perl

$iswindows = (index($^O, "Win")>0);

# Specify where to find input files (use slashes instead of backslashes for directories)
$input_dir = "../examples/";
# Specify where to put output files (use slashes instead of backslashes for directories)
$output_dir = "../refs/";
# Specify where to find dump_dpx (use slashes instead of backslashes for directories)
$dump_dpx_exe = "../dump_dpx/dump_dpx";

if($iswindows)
{
    print "Detected Windows system\n";
    $input_dir =~ s/\//\\/g;
    $output_dir =~ s/\//\\/g;
    $dump_dpx_exe = "..\\dump_dpx\\debug\\dump_dpx.exe";
} else {
    print "Detected non-Windows system\n";
}

if(@ARGV < 1)
{
    die "Usage: $0 <listfile>\nCreates reference files (header info and pixel data) for a list of .dpx files\n";
}

if(-d $output_dir)
{
  print "Output directory $output_dir exists\n";
} else {
  print "Output directory $output_dir does not exist. Creating directory.\n";
  mkdir $output_dir;
}

if(!(-e $dump_dpx_exe))
{
    die "Cannot find $dump_dpx_exe. Make sure it is built and its location is correct (or edit the beginning of this script with the correct location)\n"; 
}

open(INFILE, $ARGV[0]) or die "Cannot open $ARGV[0] list file for input\n";

while(<INFILE>)
{
    # remove cr or cr/lf
    chomp; chomp;

    $basename = $_;
    $basename =~ s/\\/\//g;   # make directories use forward slashes
    while(index($basename, "/") >= 0) { $basename = substr($basename, index($basename, "/") + 1); }
    $basename =~ s/.dpx//;

    $dump_cmd = "$dump_dpx_exe ${input_dir}$_ -rawout ${output_dir}${basename} > ${output_dir}${basename}.txt";
    print "Executing: $dump_cmd\n";
    system $dump_cmd;
}

close(INFILE);
