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
## Configure the Orthanc Framework
#####################################################################

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "system")
  include(${CMAKE_CURRENT_LIST_DIR}/../Orthanc/CMake/DownloadOrthancFramework.cmake)
  link_libraries(${ORTHANC_FRAMEWORK_LIBRARIES})

  # Switch to the C++11 standard if the version of JsonCpp is 1.y.z
  if (EXISTS ${JSONCPP_INCLUDE_DIR}/json/version.h)
    file(STRINGS
      "${JSONCPP_INCLUDE_DIR}/json/version.h" 
      JSONCPP_VERSION_MAJOR1 REGEX
      ".*define JSONCPP_VERSION_MAJOR.*")

    if (NOT JSONCPP_VERSION_MAJOR1)
      message(FATAL_ERROR "Unable to extract the major version of JsonCpp")
    endif()
    
    string(REGEX REPLACE
      ".*JSONCPP_VERSION_MAJOR.*([0-9]+)$" "\\1" 
      JSONCPP_VERSION_MAJOR ${JSONCPP_VERSION_MAJOR1})
    message("JsonCpp major version: ${JSONCPP_VERSION_MAJOR}")

    if (JSONCPP_VERSION_MAJOR GREATER 0)
      message("Switching to C++11 standard")
      if (CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
      elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
      endif()
    endif()
  endif()
  
else()
  if (ENABLE_DCMTK)
    set(ENABLE_LOCALE ON)
  else()
    if (NOT DEFINED ENABLE_LOCALE)
      set(ENABLE_LOCALE OFF)  # Disable support for locales (notably in Boost)
    endif()
  endif()
  
  include(${ORTHANC_FRAMEWORK_ROOT}/Resources/CMake/OrthancFrameworkConfiguration.cmake)
  include_directories(
    ${ORTHANC_FRAMEWORK_ROOT}/Sources/
    )
endif()


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

  if (ENABLE_SSL)
    message(FATAL_ERROR "Cannot enable SSL in sandboxed environments")
  endif()
endif()

if (ENABLE_OPENGL)
  if (NOT ENABLE_SDL AND NOT ENABLE_WASM)
    message(FATAL_ERROR "Cannot enable OpenGL if WebAssembly and SDL are both disabled")
  endif()
endif()

if (ENABLE_WASM)
  if (NOT ORTHANC_SANDBOXED)
    message(FATAL_ERROR "WebAssembly target must me configured as sandboxed")
  endif()

  if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(FATAL_ERROR "WebAssembly target requires the emscripten compiler")    
  endif()

  set(ENABLE_THREADS OFF)
  add_definitions(-DORTHANC_ENABLE_WASM=1)
else()
  if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR
      CMAKE_SYSTEM_NAME STREQUAL "PNaCl" OR
      CMAKE_SYSTEM_NAME STREQUAL "NaCl32" OR
      CMAKE_SYSTEM_NAME STREQUAL "NaCl64")
    message(FATAL_ERROR "Trying to use a Web compiler for a native build")
  endif()

  set(ENABLE_THREADS ON)
  add_definitions(-DORTHANC_ENABLE_WASM=0)
endif()
  

#####################################################################
## Configure mandatory third-party components
#####################################################################

include(FindPkgConfig)
include(${CMAKE_CURRENT_LIST_DIR}/CairoConfiguration.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/FreetypeConfiguration.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/PixmanConfiguration.cmake)



#####################################################################
## Configure optional third-party components
#####################################################################

if (NOT ORTHANC_SANDBOXED)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_STONE_ROOT}/Sources/Toolbox/OrthancDatasets/OrthancHttpConnection.cpp
    )
endif()


if(ENABLE_SDL)
  message("SDL is enabled")
  include(${CMAKE_CURRENT_LIST_DIR}/SdlConfiguration.cmake)
  add_definitions(
    -DORTHANC_ENABLE_SDL=1
    )
