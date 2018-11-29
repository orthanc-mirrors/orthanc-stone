/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include <string>

namespace OrthancStone
{
  enum SliceOffsetMode
  {
    SliceOffsetMode_Absolute,
    SliceOffsetMode_Relative,
    SliceOffsetMode_Loop
  };

  enum ImageWindowing
  {
    ImageWindowing_Default,
    ImageWindowing_Bone,
    ImageWindowing_Lung,
    ImageWindowing_Custom
  };

  enum MouseButton
  {
    MouseButton_Left,
    MouseButton_Right,
    MouseButton_Middle
  };

  enum MouseWheelDirection
  {
    MouseWheelDirection_Up,
    MouseWheelDirection_Down
  };

  enum VolumeProjection
  {
    VolumeProjection_Axial,
    VolumeProjection_Coronal,
    VolumeProjection_Sagittal
  };

  enum ImageInterpolation
  {
    ImageInterpolation_Nearest,
    ImageInterpolation_Bilinear,
    ImageInterpolation_Trilinear
  };

  enum KeyboardModifiers
  {
    KeyboardModifiers_None = 0,
    KeyboardModifiers_Shift = (1 << 0),
    KeyboardModifiers_Control = (1 << 1),
    KeyboardModifiers_Alt = (1 << 2)
  };

  enum KeyboardKeys
  {
    KeyboardKeys_Generic = 0,

    // let's use the same ids as in javascript to avoid some conversion in WASM: https://css-tricks.com/snippets/javascript/javascript-keycodes/
    KeyboardKeys_Left = 37,
    KeyboardKeys_Up = 38,
    KeyboardKeys_Right = 39,
    KeyboardKeys_Down = 40
  };

  enum SliceImageQuality
  {
    SliceImageQuality_FullPng,  // smaller to transmit but longer to generate on Orthanc side (better choice when on low bandwidth)
    SliceImageQuality_FullPam,  // bigger to transmit but faster to generate on Orthanc side (better choice when on localhost or LAN)
    SliceImageQuality_Jpeg50,
    SliceImageQuality_Jpeg90,
    SliceImageQuality_Jpeg95,

    SliceImageQuality_InternalRaw   // downloads the raw pixels data as they are stored in the DICOM file (internal use only)
  };

  enum SopClassUid
  {
    SopClassUid_RTDose
  };

  enum BitmapAnchor
  {
    BitmapAnchor_BottomLeft,
    BitmapAnchor_BottomCenter,
    BitmapAnchor_BottomRight,
    BitmapAnchor_CenterLeft,
    BitmapAnchor_Center,
    BitmapAnchor_CenterRight,
    BitmapAnchor_TopLeft,
    BitmapAnchor_TopCenter,
    BitmapAnchor_TopRight
  };

  enum MessageType
  {
    MessageType_Widget_GeometryChanged,
    MessageType_Widget_ContentChanged,

    MessageType_VolumeSlicer_GeometryReady,   // instance tags have been loaded
    MessageType_VolumeSlicer_GeometryError,
    MessageType_VolumeSlicer_ContentChanged,
    MessageType_VolumeSlicer_SliceChanged,
    MessageType_VolumeSlicer_LayerReady,      // layer is ready to be rendered
    MessageType_VolumeSlicer_LayerError,

    MessageType_DicomSeriesVolumeSlicer_FrameReady,      // pixels data of the frame have been loaded

    MessageType_SliceViewerWidget_DisplayedSlice,  // The displayed slice has changed

    MessageType_SliceLoader_GeometryReady,
    MessageType_SliceLoader_GeometryError,
    MessageType_SliceLoader_ImageReady,
    MessageType_SliceLoader_ImageError,

    MessageType_VolumeLoader_GeometryReady,
    MessageType_VolumeLoader_GeometryError,
    MessageType_VolumeLoader_ContentChanged,  // Content of several slices in the loader has changed

    MessageType_SlicedVolume_GeometryReady,
    MessageType_SlicedVolume_GeometryError,
    MessageType_SlicedVolume_VolumeReady,
    MessageType_SlicedVolume_ContentChanged,
    MessageType_SlicedVolume_SliceContentChanged,

    MessageType_HttpRequestSuccess,
    MessageType_HttpRequestError,

    MessageType_OrthancApi_InternalGetJsonResponseReady,
    MessageType_OrthancApi_InternalGetJsonResponseError,

    MessageType_OrthancApi_GenericGetJson_Ready,
    MessageType_OrthancApi_GenericGetBinary_Ready,
    MessageType_OrthancApi_GenericHttpError_Ready,
    MessageType_OrthancApi_GenericEmptyResponse_Ready,

    MessageType_Scene_GeometryChanged,
    MessageType_Scene_ContentChanged,

    MessageType_ViewportChanged,

    MessageType_Timeout,

    // used in unit tests only
    MessageType_Test1,
    MessageType_Test2,

    MessageType_CustomMessage // Custom messages ids ust be greater than this (this one must remain in last position)
  };

  
  enum Corner
  {
    Corner_TopLeft,
    Corner_TopRight,
    Corner_BottomLeft,
    Corner_BottomRight
  };

  
  bool StringToSopClassUid(SopClassUid& result,
                           const std::string& source);

  void ComputeWindowing(float& targetCenter,
                        float& targetWidth,
                        ImageWindowing windowing,
                        float defaultCenter,
                        float defaultWidth);

  void ComputeAnchorTranslation(double& deltaX /* out */,
                                double& deltaY /* out */,
                                BitmapAnchor anchor,
                                unsigned int bitmapWidth,
                                unsigned int bitmapHeight);
}
