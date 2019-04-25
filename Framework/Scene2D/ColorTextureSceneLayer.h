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

#include "ISceneLayer.h"
#include "../Toolbox/AffineTransform2D.h"

#include <Core/Images/ImageAccessor.h>
#include <memory>

namespace OrthancStone
{
  class ColorTextureSceneLayer : public ISceneLayer
  {
  private:
    std::auto_ptr<Orthanc::ImageAccessor>  texture_;
    double                                 originX_;
    double                                 originY_;
    double                                 pixelSpacingX_;
    double                                 pixelSpacingY_;
    double                                 angle_;
    bool                                   isLinearInterpolation_;

  public:
    ColorTextureSceneLayer(const Orthanc::ImageAccessor& texture,
                           double originX,  // Center of the top-left pixel
                           double originY,
                           double pixelSpacingX,
                           double pixelSpacingY,
                           double angle,
                           bool isLinearInterpolation);

    virtual ISceneLayer* Clone() const;

    const Orthanc::ImageAccessor& GetTexture() const
    {
      return *texture_;
    }

    AffineTransform2D GetTransform() const;

    bool IsLinearInterpolation() const
    {
      return isLinearInterpolation_;
    }

    virtual Type GetType() const
    {
      return Type_ColorTexture;
    }

    virtual bool GetBoundingBox(Extent2D& target) const;
    
    virtual uint64_t GetRevision() const
    {
      return 0;
    }
  };
}