else()
  message("SDL is disabled")
  unset(USE_SYSTEM_SDL CACHE)
  add_definitions(
    -DORTHANC_ENABLE_SDL=0
    )
endif()


if (ENABLE_THREADS)
  add_definitions(-DORTHANC_ENABLE_THREADS=1)
else()
  add_definitions(-DORTHANC_ENABLE_THREADS=0)
endif()


if (ENABLE_OPENGL AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
  include(${CMAKE_CURRENT_LIST_DIR}/GlewConfiguration.cmake)
  add_definitions(
    -DORTHANC_ENABLE_GLEW=1
    )
else()
  add_definitions(
    -DORTHANC_ENABLE_GLEW=0
    )
endif()


if (ENABLE_OPENGL)
  if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # If including "FindOpenGL.cmake" using Emscripten (targeting
    # WebAssembly), the "OPENGL_LIBRARIES" value incorrectly includes
    # the "nul" library, which leads to warning message in Emscripten:
    # 'shared:WARNING: emcc: cannot find library "nul"'.
    include(FindOpenGL)
    if (NOT OPENGL_FOUND)
      message(FATAL_ERROR "Cannot find OpenGL on your system")
    endif()

    link_libraries(${OPENGL_LIBRARIES})
  endif()

  add_definitions(
    -DORTHANC_ENABLE_OPENGL=1
    )
else()
  add_definitions(-DORTHANC_ENABLE_OPENGL=0)  
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
  )

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DCHECK_OBSERVERS_MESSAGES)
endif()



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

if (NOT ORTHANC_SANDBOXED)
  set(PLATFORM_SOURCES
    ${ORTHANC_STONE_ROOT}/Sources/Loaders/GenericLoadersContext.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Loaders/GenericLoadersContext.h
    )

  if (ENABLE_SDL)
    list(APPEND ORTHANC_STONE_SOURCES
      ${ORTHANC_STONE_ROOT}/Sources/Viewport/SdlWindow.cpp
      ${ORTHANC_STONE_ROOT}/Sources/Viewport/SdlWindow.h
      )
  endif()

  if (ENABLE_SDL)
    if (ENABLE_OPENGL)
      list(APPEND ORTHANC_STONE_SOURCES
        ${ORTHANC_STONE_ROOT}/Sources/OpenGL/SdlOpenGLContext.cpp
        ${ORTHANC_STONE_ROOT}/Sources/OpenGL/SdlOpenGLContext.h
        ${ORTHANC_STONE_ROOT}/Sources/Viewport/SdlViewport.cpp
        ${ORTHANC_STONE_ROOT}/Sources/Viewport/SdlViewport.h
        )
    endif()
  endif()
endif()


if (ENABLE_DCMTK)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_STONE_ROOT}/Sources/Oracle/ParseDicomSuccessMessage.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Toolbox/ParsedDicomCache.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Toolbox/ParsedDicomDataset.cpp
    )
endif()

if (ENABLE_THREADS)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_STONE_ROOT}/Sources/Oracle/ThreadedOracle.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Oracle/GenericOracleRunner.cpp
    )
endif()


if (ENABLE_WASM)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_STONE_ROOT}/Sources/Loaders/WebAssemblyLoadersContext.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Oracle/WebAssemblyOracle.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Viewport/WebAssemblyCairoViewport.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Viewport/WebAssemblyViewport.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Viewport/WebAssemblyViewport.h
    )
endif()

if ((ENABLE_SDL OR ENABLE_WASM) AND ENABLE_GUIADAPTER)
  list(APPEND APPLICATIONS_SOURCES
    ${ORTHANC_STONE_ROOT}/Sources/Deprecated/GuiAdapter.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Deprecated/GuiAdapter.h
    )
endif()


