#!/bin/bash
# AESD - Assignment 1
# Author: Ruchit Naik
# 1/13/22

#Validate if less arguments passed
if [ $# -lt 2 ]
then
	echo Error: $# arguments entered. 2 arguments expected
	exit 1
fi

# Assign arguments to variables
writefile=$1
writestr=$2

#make the directory as per the argument 1
mkdir -p ${writefile%/*}
#make the file specificed in thr argument 1 path
touch $writefile

#Handle error if file not created from the previous step
if [ ! -f $writefile ]
then
	echo Error: $writefile file not created
	exit 1
fi

#Write the new file or overwrite the existing file
echo $writestr > $writefile
