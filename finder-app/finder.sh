#!/bin/bash
# AESD - Assignment 1
# Author: Ruchit Naik
# 1/13/22

#Check if 2 arguments are passed
if [ $# -ne 2 ]
then
	echo "Error:$# aruguments entered. Enter only 2 arguments"
	exit 1
fi

# Assign arguments to variables
filedir=$1
searchdir=$2

#Check if filesdir exits on file system
if [ ! -d "$filedir" ]
then
	echo "Error: $filedir does not exists on the file system"
	exit 1
fi

#Using find command to recursively find the number of files in the
#given directory and pipe it with wc -l command
FILECOUNT=$( find $filedir -type f | wc -l )

#Using grep commnad in recursive mode to find the number of strings
#matching with the argument 2 string in the entire given directory
LINECOUNT=$( grep -re $searchdir $filedir | wc -l )

echo "The number of files are $FILECOUNT and the number of matching lines are $LINECOUNT."
