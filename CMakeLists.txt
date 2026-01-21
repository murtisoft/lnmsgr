cmake_minimum_required(VERSION 3.16)
project(LanMessenger VERSION 2026.7.0 LANGUAGES CXX)
include("resources/@cmake/common.cmake")
if(WIN32)
    include("resources/@cmake/win.cmake")
elseif(ANDROID)
    include("resources/@cmake/android.cmake")
elseif(APPLE)
    include("resources/@cmake/mac.cmake")
elseif(UNIX)
    include("resources/@cmake/lin.cmake")
endif()

#==========================================================================================================================================<< TO DO
#Search for NEED2TEST within the project.
#   Messages are sometimes lost(Corrupted and discarded?).
#   Sending messages from old to new, no losses. New to any... lots of decryption errors / lost messages. great.
#   All font coloring options can be safely removed, so it can be handled via colorschemes and palettes.

#Remove themes.
#colorschemes are a complete mess. Manually set fonts and color palettes causing problems with those.
#File transfers dont have a timestamp parameter, but they generate their own for now. Should change this.
#Right now "User is online" Toast notification cant display avatar, because the file arrives too late. Persistent avatar cache and contact list might solve it.
#Font size settings in settings dialog are local only, and dont even affect public chat for some reason. Maybe i should remove those ancient buttons?
#There should be links to open or show in folder after a file transfer. Not just "Completed."
#Link / path handling needs to be fixed and expanded.
#Chat window pops up in the monitor where the cursor is, not where it was last left.
#   New chatroom does the same, public chat works as expected, though.
#Broadcasts arent logged. Chat History needs a complete replacement.
#Remove chat groups, instead add roles, set by peers themselves, which will act like global grouping.

#Implement voice&video settings
#Implement group setting
#Add opus voice chat
#Add vp8 video chat / desktop sharing
#Merge chat, public chat, group chat, transfer history, chat history. Something like discord. Convenience of the new + Simplicity of the old.

