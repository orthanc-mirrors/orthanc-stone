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
  static void StringToVector(std::vector<uint8_t>& target,
                             const std::string& source)
  {
    target.resize(source.size());

    for (size_t i = 0; i < source.size(); i++)
    {
      target[i] = source[i];
    }
  }

  
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

    SetLookupTableGrayscale(1);
    SetRange(0, 1);
  }


  void LookupTableTextureSceneLayer::SetLookupTableGrayscale(float alpha)
  {
    std::vector<uint8_t> rgb(3 * 256);

    for (size_t i = 0; i < 256; i++)
    {
      rgb[3 * i] = i;
      rgb[3 * i + 1] = i;
      rgb[3 * i + 2] = i;
    }

    SetLookupTableRgb(rgb, alpha);
  }  


  void LookupTableTextureSceneLayer::SetLookupTableRgb(const std::vector<uint8_t>& lut,
                                                       float alpha)
  {
    if (lut.size() != 3 * 256 ||
        alpha < 0 ||
        alpha > 1)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
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
        float r = static_cast<float>(lut[3 * i]) * alpha;
        float g = static_cast<float>(lut[3 * i + 1]) * alpha;
        float b = static_cast<float>(lut[3 * i + 2]) * alpha;
        
        lut_[4 * i] = static_cast<uint8_t>(std::floor(r));
        lut_[4 * i + 1] = static_cast<uint8_t>(std::floor(g));
        lut_[4 * i + 2] = static_cast<uint8_t>(std::floor(b));
        lut_[4 * i + 3] = static_cast<uint8_t>(std::floor(alpha * 255.0f));
      }
    }

    IncrementRevision();
  }


  void LookupTableTextureSceneLayer::SetLookupTableRgb(const std::string& lut,
                                                       float alpha)
  {
    std::vector<uint8_t> tmp;
    StringToVector(tmp, lut);
    SetLookupTableRgb(tmp, alpha);
  }

  
  void LookupTableTextureSceneLayer::SetLookupTable(const std::vector<uint8_t>& lut)
  {
    if (lut.size() != 4 * 256)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    lut_ = lut;

    IncrementRevision();
  }


  void LookupTableTextureSceneLayer::SetLookupTable(const std::string& lut)
  {
    std::vector<uint8_t> tmp;
    StringToVector(tmp, lut);
    SetLookupTable(tmp);
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
    
    IncrementRevision();
  }

    
  ISceneLayer* LookupTableTextureSceneLayer::Clone() const
  {
    std::auto_ptr<LookupTableTextureSceneLayer> cloned
      (new LookupTableTextureSceneLayer(GetTexture()));

    cloned->CopyParameters(*this);
    cloned->minValue_ = minValue_;
    cloned->maxValue_ = maxValue_;
    cloned->lut_ = lut_;

    return cloned.release();
  }
}
