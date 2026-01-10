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
    ${OPENSSL_DIR}/lib/VC/x64/MT/libssl.lib
    ${OPENSSL_DIR}/lib/VC/x64/MT/libcrypto.lib
    ${OPUS_DIR}/build/Release/libopus.a
)

file(GLOB LanMessenger_LANGUAGES "${CMAKE_SOURCE_DIR}/resources/languages/*.ts")
qt6_add_translations(LanMessenger
    TS_FILES ${LanMessenger_LANGUAGES}
    RESOURCE_PREFIX "/translations"
)
