#!/bin/bash

#Validate if less arguments passed
if [ $# -lt 2 ]
then
	echo Error: $# arguments entered. 2 arguments expected
	exit 1
fi

#make the directory as per the argument 1
mkdir -p ${1%/*}
#make the file specificed in thr argument 1 path
touch $1

#Handle error if file not created from the previous step
if [ ! -f $1 ]
then
	echo Error: $1 file not created
	exit 1
fi

#Write the new file or overwrite the existing file
echo $2 > $1
