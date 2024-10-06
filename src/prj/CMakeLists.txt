#
# Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
cmake_minimum_required (VERSION 3.15)
# https://cmake.org/cmake/help/latest/prop_tgt/MSVC_RUNTIME_LIBRARY.html
cmake_policy(SET CMP0091 NEW)

include(cmake/get_cpm.cmake)

project(HTMLTag LANGUAGES CXX)

# ==================================================
# Detect build configuration
# ==================================================
set (${PROJECT_NAME}_BIN_DIR "${CMAKE_SOURCE_DIR}/../../out/${TARGET_PLATFORM}")

if ("${CMAKE_GENERATOR}" MATCHES "^NMake")
  set (NMAKE_BUILD "YES" CACHE INTERNAL "Building with NMake")
  set (${PROJECT_NAME}_BIN_DIR "${${PROJECT_NAME}_BIN_DIR}/${CMAKE_BUILD_TYPE}")
endif ()

if (MSVC OR "${CMAKE_GENERATOR}" MATCHES "(Visual Studio)")
  set (VC_BUILD "ON" CACHE BOOL "Use Microsoft Visual C++ build tools")
endif ()

# ==================================================
# Add source files, libraries, resources
# ==================================================
CPMAddPackage ("gh:npp-plugins/plugintemplate@4.4")
CPMAddPackage ("gh:brofield/simpleini@4.22")
CPMAddPackage ("gh:leethomason/tinyxml2@10.0.0")

set (${PROJECT_NAME}_src
  ${CMAKE_SOURCE_DIR}/../LibNppPlugin/include/SciTextObjects.cpp
  ${CMAKE_SOURCE_DIR}/../LibNppPlugin/include/TextConv.cpp
  ${CMAKE_SOURCE_DIR}/../LibNppPlugin/FuncArray.cpp
  ${CMAKE_SOURCE_DIR}/../LibNppPlugin/PluginBase.cpp
  ${CMAKE_SOURCE_DIR}/../LibNppPlugin/LocalizedPlugin.cpp
  ${CMAKE_SOURCE_DIR}/../Forms/AboutDlg.cpp
  ${CMAKE_SOURCE_DIR}/../TagFinder.cpp
  ${CMAKE_SOURCE_DIR}/../Entities.cpp
  ${CMAKE_SOURCE_DIR}/../Unicode.cpp
  ${CMAKE_SOURCE_DIR}/../HtmlTag.cpp
  ${CMAKE_SOURCE_DIR}/DllMain.cpp
)

