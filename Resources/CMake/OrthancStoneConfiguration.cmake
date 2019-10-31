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



#####################################################################
## Configure the Orthanc Framework
#####################################################################

if (ENABLE_DCMTK)
  set(ENABLE_LOCALE ON)
else()
  if (NOT DEFINED ENABLE_LOCALE)
    set(ENABLE_LOCALE OFF)  # Disable support for locales (notably in Boost)
  endif()
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

SET(ORTHANC_STONE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

include(FindPkgConfig)
include(${CMAKE_CURRENT_LIST_DIR}/BoostExtendedConfiguration.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CairoConfiguration.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/FreetypeConfiguration.cmake)
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
  message("SDL and QT cannot not be enabled together")
elseif(ENABLE_SDL)
  message("SDL is enabled")
  include(${CMAKE_CURRENT_LIST_DIR}/SdlConfiguration.cmake)
  add_definitions(
    -DORTHANC_ENABLE_QT=0
    -DORTHANC_ENABLE_SDL=1
    )
elseif(ENABLE_QT)
  message("QT is enabled")
  include(${CMAKE_CURRENT_LIST_DIR}/QtConfiguration.cmake)
  add_definitions(
    -DORTHANC_ENABLE_QT=1
    -DORTHANC_ENABLE_SDL=0
    )
else()
  message("SDL and QT are both disabled")
  unset(USE_SYSTEM_SDL CACHE)
  add_definitions(
    -DORTHANC_ENABLE_SDL=0
    -DORTHANC_ENABLE_QT=0
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
    -DGL_GLEXT_PROTOTYPES=1
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

  # Additional resources specific to the application being built
  ${ORTHANC_STONE_APPLICATION_RESOURCES}
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
  )

if (NOT ORTHANC_SANDBOXED)
  set(PLATFORM_SOURCES
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/WebServiceCommandBase.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/WebServiceGetCommand.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/WebServicePostCommand.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/WebServiceDeleteCommand.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/DelayedCallCommand.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/Oracle.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Generic/OracleDelayedCallExecutor.h
    )

  if (ENABLE_STONE_DEPRECATED)
    list(APPEND PLATFORM_SOURCES
      ${ORTHANC_STONE_ROOT}/Platforms/Generic/OracleWebService.cpp
      ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Viewport/CairoFont.cpp
      )
  endif()

  if (ENABLE_SDL OR ENABLE_QT)
    if (ENABLE_STONE_DEPRECATED)
      list(APPEND APPLICATIONS_SOURCES
        ${ORTHANC_STONE_ROOT}/Applications/Generic/NativeStoneApplicationRunner.cpp
        ${ORTHANC_STONE_ROOT}/Applications/Generic/NativeStoneApplicationContext.cpp
        )
    endif()
      
    if (ENABLE_SDL)
      list(APPEND ORTHANC_STONE_SOURCES
        ${ORTHANC_STONE_ROOT}/Framework/Viewport/SdlWindow.cpp
        )

      list(APPEND APPLICATIONS_SOURCES
        ${ORTHANC_STONE_ROOT}/Applications/Sdl/SdlCairoSurface.cpp
        ${ORTHANC_STONE_ROOT}/Applications/Sdl/SdlEngine.cpp
        ${ORTHANC_STONE_ROOT}/Applications/Sdl/SdlOrthancSurface.cpp
        ${ORTHANC_STONE_ROOT}/Applications/Sdl/SdlStoneApplicationRunner.cpp
        )

      if (ENABLE_OPENGL)
        list(APPEND ORTHANC_STONE_SOURCES
          ${ORTHANC_STONE_ROOT}/Framework/OpenGL/SdlOpenGLContext.cpp
          ${ORTHANC_STONE_ROOT}/Framework/Viewport/SdlViewport.cpp
          )
      endif()
    endif()
  endif()
elseif (ENABLE_WASM)
  list(APPEND APPLICATIONS_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Wasm/StartupParametersBuilder.cpp
    )

  set(STONE_WASM_SOURCES
    ${ORTHANC_STONE_ROOT}/Platforms/Wasm/Defaults.cpp
    ${ORTHANC_STONE_ROOT}/Platforms/Wasm/WasmDelayedCallExecutor.cpp
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
    OUTPUT "${AUTOGENERATED_DIR}/WasmDelayedCallExecutor.c"
    COMMAND ${CMAKE_COMMAND} -E touch "${AUTOGENERATED_DIR}/WasmDelayedCallExecutor.c" ""
    DEPENDS "${ORTHANC_STONE_ROOT}/Platforms/Wasm/WasmDelayedCallExecutor.js")
  add_custom_command(
    OUTPUT "${AUTOGENERATED_DIR}/default-library.c"
    COMMAND ${CMAKE_COMMAND} -E touch "${AUTOGENERATED_DIR}/default-library.c" ""
    DEPENDS "${ORTHANC_STONE_ROOT}/Platforms/Wasm/default-library.js")
endif()

if (ENABLE_SDL OR ENABLE_WASM)
  list(APPEND APPLICATIONS_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Generic/GuiAdapter.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Generic/GuiAdapter.h
    )
endif()

if (ENABLE_STONE_DEPRECATED)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/StoneApplicationContext.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/CircleMeasureTracker.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/ColorFrameRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/DicomSeriesVolumeSlicer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/DicomStructureSetSlicer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/FrameRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/GrayscaleFrameRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/IVolumeSlicer.h
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/LineLayerRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/LineMeasureTracker.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/RenderStyle.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Layers/SliceOutlineRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/SmartLoader.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/BaseWebService.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/DicomFrameConverter.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/DownloadStack.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/IDelayedCallExecutor.h
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/IWebService.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/MessagingToolbox.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/OrthancApiClient.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/OrthancSlicesLoader.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/ParallelSlices.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/ParallelSlicesCursor.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/Slice.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Toolbox/ViewportGeometry.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Viewport/IMouseTracker.h
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Viewport/IStatusBar.h
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Viewport/IViewport.h
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Viewport/WidgetViewport.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Volumes/StructureSetLoader.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/CairoWidget.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/EmptyWidget.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/IWidget.h
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/IWorldSceneInteractor.h
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/IWorldSceneMouseTracker.h
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/LayoutWidget.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/PanMouseTracker.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/PanZoomMouseTracker.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/SliceViewerWidget.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/TestCairoWidget.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/TestWorldSceneWidget.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/WidgetBase.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/WorldSceneWidget.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/Widgets/ZoomMouseTracker.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Deprecated/dev.h
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyAlphaLayer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyDicomLayer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyLayer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyLayerCropTracker.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyLayerMaskTracker.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyLayerMoveTracker.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyLayerResizeTracker.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyLayerRotateTracker.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyMaskLayer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyScene.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographySceneCommand.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographySceneReader.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographySceneWriter.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyTextLayer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyWidget.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Radiography/RadiographyWindowingTracker.cpp
    )
