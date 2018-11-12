# Stone of Orthanc
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2018 Osimis S.A., Belgium
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
## Configure the Orthanc Framework
#####################################################################

if (ENABLE_DCMTK)
  set(ENABLE_LOCALE ON)
else()
  set(ENABLE_LOCALE OFF)  # Disable support for locales (notably in Boost)
endif()

include(${ORTHANC_ROOT}/Resources/CMake/OrthancFrameworkConfiguration.cmake)
include_directories(${ORTHANC_ROOT})


#####################################################################
## Sanity check of the configuration
#####################################################################

if (ORTHANC_SANDBOXED)
  if (ENABLE_CURL)
    message(FATAL_ERROR "Cannot enable curl in sandboxed environments")
  endif()

  if (ENABLE_SDL)
    message(FATAL_ERROR "Cannot enable SDL in sandboxed environments")
  endif()

  if (ENABLE_QT)
    message(FATAL_ERROR "Cannot enable QT in sandboxed environments")
  endif()

  if (ENABLE_SSL)
    message(FATAL_ERROR "Cannot enable SSL in sandboxed environments")
  endif()
endif()
  

#####################################################################
## Configure mandatory third-party components
#####################################################################

SET(ORTHANC_STONE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

include(FindPkgConfig)
include(${CMAKE_CURRENT_LIST_DIR}/BoostExtendedConfiguration.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CairoConfiguration.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/PixmanConfiguration.cmake)



#####################################################################
## Configure optional third-party components
#####################################################################

if (NOT ORTHANC_SANDBOXED)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_ROOT}/Plugins/Samples/Common/OrthancHttpConnection.cpp
    )
endif()


if (ENABLE_SDL AND ENABLE_QT)
  message("SDL and QT may not be defined together")
elseif(ENABLE_SDL)
  message("SDL is enabled")
  include(${CMAKE_CURRENT_LIST_DIR}/SdlConfiguration.cmake)
  add_definitions(-DORTHANC_ENABLE_NATIVE=1)
  add_definitions(-DORTHANC_ENABLE_QT=0)
  add_definitions(-DORTHANC_ENABLE_SDL=1)
elseif(ENABLE_QT)
  message("QT is enabled")
  include(${CMAKE_CURRENT_LIST_DIR}/QtConfiguration.cmake)
  add_definitions(-DORTHANC_ENABLE_NATIVE=1)
  add_definitions(-DORTHANC_ENABLE_QT=1)
  add_definitions(-DORTHANC_ENABLE_SDL=0)
else()
  message("SDL and QT are both disabled")
  unset(USE_SYSTEM_SDL CACHE)
  add_definitions(-DORTHANC_ENABLE_SDL=0)
  add_definitions(-DORTHANC_ENABLE_QT=0)
  add_definitions(-DORTHANC_ENABLE_NATIVE=0)
endif()



#####################################################################
## Configuration of the C/C++ macros
#####################################################################

if (MSVC)
  # Remove some warnings on Visual Studio 2015
  add_definitions(-D_SCL_SECURE_NO_WARNINGS=1) 
endif()

add_definitions(
  -DHAS_ORTHANC_EXCEPTION=1
  -DORTHANC_ENABLE_LOGGING_PLUGIN=0
  )

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DCHECK_OBSERVERS_MESSAGES)
endif()

#####################################################################
## Embed the colormaps into the binaries
#####################################################################

EmbedResources(
  # Resources coming from the core of Orthanc. They must be copied
  # here, as HAS_EMBEDDED_RESOURCES is set to ON in
  # "OrthancStoneParameters.cmake"
  ${DCMTK_DICTIONARIES}

  FONT_UBUNTU_MONO_BOLD_16   ${ORTHANC_ROOT}/Resources/Fonts/UbuntuMonoBold-16.json
  #FONT_UBUNTU_MONO_BOLD_64   ${ORTHANC_ROOT}/Resources/Fonts/UbuntuMonoBold-64.json

  # Resources specific to the Stone of Orthanc
  COLORMAP_HOT    ${ORTHANC_STONE_ROOT}/Resources/Colormaps/hot.lut
  COLORMAP_JET    ${ORTHANC_STONE_ROOT}/Resources/Colormaps/jet.lut
  COLORMAP_RED    ${ORTHANC_STONE_ROOT}/Resources/Colormaps/red.lut
  COLORMAP_GREEN  ${ORTHANC_STONE_ROOT}/Resources/Colormaps/green.lut
  COLORMAP_BLUE   ${ORTHANC_STONE_ROOT}/Resources/Colormaps/blue.lut
  )