add_library (${PROJECT_NAME} SHARED ${${PROJECT_NAME}_src})
set_target_properties (${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${${PROJECT_NAME}_BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${${PROJECT_NAME}_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${${PROJECT_NAME}_BIN_DIR}"
)

target_include_directories (${PROJECT_NAME} PUBLIC
  "${CMAKE_SOURCE_DIR}/.."
  "${CMAKE_SOURCE_DIR}/../Forms"
  "${CMAKE_SOURCE_DIR}/../LibNppPlugin"
  "${CMAKE_SOURCE_DIR}/../LibNppPlugin/include"
)

if (plugintemplate_ADDED)
  target_include_directories (${PROJECT_NAME} PUBLIC
    "${plugintemplate_SOURCE_DIR}/src"
    "${plugintemplate_SOURCE_DIR}/src/DockingFeature"
  )
  target_sources (${PROJECT_NAME} PRIVATE
    "${plugintemplate_SOURCE_DIR}/src/DockingFeature/StaticDialog.cpp")
endif ()

if (simpleini_ADDED)
  target_include_directories (${PROJECT_NAME} PUBLIC "${simpleini_SOURCE_DIR}")
endif ()

if (tinyxml2_ADDED)
  target_include_directories (${PROJECT_NAME} PUBLIC "${tinyxml2_SOURCE_DIR}")
  target_sources (${PROJECT_NAME} PRIVATE "${tinyxml2_SOURCE_DIR}/tinyxml2.cpp")
endif ()

set (WINAPI_LIBS
  kernel32.lib
  user32.lib
  comdlg32.lib
  shlwapi.lib
  shell32.lib
)

target_link_libraries (${PROJECT_NAME} PRIVATE ${WINAPI_LIBS})
target_sources (${PROJECT_NAME} PRIVATE ${PROJECT_NAME}.rc)

# ==================================================
# Configure build mode
# ==================================================
target_compile_definitions (${PROJECT_NAME} PUBLIC
  UNICODE
  _UNICODE
  _WINDOWS
  _USRDLL
  WIN32_LEAN_AND_MEAN
  SCI_DISABLE_PROVISIONAL
)

if (VC_BUILD)
  string(TOLOWER "${TARGET_PLATFORM}" PLATFORM_ID)
  # https://stackoverflow.com/a/24767451
  # https://cmake.org/cmake/help/latest/command/set.html#set-cache-entry
  set (CMAKE_C_FLAGS "" CACHE STRING "" FORCE)
  set (CMAKE_C_FLAGS_DEBUG "" CACHE STRING "" FORCE)
  set (CMAKE_C_FLAGS_RELEASE "" CACHE STRING "" FORCE)
  set (CMAKE_C_FLAGS_RELWITHDEBINFO "" CACHE STRING "" FORCE)
  set (CMAKE_C_FLAGS_MINSIZEREL "" CACHE STRING "" FORCE)
  set (CMAKE_CXX_FLAGS CACHE STRING "" FORCE)
  set (CMAKE_CXX_FLAGS_DEBUG CACHE STRING "" FORCE)
  set (CMAKE_CXX_FLAGS_RELEASE CACHE STRING "" FORCE)
  set (CMAKE_CXX_FLAGS_RELWITHDEBINFO CACHE STRING "" FORCE)
  set (CMAKE_CXX_FLAGS_MINSIZEREL CACHE STRING "" FORCE)

  if (NOT "${PLATFORM_ID}" MATCHES "arm")
    set_target_properties(${PROJECT_NAME} PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
      VS_PLATFORM_TOOLSET "v141_xp"
    )
    target_compile_definitions (${PROJECT_NAME} PRIVATE
      _USING_V110_SDK71_
      _WIN32_WINNT=_WIN32_WINNT_WINXP
      NTDDI_VERSION=NTDDI_WINXPSP3
    )
  endif ()

  target_compile_definitions (${PROJECT_NAME} PUBLIC
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
  )

  target_compile_options (${PROJECT_NAME} PRIVATE
    /nologo
    /source-charset:utf-8
    /std:c++17
    /permissive-
    /Zc:__cplusplus,inline
    /EHsc
    /TP
    /MP
    /W4
  )

  set (LINK_FLAGS "/SUBSYSTEM:WINDOWS")
  if ("${PLATFORM_ID}" MATCHES "x64")
    set (LINK_FLAGS "${LINK_FLAGS},5.02")
  elseif (NOT "${PLATFORM_ID}" MATCHES "arm")
    set (LINK_FLAGS "${LINK_FLAGS},5.01")
  endif ()

  if ("${CMAKE_BUILD_TYPE}" MATCHES "[dD]eb(ug)?")
    target_compile_options (${PROJECT_NAME} PRIVATE -Zi -Od -D_DEBUG)
    if (NMAKE_BUILD)
      set (LINK_FLAGS "${LINK_FLAGS};/LTCG:INCREMENTAL")
    endif ()
  else ()
    target_compile_options (${PROJECT_NAME} PRIVATE -Gw -GL -GR- -O2 -WX)
    if ("${PLATFORM_ID}" MATCHES "arm")
      set (LINK_FLAGS "${LINK_FLAGS};/OPT:REF;/LTCG:INCREMENTAL")
    else()
      set (LINK_FLAGS "${LINK_FLAGS};/OPT:REF,ICF;/LTCG")
    endif ()
  endif ()
  target_link_options (${PROJECT_NAME} PRIVATE ${LINK_FLAGS})
else ()
  target_compile_options (${PROJECT_NAME} PRIVATE
    -std=c++17
    -Wall
    -Werror
    -Werror=init-self
    -Werror=uninitialized
    -Wno-overloaded-virtual
    -Wno-conversion-null
    -fmessage-length=0
  )
endif (VC_BUILD)
