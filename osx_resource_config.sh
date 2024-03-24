#!/bin/bash


while getopts "hdms:" flag; do
 case $flag in
   h) # Handle the -h flag
   # Display script help information
   echo "This is a wrapper that sets the SHMSEG Keynel Variable which limits the permitted segments per process"
   echo ""
   echo "sudo ./osx_resource_config.sh -m       set the SHMSEG to the system max (SHMMNI) of 32 which is plenty to run the tests"
   echo "sudo ./osx_resource_config.sh -d       set the SHMSEG to OSX default of 8"
   echo "sudo ./osx_resource_config.sh -s value          set the SHMSEG to desired value between 8-32"

   ;;
   d) # Handle the -d flag
   # Sets default value
   sysctl kern.sysv.shmseg=8
   ;;
   m) # Handle the -f flag with an argument
   # Sets max value (the one that can run the tests)
   sysctl kern.sysv.shmseg=32
   ;;
   s) # Handle the -f flag with an argument
   value=$OPTARG
   sysctl kern.sysv.shmseg=$value
   ;;
   \?)
   # Handle invalid options
   ;;
 esac
done