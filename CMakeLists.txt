cmake_minimum_required(VERSION 3.16)
project(LanMessenger VERSION 2026.4.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Xml Multimedia LinguistTools)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/win32.rc" #==============================================================================<< Windows specific resource
"#include <windows.h>
IDI_MAIN_ICON ICON \"${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/lanmsg_win.ico\"
1 VERSIONINFO
FILEVERSION ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},0,0
PRODUCTVERSION ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},0,0
{
    BLOCK \"StringFileInfo\" {
        BLOCK \"040904B0\" {
            VALUE \"FileDescription\", \"LAN Messenger\"
            VALUE \"FileVersion\",     \"${PROJECT_VERSION}\"
            VALUE \"ProductName\",     \"LAN Messenger\"
            VALUE \"ProductVersion\",  \"${PROJECT_VERSION}\"
        }
    }
    BLOCK \"VarFileInfo\" { VALUE \"Translation\", 0x409, 1200 }
}")

file(GLOB LanMessenger_SOURCES
    "sources/*.cpp" "sources/*.h" "sources/*.ui"
)

add_executable(LanMessenger #============================================================================================================<< Executable
    WIN32
    ${LanMessenger_SOURCES}
    "${CMAKE_CURRENT_BINARY_DIR}/win32.rc"
    resources/resources.qrc
)

set(OPUS_DIR "C:/Qt/Tools/opus-1.6")          #libopus 1.6
set(OPENSSL_DIR "C:/Qt/Tools/OpenSSL-Win64")  #Win64OpenSSL-3_6_0
target_include_directories(LanMessenger PRIVATE
    ${CMAKE_SOURCE_DIR}/sources
    ${OPENSSL_DIR}/include
    ${OPUS_DIR}/include
)

target_compile_definitions(LanMessenger PRIVATE PROJECT_VERSION="${PROJECT_VERSION}") #Passing cmake project version into code.
target_link_libraries(LanMessenger PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Xml Qt6::Multimedia
    ${OPENSSL_DIR}/lib/VC/x64/MTd/libssl.lib
    ${OPENSSL_DIR}/lib/VC/x64/MTd/libcrypto.lib
    ${OPUS_DIR}/build/Release/libopus.a
)

file(GLOB LanMessenger_LANGUAGES "${CMAKE_SOURCE_DIR}/resources/languages/*.ts")
qt6_add_translations(LanMessenger
    TS_FILES ${LanMessenger_LANGUAGES}
    RESOURCE_PREFIX "/translations"
)


#==========================================================================================================================================<< TO DO
#Search for NEED2TEST within the project.
#Remove themes.

#Add nudge feature / Need to find a way to transmit the command, while also not crashing old clients.
#File transfers dont have a timestamp parameter, but they generate their own for now. Should change this.
#Crashing somewhere after "New connection received" and before "Accepted connection from user", (when a new peer joins network.)
#   Messages are sometimes lost(Corrupted and discarded?). I think these are related.
#   New client starting for the first time reliably crashes all old clients.
#Right now "User is online" Toast notification cant display avatar, because the file arrives too late. Persistent avatar cache and contact list might solve it.
#Font size settings in settings dialog are local only, and dont even affect public chat for some reason. Maybe i should remove those ancient buttons?
#There should be links to open or show in folder after a file transfer. Not just "Completed."
#Chat window pops up in the monitor where the cursor is, not where it was last left.
#   New chatroom does the same, public chat works as expected, though.
#Broadcasts arent logged. Chat History needs a complete replacement.
#Remove chat groups, instead add roles, set by peers themselves, which will act like global grouping.

#Implement voice&video settings
#Implement group setting
#Add opus voice chat
#Add vp8 video chat / desktop sharing
#Merge chat, public chat, group chat, transfer history, chat history. Something like discord. Convenience of the new + Simplicity of the old.