list(APPEND ORTHANC_STONE_SOURCES
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/OrthancDatasets/DicomDatasetReader.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/OrthancDatasets/DicomPath.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/OrthancDatasets/FullOrthancDataset.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/OrthancDatasets/IOrthancConnection.cpp

  ${ORTHANC_STONE_ROOT}/Sources/Fonts/FontRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Fonts/Glyph.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Fonts/GlyphAlphabet.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Fonts/GlyphBitmapAlphabet.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Fonts/GlyphTextureAlphabet.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Fonts/TextBoundingBox.cpp

  ${ORTHANC_STONE_ROOT}/Sources/Loaders/BasicFetchingItemsSorter.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/BasicFetchingItemsSorter.h
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/BasicFetchingStrategy.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/BasicFetchingStrategy.h
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/DicomResourcesLoader.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/DicomSource.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/DicomStructureSetLoader.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/DicomStructureSetLoader.h
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/DicomVolumeLoader.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/IFetchingItemsSorter.h
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/IFetchingStrategy.h
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/LoadedDicomResources.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/LoaderCache.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/LoaderCache.h
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/LoaderStateMachine.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/LoaderStateMachine.h
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/OrthancMultiframeVolumeLoader.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/OrthancMultiframeVolumeLoader.h
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/OracleScheduler.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/OrthancSeriesVolumeProgressiveLoader.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/OrthancSeriesVolumeProgressiveLoader.h
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/SeriesFramesLoader.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/SeriesMetadataLoader.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/SeriesOrderedFrames.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Loaders/SeriesThumbnailsLoader.cpp

  ${ORTHANC_STONE_ROOT}/Sources/Messages/ICallable.h
  ${ORTHANC_STONE_ROOT}/Sources/Messages/IMessage.h
  ${ORTHANC_STONE_ROOT}/Sources/Messages/IMessageEmitter.h
  ${ORTHANC_STONE_ROOT}/Sources/Messages/IObservable.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Messages/IObservable.h
  ${ORTHANC_STONE_ROOT}/Sources/Messages/IObserver.h
  ${ORTHANC_STONE_ROOT}/Sources/Messages/ObserverBase.h

  ${ORTHANC_STONE_ROOT}/Sources/Oracle/GetOrthancImageCommand.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Oracle/GetOrthancWebViewerJpegCommand.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Oracle/HttpCommand.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Oracle/OracleCommandBase.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Oracle/OrthancRestApiCommand.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Oracle/ParseDicomFromFileCommand.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Oracle/ParseDicomFromWadoCommand.cpp

  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/CairoCompositor.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/CairoCompositor.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Color.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/ColorSceneLayer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/ColorTextureSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/ColorTextureSceneLayer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/FloatTextureSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/FloatTextureSceneLayer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/GrayscaleStyleConfigurator.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/GrayscaleStyleConfigurator.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/ICompositor.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/ILayerStyleConfigurator.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/InfoPanelSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/InfoPanelSceneLayer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/IPointerTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/ISceneLayer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/LookupTableStyleConfigurator.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/LookupTableStyleConfigurator.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/LookupTableTextureSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/LookupTableTextureSceneLayer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/NullLayer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/PanSceneTracker.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/PanSceneTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/PointerEvent.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/PointerEvent.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/PolylineSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/PolylineSceneLayer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/RotateSceneTracker.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/RotateSceneTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Scene2D.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Scene2D.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/ScenePoint2D.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/TextSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/TextSceneLayer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/TextureBaseSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/TextureBaseSceneLayer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/ZoomSceneTracker.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/ZoomSceneTracker.h

  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoBaseRenderer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoColorTextureRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoColorTextureRenderer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoFloatTextureRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoFloatTextureRenderer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoInfoPanelRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoInfoPanelRenderer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoLookupTableTextureRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoLookupTableTextureRenderer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoPolylineRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoPolylineRenderer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoTextRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CairoTextRenderer.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CompositorHelper.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/CompositorHelper.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/FixedPointAligner.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/FixedPointAligner.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/ICairoContextProvider.h
  
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/AngleMeasureTool.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/AngleMeasureTool.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateAngleMeasureCommand.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateAngleMeasureCommand.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateAngleMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateAngleMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateCircleMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateCircleMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateLineMeasureCommand.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateLineMeasureCommand.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateLineMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateLineMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/CreateSimpleTrackerAdapter.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/EditAngleMeasureCommand.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/EditAngleMeasureCommand.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/EditAngleMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/EditAngleMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/EditLineMeasureCommand.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/EditLineMeasureCommand.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/EditLineMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/EditLineMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/IFlexiblePointerTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/LayerHolder.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/LayerHolder.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/LineMeasureTool.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/LineMeasureTool.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/MeasureCommands.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/MeasureCommands.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/MeasureTool.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/MeasureTool.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/MeasureToolsToolbox.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/MeasureToolsToolbox.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/MeasureTrackers.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/MeasureTrackers.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/OneGesturePointerTracker.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/OneGesturePointerTracker.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/PredeclaredTypes.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/UndoStack.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/UndoStack.h
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/ViewportController.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Scene2DViewport/ViewportController.h
  ${ORTHANC_STONE_ROOT}/Sources/StoneEnumerations.cpp
  ${ORTHANC_STONE_ROOT}/Sources/StoneException.h
  ${ORTHANC_STONE_ROOT}/Sources/StoneInitialization.cpp

  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/AffineTransform2D.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/AffineTransform2D.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/CoordinateSystem3D.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/CoordinateSystem3D.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomInstanceParameters.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomInstanceParameters.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomStructure2.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomStructure2.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomStructurePolygon2.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomStructurePolygon2.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomStructureSet.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomStructureSet.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomStructureSet2.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomStructureSet2.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomStructureSetUtils.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DicomStructureSetUtils.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DisjointDataSet.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DynamicBitmap.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/DynamicBitmap.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/Extent2D.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/Extent2D.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/FiniteProjectiveCamera.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/FiniteProjectiveCamera.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/GenericToolbox.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/GenericToolbox.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/GeometryToolbox.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/GeometryToolbox.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/ImageGeometry.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/ImageGeometry.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/ImageToolbox.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/ImageToolbox.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/LinearAlgebra.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/LinearAlgebra.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/PixelTestPatterns.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/ShearWarpProjectiveTransform.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/ShearWarpProjectiveTransform.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/SlicesSorter.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/SlicesSorter.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/SortedFrames.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/SortedFrames.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/SubpixelReader.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/SubvoxelReader.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/TextRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/TextRenderer.h
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/UndoRedoStack.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Toolbox/UndoRedoStack.h
  
  ${ORTHANC_STONE_ROOT}/Sources/Viewport/IViewport.h
  
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/IGeometryProvider.h
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/IVolumeSlicer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/IVolumeSlicer.h
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/OrientedVolumeBoundingBox.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/OrientedVolumeBoundingBox.h

  ${ORTHANC_STONE_ROOT}/Sources/Volumes/VolumeImageGeometry.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/VolumeImageGeometry.h
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/VolumeReslicer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/VolumeReslicer.h
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/VolumeSceneLayerSource.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/VolumeSceneLayerSource.h
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/DicomStructureSetSlicer2.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/DicomStructureSetSlicer2.h
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/DicomVolumeImage.h
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/DicomVolumeImage.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/DicomVolumeImage.h
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/DicomVolumeImageMPRSlicer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/DicomVolumeImageMPRSlicer.h
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/DicomVolumeImageReslicer.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/DicomVolumeImageReslicer.h
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/ImageBuffer3D.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Volumes/ImageBuffer3D.h

  ${ORTHANC_STONE_ROOT}/Sources/Wrappers/CairoContext.cpp
  ${ORTHANC_STONE_ROOT}/Sources/Wrappers/CairoSurface.cpp

  ${PLATFORM_SOURCES}
  ${APPLICATIONS_SOURCES}
  ${ORTHANC_CORE_SOURCES}
  ${ORTHANC_DICOM_SOURCES}

  # Mandatory components
  ${CAIRO_SOURCES}
  ${FREETYPE_SOURCES}
  ${PIXMAN_SOURCES}

  # Optional components
  ${SDL_SOURCES}
  ${QT_SOURCES}
  ${GLEW_SOURCES}
  )


