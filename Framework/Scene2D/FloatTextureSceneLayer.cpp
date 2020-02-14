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


#include "FloatTextureSceneLayer.h"

#include "../Toolbox/ImageToolbox.h"

#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  FloatTextureSceneLayer::FloatTextureSceneLayer(const Orthanc::ImageAccessor& texture) :
    inverted_(false)
  {
    {
      std::auto_ptr<Orthanc::ImageAccessor> t(
        new Orthanc::Image(Orthanc::PixelFormat_Float32, 
                           texture.GetWidth(), 
                           texture.GetHeight(), 
                           false));

      Orthanc::ImageProcessing::Convert(*t, texture);

      if (OrthancStone_Internals_dump_LoadTexture_histogram == 1)
      {
        LOG(ERROR) << "+----------------------------------------+";
        LOG(ERROR) << "|        This is not an error!           |";
        LOG(ERROR) << "+----------------------------------------+";
        LOG(ERROR) << "Work on the \"invisible slice\" bug";
        LOG(ERROR) << "in FloatTextureSceneLayer::FloatTextureSceneLayer";
        LOG(ERROR) << "will dump \"t\" after \"Orthanc::ImageProcessing::Convert(*t, texture);\"";

        HistogramData hd;
        double minValue = 0;
        double maxValue = 0;
        ComputeMinMax(*t, minValue, maxValue);
        double binSize = (maxValue - minValue) * 0.01;
        ComputeHistogram(*t, hd, binSize);
        std::string s;
        DumpHistogramResult(s, hd);
        LOG(ERROR) << s;
        LOG(ERROR) << "+----------------------------------------+";
        LOG(ERROR) << "|        end of debug dump               |";
        LOG(ERROR) << "+----------------------------------------+";
      }

      SetTexture(t.release());
    }

    SetCustomWindowing(128, 256);
  }


  void FloatTextureSceneLayer::SetWindowing(ImageWindowing windowing)
  {
    if (windowing_ != windowing)
    {
      if (windowing == ImageWindowing_Custom)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        windowing_ = windowing;
        IncrementRevision();
      }
    }
  }

  void FloatTextureSceneLayer::SetCustomWindowing(float customCenter,
                                                  float customWidth)
  {
    if (customWidth <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      windowing_ = ImageWindowing_Custom;
      customCenter_ = customCenter;
      customWidth_ = customWidth;
      IncrementRevision();
    }
  }

  
  void FloatTextureSceneLayer::GetWindowing(float& targetCenter,
                                            float& targetWidth) const
  {
    ::OrthancStone::ComputeWindowing(targetCenter, targetWidth,
                                     windowing_, customCenter_, customWidth_);
  }


  void FloatTextureSceneLayer::SetInverted(bool inverted)
  {
    inverted_ = inverted;
    IncrementRevision();
  }

  void FloatTextureSceneLayer::FitRange()
  {
    float minValue, maxValue;
    Orthanc::ImageProcessing::GetMinMaxFloatValue(minValue, maxValue, GetTexture());

    float width;

    assert(minValue <= maxValue);
    if (LinearAlgebra::IsCloseToZero(maxValue - minValue))
    {
      width = 1;
    }
    else
    {
      width = maxValue - minValue;
    }

    SetCustomWindowing((minValue + maxValue) / 2.0f, width);
  }

    
  ISceneLayer* FloatTextureSceneLayer::Clone() const
  {
    std::auto_ptr<FloatTextureSceneLayer> cloned
      (new FloatTextureSceneLayer(GetTexture()));

    cloned->CopyParameters(*this);
    cloned->windowing_ = windowing_;
    cloned->customCenter_ = customCenter_;
    cloned->customWidth_ = customWidth_;
    cloned->inverted_ = inverted_;

    return cloned.release();
  }
}
