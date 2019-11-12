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


## Note that these set of macros MUST be defined as a "function()",
## otherwise it fails
function(DEFINE_QT_MACROS)
  include(Qt4Macros)

  ##
  ## This part is adapted from file "Qt4Macros.cmake" shipped with
  ## CMake 3.5.1, released under the following license:
  ##
  ##=============================================================================
  ## Copyright 2005-2009 Kitware, Inc.
  ##
  ## Distributed under the OSI-approved BSD License (the "License");
  ## see accompanying file Copyright.txt for details.
  ##
  ## This software is distributed WITHOUT ANY WARRANTY; without even the
  ## implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  ## See the License for more information.
  ##=============================================================================
  ## 
  macro (ORTHANC_QT_WRAP_UI outfiles)
    QT4_EXTRACT_OPTIONS(ui_files ui_options ui_target ${ARGN})
    foreach (it ${ui_files})
      get_filename_component(outfile ${it} NAME_WE)
      get_filename_component(infile ${it} ABSOLUTE)
      set(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.h)
      add_custom_command(OUTPUT ${outfile}
        COMMAND ${QT_UIC_EXECUTABLE}
        ARGS ${ui_options} -o ${outfile} ${infile}
        MAIN_DEPENDENCY ${infile} VERBATIM)
      set(${outfiles} ${${outfiles}} ${outfile})
    endforeach ()
  endmacro ()
  
  macro (ORTHANC_QT_WRAP_CPP outfiles )
    QT4_GET_MOC_FLAGS(moc_flags)
    QT4_EXTRACT_OPTIONS(moc_files moc_options moc_target ${ARGN})
    foreach (it ${moc_files})
      get_filename_component(outfile ${it} NAME_WE)
      get_filename_component(infile ${it} ABSOLUTE)
      set(outfile ${CMAKE_CURRENT_BINARY_DIR}/moc_${outfile}.cxx)
      add_custom_command(OUTPUT ${outfile}
        COMMAND ${QT_MOC_EXECUTABLE}
        ARGS ${infile} "${moc_flags}" -o ${outfile}
        MAIN_DEPENDENCY ${infile} VERBATIM)
      set(${outfiles} ${${outfiles}} ${outfile})
    endforeach ()
  endmacro ()
  ##
  ## End of "Qt4Macros.cmake" adaptation.
  ##
endfunction()


if ("${CMAKE_SYSTEM_VERSION}" STREQUAL "LinuxStandardBase")
  # Linux Standard Base version 5 ships Qt 4.2.3
  DEFINE_QT_MACROS()
 
  # The script "LinuxStandardBaseUic.py" is just a wrapper around the
  # "uic" compiler from LSB that does not support the "<?xml ...?>"
  # header that is automatically added by Qt Creator
  set(QT_UIC_EXECUTABLE ${CMAKE_CURRENT_LIST_DIR}/LinuxStandardBaseUic.py)

  set(QT_MOC_EXECUTABLE ${LSB_PATH}/bin/moc)

  include_directories(
    ${LSB_PATH}/include/QtCore
    ${LSB_PATH}/include/QtGui
    ${LSB_PATH}/include/QtOpenGL
    )

  link_libraries(QtCore QtGui QtOpenGL)

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  DEFINE_QT_MACROS()
  
  include_directories(${QT5_INSTALL_ROOT}/include)
  link_directories(${QT5_INSTALL_ROOT}/lib)

  if (OFF) #CMAKE_CROSSCOMPILING)
    set(QT_UIC_EXECUTABLE wine ${QT5_INSTALL_ROOT}/bin/uic.exe)
    set(QT_MOC_EXECUTABLE wine ${QT5_INSTALL_ROOT}/bin/moc.exe)
  else()
    set(QT_UIC_EXECUTABLE ${QT5_INSTALL_ROOT}/bin/uic)
    set(QT_MOC_EXECUTABLE ${QT5_INSTALL_ROOT}/bin/moc)
  endif()

  include_directories(
    ${QT5_INSTALL_ROOT}/include/QtCore
    ${QT5_INSTALL_ROOT}/include/QtGui
    ${QT5_INSTALL_ROOT}/include/QtOpenGL
    ${QT5_INSTALL_ROOT}/include/QtWidgets
    )

  if (OFF)
    # Dynamic Qt
    link_libraries(Qt5Core Qt5Gui Qt5OpenGL Qt5Widgets)

    file(COPY
      ${QT5_INSTALL_ROOT}/bin/Qt5Core.dll
      ${QT5_INSTALL_ROOT}/bin/Qt5Gui.dll
      ${QT5_INSTALL_ROOT}/bin/Qt5OpenGL.dll
      ${QT5_INSTALL_ROOT}/bin/Qt5Widgets.dll
      ${QT5_INSTALL_ROOT}/bin/libstdc++-6.dll
      ${QT5_INSTALL_ROOT}/bin/libgcc_s_dw2-1.dll
      ${QT5_INSTALL_ROOT}/bin/libwinpthread-1.dll
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    file(COPY
      ${QT5_INSTALL_ROOT}/plugins/platforms/qwindows.dll
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/platforms)

  else()
    # Static Qt
    link_libraries(
      ${QT5_INSTALL_ROOT}/lib/libQt5Widgets.a
      ${QT5_INSTALL_ROOT}/lib/libQt5Gui.a
      ${QT5_INSTALL_ROOT}/lib/libQt5OpenGL.a
      ${QT5_INSTALL_ROOT}/lib/libQt5Core.a
      ${QT5_INSTALL_ROOT}/lib/libqtharfbuzz.a
      ${QT5_INSTALL_ROOT}/lib/libqtpcre2.a
      ${QT5_INSTALL_ROOT}/lib/libQt5FontDatabaseSupport.a
      ${QT5_INSTALL_ROOT}/lib/libQt5EventDispatcherSupport.a
      ${QT5_INSTALL_ROOT}/lib/libQt5ThemeSupport.a
      ${QT5_INSTALL_ROOT}/plugins/platforms/libqwindows.a
      winmm
      version
      ws2_32
      uxtheme
      imm32
      dwmapi
      )
  endif()
  
else()
  # Not using Windows, not using Linux Standard Base, 
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

