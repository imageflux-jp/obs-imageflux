cmake_minimum_required(VERSION 3.16...3.21)

# Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0054 NEW)
# MSVC runtime library flags are selected by an abstraction.
cmake_policy(SET CMP0091 NEW)

project(obs-imageflux VERSION 1.1.0)
add_library(${PROJECT_NAME} MODULE)

# Replace `Your Name Here` with the name (yours or your organization's) you want to see as the
# author of the plugin (in the plugin's metadata itself and in the installers)
set(PLUGIN_AUTHOR "imageflux-jp")

# Replace `com.example.obs-plugin-template` with a unique Bundle ID for macOS releases (used both in
# the installer and when submitting the installer for notarization)
set(MACOS_BUNDLEID "com.example.${PROJECT_NAME}")

# Replace `me@contoso.com` with the maintainer email address you want to put in Linux packages
set(LINUX_MAINTAINER_EMAIL "me@mymailhost.com")

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.3")

target_sources(${PROJECT_NAME} PRIVATE
  src/plugin-main.c
  src/encoder.c
  src/output.cpp
  src/service.cpp
  src/client.cpp
  src/client.h
)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  # Import libobs as main plugin dependency
  find_package(libobs REQUIRED)
  include(cmake/ObsPluginHelpers.cmake)
endif()


# --- Platform-independent build settings ---
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 C_STANDARD 17)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(SORA_CPP_SDK_DIR "${CMAKE_SOURCE_DIR}/../obs-build-dependencies/sora-cpp-sdk")
set(BOOST_ROOT "${SORA_CPP_SDK_DIR}/boost")
set(LYRA_DIR "${SORA_CPP_SDK_DIR}/lyra")
set(WEBRTC_INCLUDE_DIR "${SORA_CPP_SDK_DIR}/webrtc/include")
set(WEBRTC_LIBRARY_DIR "${SORA_CPP_SDK_DIR}/webrtc/lib")
set(WEBRTC_LIBRARY_NAME "webrtc")
set(SORA_DIR "${SORA_CPP_SDK_DIR}/sora")
list(APPEND CMAKE_PREFIX_PATH ${SORA_DIR})
list(APPEND CMAKE_MODULE_PATH ${SORA_DIR}/share/cmake)
set(Boost_USE_STATIC_LIBS ON)
if (WIN32)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

find_package(Boost REQUIRED COMPONENTS json filesystem)
find_package(Lyra REQUIRED)
find_package(WebRTC REQUIRED)
find_package(Sora REQUIRED)
find_package(Threads REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../obs-studio/deps/libcaption)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../obs-studio/deps/uthash)
# target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../../deps/libcaption)
# target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../../deps/uthash)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE OBS::libobs Sora::sora)
# --- End of section ---

# --- Windows-specific build settings and tasks ---
if(OS_WINDOWS)
  # 文字コードを utf-8 として扱うのと、シンボルテーブル数を増やす
  target_compile_options(${PROJECT_NAME}
    PRIVATE
      "$<$<COMPILE_LANGUAGE:CXX>:/utf-8;/bigobj>"
  )
  # CRTライブラリを静的リンクさせる
  set_target_properties(${PROJECT_NAME}
    PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )

  target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      _WIN32_WINNT=0x0A00
      NOMINMAX
      WIN32_LEAN_AND_MEAN
  )

  configure_file(cmake/bundle/windows/installer-Windows.iss.in
                 ${CMAKE_BINARY_DIR}/installer-Windows.generated.iss)

  configure_file(cmake/bundle/windows/resource.rc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rc)
  target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rc)

  if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
  endif()
  # --- End of section ---

  # -- macOS specific build settings and tasks --
elseif(OS_MACOS)
  configure_file(cmake/bundle/macos/installer-macos.pkgproj.in
                 ${CMAKE_BINARY_DIR}/installer-macos.generated.pkgproj)

  set(MACOSX_PLUGIN_GUI_IDENTIFIER "${MACOS_BUNDLEID}")
  set(MACOSX_PLUGIN_BUNDLE_VERSION "${PROJECT_VERSION}")
  set(MACOSX_PLUGIN_SHORT_VERSION_STRING "1")

  target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
  # --- End of section ---

  # --- Linux-specific build settings and tasks ---
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
endif()
# --- End of section ---

setup_plugin_target(${PROJECT_NAME})
