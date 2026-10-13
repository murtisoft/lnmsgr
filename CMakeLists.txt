cmake_minimum_required(VERSION 3.16)
project(LanMessenger VERSION 2026.16.0 LANGUAGES CXX)
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

#File transfer window should save icons as webp.

#———————————————————————————————————————————————————————<< KNOWN ISSUES >>———————————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
#Loopback prtscr snip sending writes 0 byte file. Loopback calls get interrupted.
#Window popup and focus behaves unreliably, sometimes demanding immediate attention, causing frustration.
#User is typing status label is at the top now.
#Some links that do display fine in chatlog, dont display correctly in history window.
#SetUItext functions dont handle newer controls, switching translations will require a restart.
#Setting migration function doesnt handle newly added ones.
#Some older, and all new buttons dont have Alt underlines. Is this even needed in the %CurrentYear%?
#Resetting all settings cannot reset chatwindow geometry, due to how resetting and saving work. Window sizes remain in use, and get resaved.
#Disabling some sound effects doesnt work.
#Vertical size hint (Auto sizing) for userlistview (public chat/chatroom) needs more work.

#——————————————————————————————————————————————————————————<< NOTES >>———————————————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
#webp: convert the image to sRGB, then clear its color space so no ICC profile is embedded (saves 4KB)
#lblName and txtNote Elided text functionality was removed, bring it back?
#File transfers should be displayed nested inside a regular message with timestamps.
#Known filetypes should send webp preview as icon. Other types use sender file icon as preview
#Broadcasts arent logged into history.
#File transfers dont have a timestamp parameter, but they generate their own.
#Remote user going stealth doesnt remove them from chatroom. Not sure if this was intended or just an oversight.
#Avatar changes while appearing offline is sent to peers anyway.
#Get rid of all MessageBox errors. They interrupt user flow, they are unnecessary and dated.
#Thinking of switching png to webp, but not sure how painful the transition will be.
#Merge chat, public chat, group chat, transfer history, chat history. Something like discord. Convenience of the new + Simplicity of the old.
#    Encryption in the program only works 1 on 1. That kinda hinders those plans.
