#————————————————————————————————————————————————<< Linux >>—————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
file(GLOB LanMessenger_SOURCES CONFIGURE_DEPENDS
    "sources/*.cpp" "sources/*.h" "sources/*.ui"
)

add_executable(LanMessenger
    ${LanMessenger_SOURCES}
    resources/resources.qrc
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED IMPORTED_TARGET opus)   #——————————————————————<< libopus-dev
pkg_check_modules(VPX REQUIRED IMPORTED_TARGET vpx)     #——————————————————————<< libvpx-dev
find_package(OpenSSL REQUIRED)                          #——————————————————————<< libssl-dev
find_path(VPX_HEADER_DIR vpx/vpx_encoder.h)

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
