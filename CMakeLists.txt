cmake_minimum_required(VERSION 3.16)
project(LanMessenger VERSION 2026.17.0 LANGUAGES CXX)
include("resources/cmake/common.cmake")
if(WIN32)
    include("resources/cmake/win.cmake")
elseif(ANDROID)
    include("resources/cmake/android.cmake")
elseif(APPLE)
    include("resources/cmake/mac.cmake")
elseif(UNIX)
    include("resources/cmake/lin.cmake")
endif()

#——————————————————————————————————————————————————————————<< TO DO >>———————————————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
#Search for //TODO within the project. 15 year old ones are renamed to //TOD0
#Appearing offline has regressed.
#lblName and txtNote Elided text functionality was removed, bring it back.
#Some links that do display fine in chatlog, dont display correctly in history window.
#Setting migration function doesnt handle newly added ones.
#There is an incosistency with the borders around peers in userlist. investigate.
#All translations are a decade behind.

#——————————————————————————————————————————————————————————<< NOTES >>———————————————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#Broadcasts arent logged into history.
#File transfers dont have a timestamp parameter, but they generate their own.
#Remote user going stealth doesnt remove them from chatroom. Not sure if this was intended or just an oversight.
#Avatar changes while appearing offline is sent to peers anyway.
#File transfers should be displayed nested inside a regular message with timestamps.
#Merge chat, public chat, group chat, transfer history, chat history. Something like discord. Convenience of the new + Simplicity of the old.
#    Encryption in the program only works 1 on 1. That kinda hinders those plans.
#   I dont think that this is possible anymore without BTFOing the compatibility entirely so... no go.
