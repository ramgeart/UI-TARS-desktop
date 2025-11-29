cmake_minimum_required(VERSION 3.16)
project(ui-tars-linux VERSION 0.2.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_STATIC "Build static binary" OFF)
option(BUILD_DEB "Build Debian package" ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(XTST REQUIRED xtst)
pkg_check_modules(XINERAMA xinerama)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(PNG REQUIRED libpng)
pkg_check_modules(JPEG REQUIRED libjpeg)

# Optional Wayland support
pkg_check_modules(WAYLAND wayland-client)

# JSON library (header-only)
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${X11_INCLUDE_DIRS}
    ${XTST_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
)

if(WAYLAND_FOUND)
    include_directories(${WAYLAND_INCLUDE_DIRS})
    add_definitions(-DHAS_WAYLAND)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/gui_agent.cpp
    src/screenshot.cpp
    src/input_controller.cpp
    src/vlm_client.cpp
    src/action_parser.cpp
    src/config.cpp
    src/logger.cpp
    src/service.cpp
)

# Create executable
add_executable(ui-tars-agent ${SOURCES})

# Link libraries
target_link_libraries(ui-tars-agent
    ${X11_LIBRARIES}
    ${XTST_LIBRARIES}
    ${CURL_LIBRARIES}
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
    nlohmann_json::nlohmann_json
    pthread
)

if(XINERAMA_FOUND)
    target_link_libraries(ui-tars-agent ${XINERAMA_LIBRARIES})
    target_compile_definitions(ui-tars-agent PRIVATE HAS_XINERAMA)
endif()

if(WAYLAND_FOUND)
    target_link_libraries(ui-tars-agent ${WAYLAND_LIBRARIES})
endif()

# Static build
if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

# Install targets
install(TARGETS ui-tars-agent DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/systemd/ui-tars-agent.service DESTINATION lib/systemd/system)
install(FILES ${CMAKE_SOURCE_DIR}/config/ui-tars-agent.conf DESTINATION /etc/ui-tars)

# CPack for Debian package
if(BUILD_DEB)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "ui-tars-agent")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "UI-TARS Linux GUI Agent")
    set(CPACK_PACKAGE_DESCRIPTION "A native Linux GUI agent for computer automation using Vision-Language Models")
    set(CPACK_PACKAGE_MAINTAINER "ByteDance")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/bytedance/UI-TARS-desktop")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libx11-6, libxtst6, libcurl4, libpng16-16, libjpeg62-turbo")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA 
        "${CMAKE_SOURCE_DIR}/debian/postinst;${CMAKE_SOURCE_DIR}/debian/prerm")
    include(CPack)
endif()
