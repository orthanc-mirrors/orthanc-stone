# Stone of Orthanc
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2020 Osimis S.A., Belgium
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



#####################################################################
## Select the location of the Orthanc framework
#####################################################################

include(${CMAKE_CURRENT_LIST_DIR}/Version.cmake)

if (ORTHANC_STONE_VERSION STREQUAL "mainline")
  set(ORTHANC_FRAMEWORK_DEFAULT_VERSION "mainline")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "hg")
else()
  set(ORTHANC_FRAMEWORK_DEFAULT_VERSION "1.7.2")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "web")
endif()

# Parameters of the build
set(STATIC_BUILD OFF CACHE BOOL "Static build of the third-party libraries (necessary for Windows)")
set(ALLOW_DOWNLOADS OFF CACHE BOOL "Allow CMake to download packages")
set(ORTHANC_FRAMEWORK_SOURCE "${ORTHANC_FRAMEWORK_DEFAULT_SOURCE}" CACHE STRING "Source of the Orthanc framework (can be \"system\", \"hg\", \"archive\", \"web\" or \"path\")")
set(ORTHANC_FRAMEWORK_VERSION "${ORTHANC_FRAMEWORK_DEFAULT_VERSION}" CACHE STRING "Version of the Orthanc framework")
set(ORTHANC_FRAMEWORK_ARCHIVE "" CACHE STRING "Path to the Orthanc archive, if ORTHANC_FRAMEWORK_SOURCE is \"archive\"")
set(ORTHANC_FRAMEWORK_ROOT "" CACHE STRING "Path to the Orthanc source directory, if ORTHANC_FRAMEWORK_SOURCE is \"path\"")

# Advanced parameters to fine-tune linking against system libraries
set(ORTHANC_FRAMEWORK_STATIC OFF CACHE BOOL "If linking against the Orthanc framework system library, indicates whether this library was statically linked")
mark_as_advanced(ORTHANC_FRAMEWORK_STATIC)



#####################################################################
## Import the parameters of the Orthanc Framework
#####################################################################

if (NOT ORTHANC_FRAMEWORK_SOURCE STREQUAL "system")
  include(${CMAKE_CURRENT_LIST_DIR}/../Orthanc/CMake/DownloadOrthancFramework.cmake)
  include(${ORTHANC_FRAMEWORK_ROOT}/Resources/CMake/OrthancFrameworkParameters.cmake)
  
  unset(STANDALONE_BUILD CACHE)
  set(STANDALONE_BUILD ON)       # Embed DCMTK's dictionaries in static builds

  set(ENABLE_DCMTK OFF)
  set(ENABLE_GOOGLE_TEST ON)
  set(ENABLE_JPEG ON)
  set(ENABLE_OPENSSL_ENGINES ON)
  set(ENABLE_PNG ON)
  set(ENABLE_SQLITE OFF)
  set(ENABLE_ZLIB ON)
endif()
  


#####################################################################
## CMake parameters tunable by the user
#####################################################################

# Advanced parameters to fine-tune linking against system libraries
set(USE_SYSTEM_CAIRO ON CACHE BOOL "Use the system version of Cairo")
set(USE_SYSTEM_FREETYPE ON CACHE BOOL "Use the system version of Freetype")
set(USE_SYSTEM_GLEW ON CACHE BOOL "Use the system version of glew (for Windows only)")
set(USE_SYSTEM_PIXMAN ON CACHE BOOL "Use the system version of Pixman")
set(USE_SYSTEM_SDL ON CACHE BOOL "Use the system version of SDL2")



#####################################################################
## Internal CMake parameters to enable the optional subcomponents of
## the Stone of Orthanc
#####################################################################

set(ENABLE_OPENGL ON CACHE BOOL "Enable support of OpenGL")
set(ENABLE_WASM OFF CACHE INTERNAL "Enable support of WebAssembly")
set(ENABLE_GUIADAPTER OFF CACHE INTERNAL "Enable backward compatibility with the Stone GuiAdapter class")
