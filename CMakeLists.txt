cmake_minimum_required(VERSION 3.16)
project(LanMessenger VERSION 2026.10.0 LANGUAGES CXX)
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

#==========================================================================================================================================<< TO DO
#Search for TODO in the project. 15 year old ones are renamed to TOD0

#A sends file to B, while its pending confirmation, if B appears offline, A will show transfer interrupted, while B can still download the file.
#A sends file to B, while its pending confirmation, if A appears offline, B will show transfer interrupted, while A can still cancel.
#Some links that do display fine in chatlog, dont display correctly in history window, why?
#Colorschemes need more attention.
#File transfers dont have a timestamp parameter, but they generate their own for now. Should change this.
#Right now "User is online" Toast notification cant display avatar, because the file arrives too late. Persistent avatar cache and contact list might solve it.
#Link / path handling needs to be fixed and expanded.
#Broadcasts arent logged into history
#Remove chat groups, instead add roles, set by peers themselves, which will act like global grouping.

#Implement voice&video settings
#Implement group setting
#Add opus voice chat
#Add vp8 video chat / desktop sharing
#Merge chat, public chat, group chat, transfer history, chat history. Something like discord. Convenience of the new + Simplicity of the old.

