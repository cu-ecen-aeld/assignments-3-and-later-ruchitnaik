#!/bin/bash

#Check if 2 arguments are passed
if [ $# -ne 2 ]
then
	echo "Error:$# aruguments entered. Enter only 2 arguments"
	exit 1
fi

#Check if filesdir exits on file system
if [ ! -d $1 ]
then
	echo "Error: $1 does not exists on the file system"
	exit 1
fi

#Using find command to recursively find the number of files in the
#given directory and pipe it with wc -l command
FILECOUNT=$( find $1 -type f | wc -l )

#Using grep commnad in recursive mode to find the number of strings
#matching with the argument 2 string in the entire given directory
LINECOUNT=$( grep -re $2 $1 | wc -l )

echo "The number of files are $FILECOUNT and the number of matching lines are $LINECOUNT."
