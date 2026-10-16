#————————————————————————————————————————————————<< Windows specific resource >>—————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/win32.rc"
"#include <windows.h>
IDI_MAIN_ICON ICON \"${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/lanmsg_win.ico\"
1 VERSIONINFO
FILEVERSION ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0
PRODUCTVERSION ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0
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

#———————————————————————————————————————————————————————<< cmakelists >>—————————————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
file(GLOB LanMessenger_SOURCES CONFIGURE_DEPENDS
    "sources/*.cpp" "sources/*.h" "sources/*.ui"
)

add_executable(LanMessenger #———————————————————————————————————————————————————————————————————————————————————————<< Executable
    WIN32
    ${LanMessenger_SOURCES}
    "${CMAKE_CURRENT_BINARY_DIR}/win32.rc"
    resources/resources.qrc
)

set(OPUS_DIR "C:/Qt/Tools/opus-1.6")          #——————————————————————————————————————————————————————————————————————<< libopus 1.6 path
set(VPX_DIR "C:/Qt/Tools/libvpx-1.7")          #——————————————————————————————————————————————————————————————————————<< libvpx 1.7 path
set(OPENSSL_DIR "C:/Qt/Tools/Openssl-3.6")  #——————————————————————————————————————————————————————————————————————<< openssl 3.6.5 path
target_include_directories(LanMessenger PRIVATE
    ${CMAKE_SOURCE_DIR}/sources
    ${OPENSSL_DIR}/include
    ${OPUS_DIR}/include
    ${VPX_DIR}/include/vpx
)

target_compile_definitions(LanMessenger PRIVATE PROJECT_VERSION="${PROJECT_VERSION}") #Passing cmake project version into code.
target_link_libraries(LanMessenger PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Xml Qt6::Multimedia Qt6::MultimediaWidgets
    user32 ole32 oleaut32 uuid
    ${OPENSSL_DIR}/lib/libssl.a
    ${OPENSSL_DIR}/lib/libcrypto.a
    crypt32                             #Required for .a libraries without dlls.
    ${OPUS_DIR}/build/Release/libopus.a
    ${VPX_DIR}/lib/libvpx.a
)

file(GLOB LanMessenger_LANGUAGES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/resources/languages/*.ts")
qt6_add_translations(LanMessenger
    TS_FILES ${LanMessenger_LANGUAGES}
    RESOURCE_PREFIX "/translations"
    INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/sources
    LUPDATE_OPTIONS -no-obsolete -locations relative -source-language en -extensions cpp,h,ui
)
