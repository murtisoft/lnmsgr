cmake_minimum_required(VERSION 3.16)
project(LanMessenger VERSION 2026.7.0 LANGUAGES CXX)
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
#   Messages are sometimes lost(Encryption and decryption errors.).
#   Sending messages from old to new, no losses. New to any... lots of decryption errors / lost messages. great.

#A sends file to B, while its pending confirmation, if B appears offline, A will show transfer interrupted, while B can still download the file.
#There seems to be more connection issues if a client's username includes unicode characters like "ü".
#Colorschemes need more attention.
#File transfers dont have a timestamp parameter, but they generate their own for now. Should change this.
#Right now "User is online" Toast notification cant display avatar, because the file arrives too late. Persistent avatar cache and contact list might solve it.
#Link / path handling needs to be fixed and expanded.
#Broadcasts arent logged. Chat History needs a complete replacement.
#Remove chat groups, instead add roles, set by peers themselves, which will act like global grouping.

#Implement voice&video settings
#Implement group setting
#Add opus voice chat
#Add vp8 video chat / desktop sharing
#Merge chat, public chat, group chat, transfer history, chat history. Something like discord. Convenience of the new + Simplicity of the old.