endif()


if (ENABLE_DCMTK)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_STONE_ROOT}/Framework/Oracle/ParseDicomFileCommand.cpp
    )
endif()

if (ENABLE_THREADS)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_STONE_ROOT}/Framework/Messages/LockingEmitter.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Messages/LockingEmitter.h
    ${ORTHANC_STONE_ROOT}/Framework/Oracle/ThreadedOracle.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Oracle/GenericOracleRunner.cpp
    )
endif()


if (ENABLE_WASM)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_STONE_ROOT}/Framework/Oracle/WebAssemblyOracle.cpp
    )
endif()


list(APPEND ORTHANC_STONE_SOURCES
  #${ORTHANC_STONE_ROOT}/Framework/Layers/SeriesFrameRendererFactory.cpp
  #${ORTHANC_STONE_ROOT}/Framework/Layers/SingleFrameRendererFactory.cpp

  ${ORTHANC_ROOT}/Plugins/Samples/Common/DicomDatasetReader.cpp
  ${ORTHANC_ROOT}/Plugins/Samples/Common/DicomPath.cpp
  ${ORTHANC_ROOT}/Plugins/Samples/Common/FullOrthancDataset.cpp
  ${ORTHANC_ROOT}/Plugins/Samples/Common/IOrthancConnection.cpp

  ${ORTHANC_STONE_ROOT}/Framework/Fonts/FontRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Fonts/Glyph.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Fonts/GlyphAlphabet.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Fonts/GlyphBitmapAlphabet.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Fonts/GlyphTextureAlphabet.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Fonts/TextBoundingBox.cpp

  ${ORTHANC_STONE_ROOT}/Framework/Loaders/BasicFetchingItemsSorter.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/BasicFetchingItemsSorter.h
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/BasicFetchingStrategy.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/BasicFetchingStrategy.h
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/DicomStructureSetLoader.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/DicomStructureSetLoader.h
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/DicomStructureSetLoader2.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/DicomStructureSetLoader2.h
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/IFetchingItemsSorter.h
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/IFetchingStrategy.h
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/LoaderCache.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/LoaderCache.h
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/LoaderStateMachine.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/LoaderStateMachine.h
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/OrthancMultiframeVolumeLoader.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/OrthancMultiframeVolumeLoader.h
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/OrthancSeriesVolumeProgressiveLoader.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Loaders/OrthancSeriesVolumeProgressiveLoader.h
  
  ${ORTHANC_STONE_ROOT}/Framework/Messages/ICallable.h
  ${ORTHANC_STONE_ROOT}/Framework/Messages/IMessage.h
  ${ORTHANC_STONE_ROOT}/Framework/Messages/IObservable.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Messages/IObserver.h
  ${ORTHANC_STONE_ROOT}/Framework/Oracle/GetOrthancImageCommand.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Oracle/GetOrthancWebViewerJpegCommand.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Oracle/OracleCommandWithPayload.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Oracle/OrthancRestApiCommand.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Oracle/HttpCommand.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/CairoCompositor.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/ColorTextureSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/FloatTextureSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/GrayscaleStyleConfigurator.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/ICompositor.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/InfoPanelSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/CairoColorTextureRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/CairoFloatTextureRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/CairoInfoPanelRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/CairoLookupTableTextureRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/CairoPolylineRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/CairoTextRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/CompositorHelper.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/FixedPointAligner.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/LookupTableStyleConfigurator.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/LookupTableTextureSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/PanSceneTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/PointerEvent.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/PolylineSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/RotateSceneTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Scene2D.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/TextSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/TextureBaseSceneLayer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2D/ZoomSceneTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/AngleMeasureTool.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/AngleMeasureTool.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateAngleMeasureCommand.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateAngleMeasureCommand.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateAngleMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateAngleMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateCircleMeasureCommand.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateCircleMeasureCommand.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateCircleMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateCircleMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateLineMeasureCommand.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateLineMeasureCommand.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateLineMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateLineMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/CreateSimpleTrackerAdapter.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/EditAngleMeasureCommand.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/EditAngleMeasureCommand.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/EditAngleMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/EditAngleMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/EditLineMeasureCommand.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/EditLineMeasureCommand.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/EditLineMeasureTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/EditLineMeasureTracker.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/IFlexiblePointerTracker.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/LayerHolder.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/LayerHolder.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/LineMeasureTool.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/LineMeasureTool.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/MeasureCommands.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/MeasureCommands.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/MeasureTool.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/MeasureTool.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/MeasureToolsToolbox.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/MeasureToolsToolbox.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/MeasureTrackers.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/MeasureTrackers.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/OneGesturePointerTracker.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/OneGesturePointerTracker.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/PredeclaredTypes.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/UndoStack.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/UndoStack.h
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/ViewportController.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Scene2DViewport/ViewportController.h
  ${ORTHANC_STONE_ROOT}/Framework/StoneEnumerations.cpp
  ${ORTHANC_STONE_ROOT}/Framework/StoneException.h
  ${ORTHANC_STONE_ROOT}/Framework/StoneInitialization.cpp
  
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/AffineTransform2D.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/AffineTransform2D.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/CoordinateSystem3D.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/CoordinateSystem3D.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomInstanceParameters.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomInstanceParameters.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructure2.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructure2.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructurePolygon2.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructurePolygon2.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructureSet.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructureSet.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructureSet2.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructureSet2.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructureSetUtils.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DicomStructureSetUtils.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DisjointDataSet.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DynamicBitmap.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/DynamicBitmap.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/Extent2D.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/Extent2D.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/FiniteProjectiveCamera.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/FiniteProjectiveCamera.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/GeometryToolbox.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/GeometryToolbox.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/ImageGeometry.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/ImageGeometry.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/LinearAlgebra.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/LinearAlgebra.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/ShearWarpProjectiveTransform.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/ShearWarpProjectiveTransform.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/SlicesSorter.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/SlicesSorter.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/SubpixelReader.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/SubvoxelReader.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/TextRenderer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/TextRenderer.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/UndoRedoStack.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/UndoRedoStack.h
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/GenericToolbox.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Toolbox/GenericToolbox.h
  
  ${ORTHANC_STONE_ROOT}/Framework/Viewport/IViewport.h
  ${ORTHANC_STONE_ROOT}/Framework/Viewport/ViewportBase.h
  ${ORTHANC_STONE_ROOT}/Framework/Viewport/ViewportBase.cpp
  
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/IVolumeSlicer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/IVolumeSlicer.h
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/OrientedVolumeBoundingBox.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/OrientedVolumeBoundingBox.h
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/VolumeImageGeometry.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/VolumeImageGeometry.h
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/VolumeReslicer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/VolumeReslicer.h
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/VolumeSceneLayerSource.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/VolumeSceneLayerSource.h
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/DicomStructureSetSlicer2.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/DicomStructureSetSlicer2.h
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/DicomVolumeImage.h
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/DicomVolumeImage.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/DicomVolumeImage.h
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/DicomVolumeImageMPRSlicer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/DicomVolumeImageMPRSlicer.h
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/DicomVolumeImageReslicer.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/DicomVolumeImageReslicer.h
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/ImageBuffer3D.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Volumes/ImageBuffer3D.h

  ${ORTHANC_STONE_ROOT}/Framework/Wrappers/CairoContext.cpp
  ${ORTHANC_STONE_ROOT}/Framework/Wrappers/CairoSurface.cpp

  ${PLATFORM_SOURCES}
  ${APPLICATIONS_SOURCES}
  ${ORTHANC_CORE_SOURCES}
  ${ORTHANC_DICOM_SOURCES}
  ${AUTOGENERATED_SOURCES}

  # Mandatory components
  ${CAIRO_SOURCES}
  ${FREETYPE_SOURCES}
  ${PIXMAN_SOURCES}

  # Optional components
  ${SDL_SOURCES}
  ${QT_SOURCES}
  ${BOOST_EXTENDED_SOURCES}
  ${GLEW_SOURCES}
  )


