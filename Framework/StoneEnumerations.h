/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


namespace Deprecated
{
  enum SliceImageQuality
  {
    SliceImageQuality_FullPng,  // smaller to transmit but longer to generate on Orthanc side (better choice when on low bandwidth)
    SliceImageQuality_FullPam,  // bigger to transmit but faster to generate on Orthanc side (better choice when on localhost or LAN)
    SliceImageQuality_Jpeg50,
    SliceImageQuality_Jpeg90,
    SliceImageQuality_Jpeg95,

    SliceImageQuality_InternalRaw   // downloads the raw pixels data as they are stored in the DICOM file (internal use only)
  };  
}


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
    KeyboardKeys_Backspace = 8,
    KeyboardKeys_Left = 37,
    KeyboardKeys_Up = 38,
    KeyboardKeys_Right = 39,
    KeyboardKeys_Down = 40,
    KeyboardKeys_Delete = 46,

    KeyboardKeys_F1 = 112,
    KeyboardKeys_F2 = 113,
    KeyboardKeys_F3 = 114,
    KeyboardKeys_F4 = 115,
    KeyboardKeys_F5 = 116,
    KeyboardKeys_F6 = 117,
    KeyboardKeys_F7 = 118,
    KeyboardKeys_F8 = 119,
    KeyboardKeys_F9 = 120,
    KeyboardKeys_F10 = 121,
    KeyboardKeys_F11 = 122,
    KeyboardKeys_F12 = 123,
  };

  enum SopClassUid
  {
    SopClassUid_Other,
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

  SopClassUid StringToSopClassUid(const std::string& source);

  void ComputeWindowing(float& targetCenter,
                        float& targetWidth,
                        ImageWindowing windowing,
                        float customCenter,
                        float customWidth);

  void ComputeAnchorTranslation(double& deltaX /* out */,
                                double& deltaY /* out */,
                                BitmapAnchor anchor,
                                unsigned int bitmapWidth,
                                unsigned int bitmapHeight,
                                unsigned int border = 0);
}
