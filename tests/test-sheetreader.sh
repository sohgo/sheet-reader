#!/bin/sh

# ../src/sheetreader -m sql -c ./receive_sheetreader -r UNNUMBER -s UNNUMBER -p ./analyzedimage/ -l ./etc/ $1 | grep INSERT | awk -F "," '{print $6 $7}' | sort

# ../src/sheetreader -m sql -c ./receive_sheetreader -r UNNUMBER -s UNNUMBER -p ./analyzedimage/ -l ./etc/ $1

../src/sheetreader -m sql -c ./receive_sheetreader -r UNNUMBER -s UNNUMBER -p ./analyzedimage/ -l ./etc/ $1 | ./filter | sort
