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

#include "ISceneLayer.h"
#include "../Toolbox/AffineTransform2D.h"

#include <Compatibility.h>
#include <Images/ImageAccessor.h>

namespace OrthancStone
{
  class TextureBaseSceneLayer : public ISceneLayer
  {
  private:
    std::unique_ptr<Orthanc::ImageAccessor>  texture_;
    double                                 originX_;
    double                                 originY_;
    double                                 pixelSpacingX_;
    double                                 pixelSpacingY_;
    double                                 angle_;
    bool                                   isLinearInterpolation_;
    bool                                   flipX_;
    bool                                   flipY_;
    uint64_t                               revision_;

  protected:
    void SetTexture(Orthanc::ImageAccessor* texture);

    void IncrementRevision() 
    {
      revision_++;
    }

    void CopyParameters(const TextureBaseSceneLayer& other);

  public:
    TextureBaseSceneLayer();

    // Center of the top-left pixel
    void SetOrigin(double x,
                   double y);

    void SetPixelSpacing(double sx,
                         double sy);

    // In radians
    void SetAngle(double angle);

    void SetLinearInterpolation(bool isLinearInterpolation);

    void SetFlipX(bool flip);
    
    void SetFlipY(bool flip);
    
    double GetOriginX() const
    {
      return originX_;
    }

    double GetOriginY() const
    {
      return originY_;
    }

    double GetPixelSpacingX() const
    {
      return pixelSpacingX_;
    }

    double GetPixelSpacingY() const
    {
      return pixelSpacingY_;
    }

    double GetAngle() const
    {
      return angle_;
    }

    bool IsLinearInterpolation() const
    {
      return isLinearInterpolation_;
    }

    bool HasTexture() const
    {
      return (texture_.get() != NULL);
    }

    bool IsFlipX() const
    {
      return flipX_;
    }

    bool IsFlipY() const
    {
      return flipY_;
    }

    const Orthanc::ImageAccessor& GetTexture() const;

    AffineTransform2D GetTransform() const;
    
    virtual bool GetBoundingBox(Extent2D& target) const ORTHANC_OVERRIDE;

    virtual uint64_t GetRevision() const ORTHANC_OVERRIDE
    {
      return revision_;
    }
  };
}
