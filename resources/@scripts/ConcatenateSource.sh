#!/bin/bash
#================================================================================================
#	This script is for concatenating the entire source into a single file
#	In order to easily upload to any large language model and ask questions about the project.
#	Edit the variables given at the top to your liking or shorten output via exclusion list.
#	The rest of the script should work fine on any system.
#================================================================================================
System="//Win 11, Qt Creator 18.0.1, Qt 6.10.1, MinGW 64 bit, Cmake"
Sources=../../sources
Output=~/Desktop/LanMsg.cpp
SplitAtLine=0			#Set to 0 to disable splitting.
Exclude=(				#➤Add a character to disable exclusion
	"aformabout.cpp"				"aformbroadcast.cpp"			"➤aformchat.cpp"
	"aformchatroom.cpp"				"aformhistory.cpp"				"➤aformmain.cpp"
	"aformsettings.cpp"				"aformtransfer.cpp"				"aformuserinfo.cpp"
	"aformuserselect.cpp"
	"definitionssettings.cpp"
	"historydatabase.cpp"
	"➤lmcore.cpp"
	"main.cpp"						"messaging.cpp"					"messagingfileproc.cpp"
	"messagingproc.cpp"
	"➤networkdatagram.cpp"			"networkinterface.cpp"			"➤networkstreamer.cpp"
	"networktcp.cpp"				"networkudp.cpp"
	"opensslhandler.cpp"			"opushandler.cpp"
	"sharedchatfunctions.cpp"		"shareduifunctions.cpp"			"soundplayer.cpp"
	"translatabletext.cpp"			"translationloader.cpp"
	"widgetchatlog.cpp"				"widgetimagepicker.cpp"			"widgettransferlist.cpp"
	"widgetusertree.cpp"
	"xmlhandler.cpp"
	"zdebuglog.cpp"
    "*.h"						"*.ui"
)
#================================================================================================

shopt -s extglob
pat=$(IFS='|'; echo "${Exclude[*]}")
cd $Sources
{
    echo "$System"
for f in !($pat); do
    [[ -d "$f" ]] && continue
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
} > "$Output"

if [[ "$SplitAtLine" -gt 0 ]]; then
    total=$(wc -l < "$Output")
    parts=$(( (total + SplitAtLine - 1) / SplitAtLine ))
    base="${Output%.*}"
    ext="${Output##*.}"
	if [[ $(( (total + SplitAtLine - 1) / SplitAtLine )) -gt 10 ]]; then
		echo    "Error: Would create too many files ($parts). Maximum is 10."
		echo    "Increase SplitAtLine."
		echo    ""
		read -p "Press enter to continue..."
		exit 1
	fi
    split -l "$SplitAtLine" --numeric-suffixes=1 --suffix-length=${#parts} "$Output" "${base}_part"
    for f in "${base}_part"*; do
        mv "$f" "${f}.${ext}"
    done
    rm "$Output"
fi