#####################################################################
## System-specific patches
#####################################################################

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND
    NOT MSVC AND
    ENABLE_SDL)
  # This is necessary when compiling EXE for Windows using MinGW
  link_libraries(mingw32)
endif()

if (ORTHANC_SANDBOXED)
  # Remove functions not suitable for a sandboxed environment
  list(REMOVE_ITEM ORTHANC_CORE_SOURCES
    ${ZLIB_SOURCES_DIR}/gzlib.c
    ${ZLIB_SOURCES_DIR}/gzwrite.c
    ${ZLIB_SOURCES_DIR}/gzread.c
    )
endif()



#####################################################################
## All the source files required to build Stone of Orthanc
#####################################################################

set(APPLICATIONS_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/IStoneApplication.h
    ${ORTHANC_STONE_ROOT}/Applications/StoneApplicationContext.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Commands/BaseCommandBuilder.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Commands/ICommand.h
    ${ORTHANC_STONE_ROOT}/Applications/Commands/ICommandExecutor.h
    ${ORTHANC_STONE_ROOT}/Applications/Commands/ICommandBuilder.h
    )

if (NOT ORTHANC_SANDBOXED)
  set(PLATFORM_SOURCES
    ${ORTHANC_STONE_ROOT}/Framework/Viewport/CairoFont.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/WebServiceCommandBase.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/WebServiceGetCommand.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/WebServicePostCommand.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/WebServiceDeleteCommand.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/Oracle.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/OracleWebService.h
    )

  if (ENABLE_SDL OR ENABLE_QT)
    list(APPEND APPLICATIONS_SOURCES
      ${ORTHANC_STONE_ROOT}/Applications/Generic/NativeStoneApplicationRunner.cpp
      ${ORTHANC_STONE_ROOT}/Applications/Generic/NativeStoneApplicationContext.cpp
      )
    if (ENABLE_SDL)
      list(APPEND APPLICATIONS_SOURCES
        ${ORTHANC_STONE_ROOT}/Applications/Sdl/SdlStoneApplicationRunner.cpp
        ${ORTHANC_STONE_ROOT}/Applications/Sdl/SdlEngine.cpp
        ${ORTHANC_STONE_ROOT}/Applications/Sdl/SdlCairoSurface.cpp
        ${ORTHANC_STONE_ROOT}/Applications/Sdl/SdlOrthancSurface.cpp
        ${ORTHANC_STONE_ROOT}/Applications/Sdl/SdlWindow.cpp
        )
    endif()
  endif()
elseif (ENABLE_WASM)
  list(APPEND APPLICATIONS_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Wasm/StartupParametersBuilder.cpp
    )

  set(STONE_WASM_SOURCES
    ${ORTHANC_STONE_ROOT}/Platforms/Wasm/Defaults.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Wasm/WasmWebService.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Wasm/WasmViewport.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Wasm/WasmPlatformApplicationAdapter.cpp
    ${AUTOGENERATED_DIR}/WasmWebService.c
    ${AUTOGENERATED_DIR}/default-library.c
  )

  # Regenerate a dummy "WasmWebService.c" file each time the "WasmWebService.js" file
  # is modified, so as to force a new execution of the linking
  add_custom_command(
    OUTPUT "${AUTOGENERATED_DIR}/WasmWebService.c"
    COMMAND ${CMAKE_COMMAND} -E touch "${AUTOGENERATED_DIR}/WasmWebService.c" ""
    DEPENDS "${ORTHANC_STONE_ROOT}/Platforms/Wasm/WasmWebService.js")
  add_custom_command(
    OUTPUT "${AUTOGENERATED_DIR}/default-library.c"
    COMMAND ${CMAKE_COMMAND} -E touch "${AUTOGENERATED_DIR}/default-library.c" ""
    DEPENDS "${ORTHANC_STONE_ROOT}/Platforms/Wasm/default-library.js")
