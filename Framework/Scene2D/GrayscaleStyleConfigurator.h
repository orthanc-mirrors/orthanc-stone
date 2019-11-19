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


#pragma once

#include "ILayerStyleConfigurator.h"

namespace OrthancStone
{
  /**
  Creates layers to display the supplied image in grayscale. No dynamic 
  style is available.
  */
  class GrayscaleStyleConfigurator : public ILayerStyleConfigurator
  {
  private:
    uint64_t        revision_;
    bool            linearInterpolation_;
    bool            hasWindowing_;
    ImageWindowing  windowing_;
    float           customWindowWidth_;
    float           customWindowCenter_;
    bool            inverted_;
    bool            applyLog_;
    
  public:
    GrayscaleStyleConfigurator() :
      revision_(0),
      linearInterpolation_(false),
      hasWindowing_(false),
      customWindowWidth_(0),
      customWindowCenter_(0),
      inverted_(false),
      applyLog_(false)
    {
    }

    void SetWindowing(ImageWindowing windowing);

    void SetCustomWindowing(float windowCenter, float windowWidth);

    void GetCustomWindowing(float& windowCenter, float& windowWidth) const;

    void SetInverted(bool inverted);

    void SetLinearInterpolation(bool enabled);

    bool IsLinearInterpolation() const
    {
      return linearInterpolation_;
    }

    void SetApplyLog(bool apply);

    bool IsApplyLog() const
    {
      return applyLog_;
    }

    virtual uint64_t GetRevision() const
    {
      return revision_;
    }
    
    virtual TextureBaseSceneLayer* CreateTextureFromImage(
      const Orthanc::ImageAccessor& image) const;

    virtual TextureBaseSceneLayer* CreateTextureFromDicom(
      const Orthanc::ImageAccessor& frame,
      const DicomInstanceParameters& parameters) const;

    virtual void ApplyStyle(ISceneLayer& layer) const;
  };
}
