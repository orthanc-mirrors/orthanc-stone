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


#include "LookupTableStyleConfigurator.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  LookupTableStyleConfigurator::LookupTableStyleConfigurator() :
    revision_(0),
    hasLut_(false),
    hasRange_(false)
  {
  }


  void LookupTableStyleConfigurator::SetLookupTable(Orthanc::EmbeddedResources::FileResourceId resource)
  {
    hasLut_ = true;
    Orthanc::EmbeddedResources::GetFileResource(lut_, resource);
  }


  void LookupTableStyleConfigurator::SetLookupTable(const std::string& lut)
  {
    hasLut_ = true;
    lut_ = lut;
  }


  void LookupTableStyleConfigurator::SetRange(float minValue,
                                              float maxValue)
  {
    if (minValue > maxValue)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      hasRange_ = true;
      minValue_ = minValue;
      maxValue_ = maxValue;
    }
  }

    
  TextureBaseSceneLayer* LookupTableStyleConfigurator::CreateTextureFromImage(const Orthanc::ImageAccessor& image) const
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }


  void LookupTableStyleConfigurator::ApplyStyle(ISceneLayer& layer) const
  {
    LookupTableTextureSceneLayer& l = dynamic_cast<LookupTableTextureSceneLayer&>(layer);
      
    if (hasLut_)
    {
      l.SetLookupTable(lut_);
    }

    if (hasRange_)
    {
      l.SetRange(minValue_, maxValue_);
    }
    else
    {
      l.FitRange();
    }
  }
}
