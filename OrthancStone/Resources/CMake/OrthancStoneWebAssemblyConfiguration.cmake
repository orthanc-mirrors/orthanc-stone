# Stone of Orthanc
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2022 Osimis S.A., Belgium
# Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program. If not, see
# <http://www.gnu.org/licenses/>.




#####################################################################
## Sanity check of the configuration
#####################################################################

set(ENABLE_WEB_CLIENT OFF)
include(${CMAKE_CURRENT_LIST_DIR}/OrthancStoneConfiguration.cmake)

if (NOT ORTHANC_SANDBOXED)
  message(FATAL_ERROR "WebAssembly target must me configured as sandboxed")
endif()

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message(FATAL_ERROR "WebAssembly target requires the emscripten compiler")    
endif()

add_definitions(
  -DORTHANC_ENABLE_SDL=0
  -DORTHANC_ENABLE_WASM=1
  )


#####################################################################
## Additional source files for WebAssembly
#####################################################################

list(APPEND ORTHANC_STONE_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/../../Sources/Platforms/WebAssembly/WebAssemblyCairoViewport.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../../Sources/Platforms/WebAssembly/WebAssemblyLoadersContext.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../../Sources/Platforms/WebAssembly/WebAssemblyOracle.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../../Sources/Platforms/WebAssembly/WebAssemblyViewport.cpp
  )

if (ENABLE_OPENGL)
  list(APPEND ORTHANC_STONE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../Sources/Platforms/WebAssembly/WebAssemblyOpenGLContext.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../Sources/Platforms/WebAssembly/WebGLViewport.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../Sources/Platforms/WebAssembly/WebGLViewportsRegistry.cpp
    )
endif()