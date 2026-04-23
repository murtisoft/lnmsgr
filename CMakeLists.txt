cmake_minimum_required(VERSION 3.16)
project(LanMessenger VERSION 2026.11.0 LANGUAGES CXX)
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
#Search for TODO within the project. 15 year old ones are renamed to TOD0

#Call toggle buttons enabling/disabling needs work.
#Unify font size setting.
#Remove chat groups, instead add roles, set by peers themselves, which will act like global grouping.

#Implement voice&video settings
#Implement group setting
#Add opus voice chat
#Add vp8 video chat / desktop sharing
#Merge chat, public chat, group chat, transfer history, chat history. Something like discord. Convenience of the new + Simplicity of the old.


#———————————————————————————————————————————————————————<< KNOWN ISSUES >>———————————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
#A sends file to B, while its pending confirmation, if B appears offline, A will show transfer interrupted, while B can still download the file.
#A sends file to B, while its pending confirmation, if A appears offline, B will show transfer interrupted, while A can still cancel.
#Some links that do display fine in chatlog, dont display correctly in history window.
#SetUItext functions dont handle newer controls, switching translations will require a restart.
#Setting migration function doesnt handle newly added ones.
#Some older, and all new buttons dont have Alt underlines. Is this even needed in the %CurrentYear%?
#Colorscheme switching: Main window status selector arrow doesnt change color. Chatlog widget contents lag behind, but switch fine after clicking on widget once.
#Path normalization made it so that all backward slashes will be converted to forward slash. (Cant send backward slash anymore.)
#"User is online" Toast notification cant display avatar, because the file arrives too late.
#"User is offline" Toast notification displays a lower res avatar.
#Broadcasts arent logged into history.
#Within the chatlog, first line of every message has its collusion box shadowed by the table above. I have found no solution other than leaving 1 line gap.
#File transfers dont have a timestamp parameter, but they generate their own.
#Chatwindows save their geometry even when they are maximized, that should be temporary.
#Resetting all settings cannot reset chatwindow geometry, due to how resetting and saving work.
#While A and B are in a call, C calling one of them will end their call. Their chatlog wont show anything, but C will show call declined.
#While A and B are in a call, a chatwindow with C will have hangup button enabled, but the button wont do anything.
#Remote user going stealth doesnt remove them from chatroom. Not sure if this was intended or just an oversight.
#Disabling some sound effects doesnt work.