if (ENABLE_OPENGL)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_STONE_ROOT}/Sources/Fonts/OpenGLTextCoordinates.h
    ${ORTHANC_STONE_ROOT}/Sources/Fonts/OpenGLTextCoordinates.cpp
    ${ORTHANC_STONE_ROOT}/Sources/OpenGL/OpenGLProgram.h
    ${ORTHANC_STONE_ROOT}/Sources/OpenGL/OpenGLProgram.cpp
    ${ORTHANC_STONE_ROOT}/Sources/OpenGL/OpenGLShader.h
    ${ORTHANC_STONE_ROOT}/Sources/OpenGL/OpenGLShader.cpp
    ${ORTHANC_STONE_ROOT}/Sources/OpenGL/OpenGLTexture.h
    ${ORTHANC_STONE_ROOT}/Sources/OpenGL/OpenGLTexture.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/OpenGLCompositor.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/OpenGLCompositor.cpp

    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLAdvancedPolylineRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLAdvancedPolylineRenderer.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLBasicPolylineRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLBasicPolylineRenderer.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLColorTextureProgram.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLColorTextureProgram.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLColorTextureRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLColorTextureRenderer.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLFloatTextureProgram.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLFloatTextureProgram.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLFloatTextureRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLFloatTextureRenderer.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLInfoPanelRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLInfoPanelRenderer.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLLinesProgram.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLLinesProgram.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLLookupTableTextureRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLLookupTableTextureRenderer.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLShaderVersionDirective.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLTextProgram.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLTextProgram.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLTextRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLTextRenderer.h
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLTextureProgram.cpp
    ${ORTHANC_STONE_ROOT}/Sources/Scene2D/Internals/OpenGLTextureProgram.h
    )

  if (ENABLE_WASM)
    list(APPEND ORTHANC_STONE_SOURCES
      ${ORTHANC_STONE_ROOT}/Sources/OpenGL/WebAssemblyOpenGLContext.cpp
      ${ORTHANC_STONE_ROOT}/Sources/OpenGL/WebAssemblyOpenGLContext.h
      ${ORTHANC_STONE_ROOT}/Sources/Viewport/WebGLViewport.cpp
      ${ORTHANC_STONE_ROOT}/Sources/Viewport/WebGLViewportsRegistry.cpp
      )
  endif()
endif()

##
## TEST - Automatically add all ".h" headers to the list of sources
##

macro(AutodetectHeaderFiles SOURCES_VAR)
  set(TMP)
  
  foreach(f IN LISTS ${SOURCES_VAR})
    get_filename_component(_base ${f} NAME_WE)
    get_filename_component(_dir ${f} DIRECTORY)
    get_filename_component(_extension ${f} EXT)
    set(_header ${_dir}/${_base}.h)
    
    if ((_extension STREQUAL ".cpp" OR
          _extension STREQUAL ".cc" OR
          _extension STREQUAL ".h") AND
        EXISTS ${_header} AND
        NOT IS_DIRECTORY ${_header} AND
        NOT IS_SYMLINK ${_header})

      # Prevent adding the header twice if it is already manually
      # specified in the sources
      list (FIND SOURCES_VAR ${_header} _index)
      if (${_index} EQUAL -1)
        list(APPEND TMP ${_header})
      endif()
    endif()
  endforeach()

  list(APPEND ${SOURCES_VAR} ${TMP})
endmacro()


AutodetectHeaderFiles(ORTHANC_STONE_SOURCES)
