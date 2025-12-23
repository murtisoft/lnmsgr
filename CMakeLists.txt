cmake_minimum_required(VERSION 3.16)
project(LanMessenger VERSION 2026.1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Xml Multimedia LinguistTools)

file(GLOB LanMessenger_SOURCES
    "sources/*.cpp" "sources/*.h" "sources/*.ui"
)

add_executable(LanMessenger
    WIN32
    ${LanMessenger_SOURCES}
    resources/win32.rc
    resources/resources.qrc
)

set(OPENSSL_DIR "C:/Qt/Tools/OpenSSL-Win64")  #Win64OpenSSL-3_6_0
target_include_directories(LanMessenger PRIVATE
    ${CMAKE_SOURCE_DIR}/sources
    ${OPENSSL_DIR}/include
)

target_link_libraries(LanMessenger PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Xml Qt6::Multimedia
    ${OPENSSL_DIR}/lib/VC/x64/MTd/libssl.lib
    ${OPENSSL_DIR}/lib/VC/x64/MTd/libcrypto.lib
)

file(GLOB LanMessenger_LANGUAGES "${CMAKE_SOURCE_DIR}/resources/languages/*.ts")
qt6_add_translations(LanMessenger
    TS_FILES ${LanMessenger_LANGUAGES}
    RESOURCE_PREFIX "/translations"
)


#=================o TO DO o================================
#Add emoji to bitmap function and replace old icons (Gotta replace smileys and avatars.)
#   And hopefully find a way to define all that in uidefinitions.h  (70th smiley has to stay a png.)
#Tray menu status icon has to be updated when status changes.

#BUG: Holding mouse button after smiley selection, keeps spamming smileys.
#BUG: Some windows flash at 0,0 - show() should be used AFTER the hidden window is initialized.
#BUG: Communication is completely broken, despite showing no errors
#BUG: The program forgets user avatar sometimes. why?
#Openssl static paths should be replaced at some point
#Have cmake handle win32.rc instead of manually + Have a single definition file to handle all version numbering, app name etc.
#Current sound functions expect extracted files, but the files are an embedded resource now.
#DragNDrop needs fixing. it was broken somewhere between 1.235-1.239
#Debug log files and chat database keeps piling up, naming should be "date-time", and should be cleaned periodically.
#Broadcasts arent logged. how about group chats?
#Remove chat groups, instead add roles, set by peers themselves, which will act like global grouping.
#High dpi? https://github.com/lanmessenger/lanmessenger/issues/29

#Add sticky screen borders
#Add nudge feature
#Add opus voice chat
#Add vp8 video chat / desktop sharing
#Merge chat, public chat, group chat, transfer history, chat history. Something like discord. Convenience of the new + Simplicity of the old.

