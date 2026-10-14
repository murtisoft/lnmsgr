#————————————————————————————————————————————————<< Android >>—————————————————————————————————————————————————
#————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
file(GLOB LanMessenger_SOURCES CONFIGURE_DEPENDS
    "sources/*.cpp" "sources/*.h" "sources/*.ui"
)

qt_add_executable(LanMessenger
    ${LanMessenger_SOURCES}
    resources/resources.qrc
)

set(OPUS_DIR "" CACHE PATH "Prebuilt libopus root, containing <ABI>/lib and include")
set(VPX_DIR "" CACHE PATH "Prebuilt libvpx root, containing <ABI>/lib and include")
set(OPENSSL_DIR "" CACHE PATH "Prebuilt OpenSSL root, containing <ABI>/lib and include")

target_include_directories(LanMessenger PRIVATE
    ${CMAKE_SOURCE_DIR}/sources
    ${OPENSSL_DIR}/include
    ${OPUS_DIR}/include
    ${VPX_DIR}/include/vpx
)

target_compile_definitions(LanMessenger PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")
target_link_libraries(LanMessenger PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Xml Qt6::Multimedia Qt6::MultimediaWidgets
    ${OPENSSL_DIR}/${ANDROID_ABI}/lib/libssl.a
    ${OPENSSL_DIR}/${ANDROID_ABI}/lib/libcrypto.a
    ${OPUS_DIR}/${ANDROID_ABI}/lib/libopus.a
    ${VPX_DIR}/${ANDROID_ABI}/lib/libvpx.a
)

file(GLOB LanMessenger_LANGUAGES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/resources/languages/*.ts")
qt6_add_translations(LanMessenger
    TS_FILES ${LanMessenger_LANGUAGES}
    RESOURCE_PREFIX "/translations"
    INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/sources
    LUPDATE_OPTIONS -locations relative -source-language en -extensions cpp,h,ui
)
