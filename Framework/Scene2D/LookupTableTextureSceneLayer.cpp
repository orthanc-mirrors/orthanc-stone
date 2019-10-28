/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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


#include "LookupTableTextureSceneLayer.h"

#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  LookupTableTextureSceneLayer::LookupTableTextureSceneLayer(const Orthanc::ImageAccessor& texture)
  {
    {
      std::auto_ptr<Orthanc::ImageAccessor> t(
        new Orthanc::Image(Orthanc::PixelFormat_Float32, 
                           texture.GetWidth(), 
                           texture.GetHeight(), 
                           false));

      Orthanc::ImageProcessing::Convert(*t, texture);
      SetTexture(t.release());
    }

    SetLookupTableGrayscale(); // simple ramp between 0 and 255
    SetRange(0, 1);
  }


  void LookupTableTextureSceneLayer::SetLookupTableGrayscale()
  {
    std::vector<uint8_t> rgb(3 * 256);

    for (size_t i = 0; i < 256; i++)
    {
      rgb[3 * i]     = static_cast<uint8_t>(i);
      rgb[3 * i + 1] = static_cast<uint8_t>(i);
      rgb[3 * i + 2] = static_cast<uint8_t>(i);
    }

    SetLookupTableRgb(rgb);
  }  


  void LookupTableTextureSceneLayer::SetLookupTableRgb(const std::vector<uint8_t>& lut)
  {
    if (lut.size() != 3 * 256)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    lut_.resize(4 * 256);

    for (size_t i = 0; i < 256; i++)
    {
      // Premultiplied alpha
      
      if (i == 0)
      {
        // Make zero transparent
        lut_[4 * i] = 0;        // R
        lut_[4 * i + 1] = 0;    // G
        lut_[4 * i + 2] = 0;    // B
        lut_[4 * i + 3] = 0;    // A
      }
      else
      {
        float a = static_cast<float>(i) / 255.0f;
        
        float r = static_cast<float>(lut[3 * i]) * a;
        float g = static_cast<float>(lut[3 * i + 1]) * a;
        float b = static_cast<float>(lut[3 * i + 2]) * a;
        
        lut_[4 * i] = static_cast<uint8_t>(std::floor(r));
        lut_[4 * i + 1] = static_cast<uint8_t>(std::floor(g));
        lut_[4 * i + 2] = static_cast<uint8_t>(std::floor(b));
        lut_[4 * i + 3] = static_cast<uint8_t>(std::floor(a * 255.0f));
      }
    }

    IncrementRevision();
  }


  void LookupTableTextureSceneLayer::SetLookupTable(const std::vector<uint8_t>& lut)
  {
    if (lut.size() == 4 * 256)
    {
      lut_ = lut;
      IncrementRevision();
    }
    else if (lut.size() == 3 * 256)
    {
      SetLookupTableRgb(lut);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }

  void LookupTableTextureSceneLayer::SetRange(float minValue,
                                              float maxValue)
  {
    if (minValue > maxValue)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      minValue_ = minValue;
      maxValue_ = maxValue;
      IncrementRevision();
    }
  }

  void LookupTableTextureSceneLayer::FitRange()
  {
    Orthanc::ImageProcessing::GetMinMaxFloatValue(minValue_, maxValue_, GetTexture());
    assert(minValue_ <= maxValue_);
    // TODO: debug to be removed
    if (fabs(maxValue_ - minValue_) < 0.0001) {
      LOG(INFO) << "LookupTableTextureSceneLayer::FitRange(): minValue_ = " << minValue_ << " maxValue_ = " << maxValue_;
    }
    IncrementRevision();
  }

    
  ISceneLayer* LookupTableTextureSceneLayer::Clone() const
  {
    std::auto_ptr<LookupTableTextureSceneLayer> cloned
      (new LookupTableTextureSceneLayer(GetTexture()));


    // TODO: why is windowing_ not copied??????
    cloned->CopyParameters(*this);
    cloned->minValue_ = minValue_;
    cloned->maxValue_ = maxValue_;
    cloned->lut_ = lut_;

    return cloned.release();
  }
}
