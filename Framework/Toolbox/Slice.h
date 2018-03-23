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

#include "CoordinateSystem3D.h"
#include "DicomFrameConverter.h"

#include "../../Resources/Orthanc/Core/DicomFormat/DicomImageInformation.h"

namespace OrthancStone
{
  class Slice : public boost::noncopyable
  {
  private:
    enum Type
    {
      Type_Invalid,
      Type_Standalone,
      Type_OrthancDecodableFrame,
      Type_OrthancRawFrame
      // TODO A slice could come from some DICOM file (URL)
    };

    bool ComputeRTDoseGeometry(const Orthanc::DicomMap& dataset,
                               unsigned int frame);

    Type                 type_;
    std::string          orthancInstanceId_;
    std::string          sopClassUid_;
    unsigned int         frame_;
    unsigned int         frameCount_;   // TODO : Redundant with "imageInformation_"
    CoordinateSystem3D   geometry_;
    double               pixelSpacingX_;
    double               pixelSpacingY_;
    double               thickness_;
    unsigned int         width_;   // TODO : Redundant with "imageInformation_"
    unsigned int         height_;   // TODO : Redundant with "imageInformation_"
    DicomFrameConverter  converter_;   // TODO : Partially redundant with "imageInformation_"

    std::auto_ptr<Orthanc::DicomImageInformation>  imageInformation_;

  public:
    Slice() :
      type_(Type_Invalid)
    {        
    }

    // TODO Is this constructor the best way to go to tackle missing
    // layers within LayerWidget?
    Slice(const CoordinateSystem3D& plane,
          double thickness) :
      type_(Type_Standalone),
      frame_(0),
      frameCount_(0),
      geometry_(plane),
      pixelSpacingX_(1),
      pixelSpacingY_(1),
      thickness_(thickness),
      width_(0),
      height_(0)
    {      
    }

    Slice(const CoordinateSystem3D& plane,
          double pixelSpacingX,
          double pixelSpacingY,
          double thickness,
          unsigned int width,
          unsigned int height,
          const DicomFrameConverter& converter) :
      type_(Type_Standalone),
      frameCount_(1),
      geometry_(plane),
      pixelSpacingX_(pixelSpacingX),
      pixelSpacingY_(pixelSpacingY),
      thickness_(thickness),
      width_(width),
      height_(height),
      converter_(converter)
    {
    }

    bool IsValid() const
    {
      return type_ != Type_Invalid;
    } 

    bool ParseOrthancFrame(const Orthanc::DicomMap& dataset,
                           const std::string& instanceId,
                           unsigned int frame);

    bool HasOrthancDecoding() const
    {
      return type_ == Type_OrthancDecodableFrame;
    }

    const std::string GetOrthancInstanceId() const;

    unsigned int GetFrame() const;

    const CoordinateSystem3D& GetGeometry() const;

    double GetThickness() const;

    double GetPixelSpacingX() const;

    double GetPixelSpacingY() const;

    unsigned int GetWidth() const;

    unsigned int GetHeight() const;

    const DicomFrameConverter& GetConverter() const;

    bool ContainsPlane(const CoordinateSystem3D& plane) const;

    void GetExtent(std::vector<Vector>& points) const;

    const Orthanc::DicomImageInformation& GetImageInformation() const;
  };
}
