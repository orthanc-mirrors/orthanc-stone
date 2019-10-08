# Stone of Orthanc
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2019 Osimis S.A., Belgium
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


set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)

if ("${CMAKE_SYSTEM_VERSION}" STREQUAL "LinuxStandardBase")
  # Linux Standard Base version 5 ships Qt 4.2.3
  include(Qt4Macros)

  set(QT_UIC_EXECUTABLE ${LSB_PATH}/bin/uic)
  set(QT_MOC_EXECUTABLE ${LSB_PATH}/bin/moc)

  include_directories(
    ${LSB_PATH}/include/QtCore
    ${LSB_PATH}/include/QtGui
    ${LSB_PATH}/include/QtOpenGL
    )

  macro(ORTHANC_QT_WRAP_UI)
    QT4_WRAP_UI(${ARGN})
  endmacro()
  
  macro(ORTHANC_QT_WRAP_CPP)
    QT4_WRAP_CPP(${ARGN})
  endmacro()  

  link_libraries(QtCore QtGui QtOpenGL)
  
else()
  # Not using Linux Standard Base
  # Find the QtWidgets library
  find_package(Qt5Widgets QUIET)

  if (Qt5Widgets_FOUND)
    message("Qt5 has been detected")
    find_package(Qt5Core REQUIRED)
    link_libraries(
      Qt5::Widgets
      Qt5::Core
      )

    if (ENABLE_OPENGL)
      find_package(Qt5OpenGL REQUIRED)
      link_libraries(
        Qt5::OpenGL
        )
    endif()
    
    # Create aliases for the CMake commands
    macro(ORTHANC_QT_WRAP_UI)
      QT5_WRAP_UI(${ARGN})
    endmacro()
    
    macro(ORTHANC_QT_WRAP_CPP)
      QT5_WRAP_CPP(${ARGN})
    endmacro()

  else()
    message("Qt5 has not been found, trying with Qt4")
    find_package(Qt4 REQUIRED QtGui)
    link_libraries(
      Qt4::QtGui
      )

    if (ENABLE_OPENGL)
      find_package(Qt4 REQUIRED QtOpenGL)
      link_libraries(
        Qt4::QtOpenGL
        )
    endif()
    
    # Create aliases for the CMake commands
    macro(ORTHANC_QT_WRAP_UI)
      QT4_WRAP_UI(${ARGN})
    endmacro()
    
    macro(ORTHANC_QT_WRAP_CPP)
      QT4_WRAP_CPP(${ARGN})
    endmacro()  
  endif()
endif()


if (ENABLE_STONE_DEPRECATED)
  list(APPEND QT_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Qt/QCairoWidget.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Qt/QStoneMainWindow.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Qt/QtStoneApplicationRunner.cpp
    )

  ORTHANC_QT_WRAP_CPP(QT_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Qt/QCairoWidget.h
    ${ORTHANC_STONE_ROOT}/Applications/Qt/QStoneMainWindow.h
    )
endif()
  

# NB: Including CMAKE_CURRENT_BINARY_DIR is mandatory, as the CMake
# macros for Qt will put their result in that directory, which cannot
# be changed.
# https://stackoverflow.com/a/4016784/881731

include_directories(
  ${ORTHANC_STONE_ROOT}/Applications/Qt/
  ${CMAKE_CURRENT_BINARY_DIR}
  )

