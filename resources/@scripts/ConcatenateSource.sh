#!/bin/bash
#================================================================================================
#	This script is for concatenating the entire source into a single file
#	In order to easily upload to any large language model and ask questions about the project.
#	Edit the variables given at the top to your liking or shorten output via exclusion list.
#	The rest of the script should work fine on any system.
#================================================================================================
SYSTEM="//Win 11, Qt Creator 18.0.1, Qt 6.10.1, MinGW 64 bit, Cmake"
OUTPUT=~/Desktop/LanMessenger2026.cpp
SOURCES=../../sources
EXCLUDE=(
    "aformabout.cpp"	"aformbroadcast.cpp"	"aformchatroom.cpp"
	"aformhistory.cpp"	"aformsettings.cpp"		"aformtransfer.cpp"
	"aformuserinfo.cpp"	"aformuserselect.cpp"
    "*.h"				"*.ui"
)
#================================================================================================

shopt -s extglob
pat=$(IFS='|'; echo "${EXCLUDE[*]}")
cd $SOURCES
{
    echo "$SYSTEM"
for f in !($pat); do
    [[ -d "$f" ]] && continue
		[[ -d "$f" || $f =~ $PATTERN ]] && continue
        echo "//File: $f"
		# Removes block comments, inline comments, empty lines and includes
        awk '
            /\/\*/{skip=1} 
            skip{if(/\*\//)skip=0; next} 
            {
                sub(/\/\/.*/, ""); 
                if ($0 ~ /^[[:space:]]*#include/ || NF == 0) next;
                print 
            }' "$f"
    done
} > "$OUTPUT"
