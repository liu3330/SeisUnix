#! /bin/sh
# Get (approx) common offset section---runs in background
# NOTE: Comment lines preceeding user input start with  #!#
# Author: Jack
set -x

#!# Set input file
input=cdp201to800.pack

### Get the variation in offset from gather to gather (cf. Getcdps)
#sugethw <cdp371to380 cdp offset >offsetvals
#exit

#!# On the basis of results of last paragraph set:
minoffset=291 # minimum offset
varoffset=83 # variation in offsets (can leave some slack)
doffset=106 # delta offset (take smallest)
j=0	# j is the index of the offset range (0 is near offset)

#!# Hard-wired output file name--can change if you wish
section=cos.$j.pack

### This takes a while!  So do in background--hit return to get prompt
min=`expr $minoffset + $j \* $doffset`
max=`expr $min + $varoffset`
suwind <$input key=offset min=$min max=$max >$section &
