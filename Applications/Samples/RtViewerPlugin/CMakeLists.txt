# Stone of Orthanc
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2023 Osimis S.A., Belgium
# Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.


cmake_minimum_required(VERSION 2.8.3...4.0)

project(StoneWebViewerPlugin)

set(ORTHANC_PLUGIN_VERSION "mainline")

if (ORTHANC_PLUGIN_VERSION STREQUAL "mainline")
  set(ORTHANC_FRAMEWORK_DEFAULT_VERSION "mainline")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "hg")
else()
  set(ORTHANC_FRAMEWORK_DEFAULT_VERSION "1.12.4")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "web")
endif()

set(ORTHANC_STONE_BINARIES "${CMAKE_SOURCE_DIR}/../../../wasm-binaries/RtViewer/" CACHE PATH "Path to the binaries of the \"../WebAssembly\" folder")
 
# Parameters of the build
set(STATIC_BUILD OFF CACHE BOOL "Static build of the third-party libraries (necessary for Windows)")
set(ALLOW_DOWNLOADS OFF CACHE BOOL "Allow CMake to download packages")
set(ORTHANC_FRAMEWORK_SOURCE "${ORTHANC_FRAMEWORK_DEFAULT_SOURCE}" CACHE STRING "Source of the Orthanc framework (can be \"system\", \"hg\", \"archive\", \"web\" or \"path\")")
set(ORTHANC_FRAMEWORK_VERSION "${ORTHANC_FRAMEWORK_DEFAULT_VERSION}" CACHE STRING "Version of the Orthanc framework")
set(ORTHANC_FRAMEWORK_ARCHIVE "" CACHE STRING "Path to the Orthanc archive, if ORTHANC_FRAMEWORK_SOURCE is \"archive\"")
set(ORTHANC_FRAMEWORK_ROOT "" CACHE STRING "Path to the Orthanc source directory, if ORTHANC_FRAMEWORK_SOURCE is \"path\"")


# Advanced parameters to fine-tune linking against system libraries
set(USE_SYSTEM_ORTHANC_SDK ON CACHE BOOL "Use the system version of the Orthanc plugin SDK")
set(ORTHANC_FRAMEWORK_STATIC OFF CACHE BOOL "If linking against the Orthanc framework system library, indicates whether this library was statically linked")
mark_as_advanced(ORTHANC_FRAMEWORK_STATIC)


# Download and setup the Orthanc framework
include(${CMAKE_SOURCE_DIR}/../../../OrthancStone/Resources/Orthanc/CMake/DownloadOrthancFramework.cmake)

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "system")
  if (ORTHANC_FRAMEWORK_USE_SHARED)
    include(FindBoost)
    find_package(Boost COMPONENTS system)
    
    if (NOT Boost_FOUND)
      message(FATAL_ERROR "Unable to locate Boost on this system")
    endif()
    
    link_libraries(${Boost_LIBRARIES} jsoncpp)
  endif()

  link_libraries(${ORTHANC_FRAMEWORK_LIBRARIES})

else()
  include(${ORTHANC_FRAMEWORK_ROOT}/../Resources/CMake/OrthancFrameworkParameters.cmake)

  set(ENABLE_MODULE_IMAGES OFF)
  set(ENABLE_MODULE_JOBS OFF)
  set(ENABLE_MODULE_DICOM OFF)

  include(${ORTHANC_FRAMEWORK_ROOT}/../Resources/CMake/OrthancFrameworkConfiguration.cmake)
  include_directories(${ORTHANC_FRAMEWORK_ROOT})
endif()

include(${CMAKE_SOURCE_DIR}/../../../RenderingPlugin/Resources/Orthanc/Plugins/OrthancPluginsExports.cmake)


if (STATIC_BUILD OR NOT USE_SYSTEM_ORTHANC_SDK)
  include_directories(${CMAKE_SOURCE_DIR}/../../../RenderingPlugin/Resources/Orthanc/Sdk-1.0.0)
else ()
  CHECK_INCLUDE_FILE_CXX(orthanc/OrthancCPlugin.h HAVE_ORTHANC_H)
  if (NOT HAVE_ORTHANC_H)
    message(FATAL_ERROR "Please install the headers of the Orthanc plugins SDK")
  endif()
endif()


add_definitions(
  -DHAS_ORTHANC_EXCEPTION=1
  -DPLUGIN_VERSION="${ORTHANC_PLUGIN_VERSION}"
  -DPLUGIN_NAME="stone-rtviewer"
  )


EmbedResources(
  # Web Viewer Folders
  # IMAGES                 ${ORTHANC_STONE_BINARIES_WEB_VIEWER}/img/
  # WEB_APPLICATION        ${CMAKE_SOURCE_DIR}/../WebApplication

  # Explorer extension code
  ORTHANC_EXPLORER       ${CMAKE_SOURCE_DIR}/OrthancExplorer.js

  # RtViewer individual files
  RT_VIEWER_WASM_JS      ${ORTHANC_STONE_BINARIES}/RtViewerWasm.js
  RT_VIEWER_WASM         ${ORTHANC_STONE_BINARIES}/RtViewerWasm.wasm
  RT_VIEWER_WASM_APP_JS  ${ORTHANC_STONE_BINARIES}/RtViewerWasmApp.js
  RT_VIEWER_INDEX_HTML   ${ORTHANC_STONE_BINARIES}/index.html
  )

add_library(RtViewerPlugin SHARED
  Plugin.cpp
  ${AUTOGENERATED_SOURCES}
  ${CMAKE_SOURCE_DIR}/../../../RenderingPlugin/Resources/Orthanc/Plugins/OrthancPluginCppWrapper.cpp
  ${ORTHANC_CORE_SOURCES}
  )

if (COMMAND DefineSourceBasenameForTarget)
  DefineSourceBasenameForTarget(RtViewerPlugin)
endif()

set_target_properties(RtViewerPlugin PROPERTIES 
  VERSION ${ORTHANC_PLUGIN_VERSION} 
  SOVERSION ${ORTHANC_PLUGIN_VERSION})

install(
  TARGETS RtViewerPlugin
  RUNTIME DESTINATION lib    # Destination for Windows
  LIBRARY DESTINATION share/orthanc/plugins    # Destination for Linux
  )