if (ENABLE_OPENGL)
  list(APPEND ORTHANC_STONE_SOURCES
    ${ORTHANC_STONE_ROOT}/Framework/Fonts/OpenGLTextCoordinates.h
    ${ORTHANC_STONE_ROOT}/Framework/Fonts/OpenGLTextCoordinates.cpp
    ${ORTHANC_STONE_ROOT}/Framework/OpenGL/OpenGLProgram.h
    ${ORTHANC_STONE_ROOT}/Framework/OpenGL/OpenGLProgram.cpp
    ${ORTHANC_STONE_ROOT}/Framework/OpenGL/OpenGLShader.h
    ${ORTHANC_STONE_ROOT}/Framework/OpenGL/OpenGLShader.cpp
    ${ORTHANC_STONE_ROOT}/Framework/OpenGL/OpenGLTexture.h
    ${ORTHANC_STONE_ROOT}/Framework/OpenGL/OpenGLTexture.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/OpenGLCompositor.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/OpenGLCompositor.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLAdvancedPolylineRenderer.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLAdvancedPolylineRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLBasicPolylineRenderer.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLBasicPolylineRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLColorTextureProgram.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLColorTextureProgram.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLColorTextureRenderer.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLColorTextureRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLFloatTextureProgram.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLFloatTextureProgram.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLFloatTextureRenderer.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLFloatTextureRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLInfoPanelRenderer.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLInfoPanelRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLLinesProgram.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLLinesProgram.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLLookupTableTextureRenderer.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLLookupTableTextureRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLTextProgram.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLTextProgram.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLTextRenderer.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLTextRenderer.cpp
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLTextureProgram.h
    ${ORTHANC_STONE_ROOT}/Framework/Scene2D/Internals/OpenGLTextureProgram.cpp
    )

  if (ENABLE_WASM)
    list(APPEND ORTHANC_STONE_SOURCES
      ${ORTHANC_STONE_ROOT}/Framework/OpenGL/WebAssemblyOpenGLContext.h
      ${ORTHANC_STONE_ROOT}/Framework/OpenGL/WebAssemblyOpenGLContext.cpp
      ${ORTHANC_STONE_ROOT}/Framework/Viewport/WebAssemblyViewport.h
      ${ORTHANC_STONE_ROOT}/Framework/Viewport/WebAssemblyViewport.cpp
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
