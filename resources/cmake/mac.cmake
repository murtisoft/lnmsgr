#————————————————————————————————————————————————<< macOS >>—————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
file(GLOB LanMessenger_SOURCES CONFIGURE_DEPENDS
    "sources/*.cpp" "sources/*.h" "sources/*.ui"
)

set(MACOSX_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/lanmsg_mac.icns")
set(MAC_RESOURCES "")
if(EXISTS "${MACOSX_ICON}")
    set_source_files_properties("${MACOSX_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MAC_RESOURCES "${MACOSX_ICON}")
endif()

add_executable(LanMessenger
    MACOSX_BUNDLE
    ${LanMessenger_SOURCES}
    ${MAC_RESOURCES}
    resources/resources.qrc
)

set_target_properties(LanMessenger PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "LAN Messenger"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.lanmessenger.app"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "lanmsg_mac.icns"
)

list(APPEND CMAKE_PREFIX_PATH /opt/homebrew /usr/local)  #——————————————————<< Homebrew: brew install opus libvpx openssl@3 pkg-config
set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl@3)
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED IMPORTED_TARGET opus)
pkg_check_modules(VPX REQUIRED IMPORTED_TARGET vpx)
find_package(OpenSSL REQUIRED)
find_path(VPX_HEADER_DIR vpx/vpx_encoder.h HINTS /opt/homebrew/include /usr/local/include)

target_include_directories(LanMessenger PRIVATE
    ${CMAKE_SOURCE_DIR}/sources
    ${VPX_HEADER_DIR}/vpx
)

target_compile_definitions(LanMessenger PRIVATE PROJECT_VERSION="${PROJECT_VERSION}") #Passing cmake project version into code.
target_link_libraries(LanMessenger PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Xml Qt6::Multimedia Qt6::MultimediaWidgets
    PkgConfig::OPUS
    PkgConfig::VPX
    OpenSSL::SSL
    OpenSSL::Crypto
)

file(GLOB LanMessenger_LANGUAGES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/resources/languages/*.ts")
qt6_add_translations(LanMessenger
    TS_FILES ${LanMessenger_LANGUAGES}
    RESOURCE_PREFIX "/translations"
    INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/sources
    LUPDATE_OPTIONS -no-obsolete -locations relative -source-language en -extensions cpp,h,ui
)
