cmake_minimum_required(VERSION 3.16)
project(LanMessenger VERSION 2026.12.0 LANGUAGES CXX)
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

#Call toggle buttons enabling/disabling needs work. Buttons are enabled early, because the callphase is passed as a boolean.
#Unify font size setting.
#Get rid of all MessageBox errors. They interrupt user flow, they are unnecessary and dated.
#Thinking of switching png to webp, but not sure how painful the transition will be.

#Implement voice&video settings
#Add opus voice chat
#Add vp8 video chat / desktop sharing
#Merge chat, public chat, group chat, transfer history, chat history. Something like discord. Convenience of the new + Simplicity of the old.
#    Encryption in the program only works 1 on 1. That kinda hinders those plans.

#———————————————————————————————————————————————————————<< KNOWN ISSUES >>———————————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
#A sends file to B, while its pending confirmation, if B appears offline, A will show transfer interrupted, while B can still download the file.
#A sends file to B, while its pending confirmation, if A appears offline, B will show transfer interrupted, while A can still cancel.
#Some links that do display fine in chatlog, dont display correctly in history window.
#SetUItext functions dont handle newer controls, switching translations will require a restart.
#Setting migration function doesnt handle newly added ones.
#Some older, and all new buttons dont have Alt underlines. Is this even needed in the %CurrentYear%?
#Colorscheme switching: Main window status selector arrow doesnt change color. Chatlog widget contents lag behind, but switch fine after clicking on widget once.
#"User is online" Toast notification cant display avatar, because the file arrives too late.
#"User is offline" Toast notification displays a lower res avatar.
#Broadcasts arent logged into history.
#Within the chatlog, first line of every message has its collusion box shadowed by the table above. I have found no solution other than leaving 1 line gap.
#File transfers dont have a timestamp parameter, but they generate their own.
#Resetting all settings cannot reset chatwindow geometry, due to how resetting and saving work.
#While A and B are in a call, C calling one of them will end their call. Their chatlog wont show anything, but C will show call declined.
#While A and B are in a call, a chatwindow with C will have hangup button enabled, but the button wont do anything.
#Remote user going stealth doesnt remove them from chatroom. Not sure if this was intended or just an oversight.
#Disabling some sound effects doesnt work.
#Vertical size hint (Auto sizing) for userlistview (public chat/chatroom) needs more work.