endif()

list(APPEND ORTHANC_STONE_SOURCES
  #${ORTHANC_STONE_ROOT}/Framework/Layers/SeriesFrameRendererFactory.cpp
  #${ORTHANC_STONE_ROOT}/Framework/Layers/SingleFrameRendererFactory.cpp

  ${ORTHANC_STONE_ROOT}/Framework/Layers/CircleMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Layers/ColorFrameRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Layers/DicomSeriesVolumeSlicer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Layers/DicomStructureSetSlicer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Layers/FrameRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Layers/GrayscaleFrameRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Layers/IVolumeSlicer.h
  ${ORTHANC_STONE_ROOT}/Framework/Layers/LineLayerRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Layers/LineMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Layers/RenderStyle.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Layers/SliceOutlineRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyScene.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyScene.h
  ${ORTHANC_STONE_ROOT}/Framework/SmartLoader.cpp
  ${ORTHANC_STONE_ROOT}/Framework/StoneEnumerations.cpp
  ${ORTHANC_STONE_ROOT}/Framework/StoneException.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/AffineTransform2D.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/AffineTransform2D.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/CoordinateSystem3D.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomFrameConverter.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructureSet.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DownloadStack.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/Extent2D.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/FiniteProjectiveCamera.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/GeometryToolbox.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/IWebService.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/IWebService.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/ImageGeometry.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/LinearAlgebra.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/MessagingToolbox.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/OrientedBoundingBox.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/OrthancApiClient.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/OrthancSlicesLoader.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/ParallelSlices.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/ParallelSlicesCursor.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/ShearWarpProjectiveTransform.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/Slice.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/SlicesSorter.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/UndoRedoStack.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/UndoRedoStack.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/ViewportGeometry.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Viewport/CairoContext.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Viewport/CairoSurface.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Viewport/IStatusBar.h
  ${ORTHANC_STONE_ROOT}/Framework/Viewport/IViewport.h
  ${ORTHANC_STONE_ROOT}/Framework/Viewport/WidgetViewport.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/ImageBuffer3D.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/StructureSetLoader.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/VolumeReslicer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/CairoWidget.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/EmptyWidget.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/IWidget.h
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/IWorldSceneInteractor.h
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/IWorldSceneMouseTracker.h
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/LayoutWidget.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/PanMouseTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/SliceViewerWidget.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/TestCairoWidget.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/TestWorldSceneWidget.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/WidgetBase.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/WorldSceneWidget.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Widgets/ZoomMouseTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/dev.h

  ${ORTHANC_STONE_ROOT}/Framework/Messages/ICallable.h
  ${ORTHANC_STONE_ROOT}/Framework/Messages/IMessage.h
  ${ORTHANC_STONE_ROOT}/Framework/Messages/IObservable.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Messages/IObservable.h
  ${ORTHANC_STONE_ROOT}/Framework/Messages/IObserver.h
  ${ORTHANC_STONE_ROOT}/Framework/Messages/MessageBroker.h
  ${ORTHANC_STONE_ROOT}/Framework/Messages/MessageForwarder.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Messages/Promise.h

  ${ORTHANC_ROOT}/Plugins/Samples/Common/DicomDatasetReader.cpp
  ${ORTHANC_ROOT}/Plugins/Samples/Common/DicomPath.cpp
  ${ORTHANC_ROOT}/Plugins/Samples/Common/FullOrthancDataset.cpp
  ${ORTHANC_ROOT}/Plugins/Samples/Common/IOrthancConnection.cpp
  
  ${PLATFORM_SOURCES}
  ${APPLICATIONS_SOURCES}
  ${ORTHANC_CORE_SOURCES}
  ${ORTHANC_DICOM_SOURCES}
  ${AUTOGENERATED_SOURCES}

  # Mandatory components
  ${CAIRO_SOURCES}
  ${PIXMAN_SOURCES}

  # Optional components
  ${SDL_SOURCES}
  ${QT_SOURCES}
  ${BOOST_EXTENDED_SOURCES}
  )

