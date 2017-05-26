/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "SliceGeometry.h"
#include "DicomFrameConverter.h"

namespace OrthancStone
{
  class Slice
  {
  private:
    enum Type
    {
      Type_Invalid,
      Type_Detached,
      Type_OrthancInstance
      // TODO A slice could come from some DICOM file (URL)
    };

    Type                 type_;
    std::string          orthancInstanceId_;
    unsigned int         frame_;
    SliceGeometry        geometry_;
    double               pixelSpacingX_;
    double               pixelSpacingY_;
    double               thickness_;
    unsigned int         width_;
    unsigned int         height_;
    DicomFrameConverter  converter_;
      
  public:
    Slice() : type_(Type_Invalid)
    {        
    }

    // TODO Is this constructor the best way to go to tackle missing
    // layers within LayerWidget?
    Slice(const SliceGeometry& plane,
          double thickness) :
      type_(Type_Detached),
      frame_(0),
      geometry_(plane),
      pixelSpacingX_(1),
      pixelSpacingY_(1),
      thickness_(thickness),
      width_(0),
      height_(0)
    {      
    }

    bool ParseOrthancFrame(const OrthancPlugins::IDicomDataset& dataset,
                           const std::string& instanceId,
                           unsigned int frame);

    bool IsOrthancInstance() const
    {
      return type_ == Type_OrthancInstance;
    }

    const std::string GetOrthancInstanceId() const;

    unsigned int GetFrame() const;

    const SliceGeometry& GetGeometry() const;

    double GetThickness() const;

    double GetPixelSpacingX() const;

    double GetPixelSpacingY() const;

    unsigned int GetWidth() const;

    unsigned int GetHeight() const;

    const DicomFrameConverter& GetConverter() const;

    bool ContainsPlane(const SliceGeometry& plane) const;
  };
}
