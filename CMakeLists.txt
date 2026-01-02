cmake_minimum_required(VERSION 3.16)
project(LanMessenger VERSION 2026.3.0 LANGUAGES CXX)

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

target_compile_definitions(LanMessenger PRIVATE PROJECT_VERSION="${PROJECT_VERSION}") #Passing cmake version into code.
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
#Gotta remove settings>messages>show date&time / maybe instead keep them and recycle the function?
#Tray menu status icon has to be updated when status changes.
#Add voice&video settings
#Group setting needs to be implemented

#Sending family emoji, splits boy and girl into separate enlarged smileys.
#      need to handle the text so the smiley spans arent stripped when pushing into messagelog.
#Switching between old and new client, causes peers to crash. (sometimes) why?
#DragNDrop needs fixing. it was broken somewhere between 1.235-1.239
#Debug log files and chat database keeps piling up, naming should be "date-time", and should be cleaned periodically.
#Broadcasts arent logged. how about group chats?
#Remove chat groups, instead add roles, set by peers themselves, which will act like global grouping.

#Add sticky window borders
#Add nudge feature
#Add opus voice chat
#Add vp8 video chat / desktop sharing
#Merge chat, public chat, group chat, transfer history, chat history. Something like discord. Convenience of the new + Simplicity of the old.

