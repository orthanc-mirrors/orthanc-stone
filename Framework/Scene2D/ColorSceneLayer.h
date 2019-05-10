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
#include <Core/Enumerations.h>

#include <stdint.h>

namespace OrthancStone
{
  class ColorSceneLayer : public ISceneLayer
  {
  private:
    uint8_t  red_;
    uint8_t  green_;
    uint8_t  blue_;
    uint64_t revision_;

  protected:
    void BumpRevision()
    {
      // this is *not* thread-safe!!!  => (SJO) no problem, Stone assumes mono-threading
      revision_++;
    }

  public:
    ColorSceneLayer() :
      red_(255),
      green_(255),
      blue_(255),
      revision_(0)
    {
    }

    virtual uint64_t GetRevision() const ORTHANC_OVERRIDE
    {
      return revision_;
    }

    void SetColor(uint8_t red,
                  uint8_t green,
                  uint8_t blue)
    {
      red_ = red;
      green_ = green;
      blue_ = blue;
      BumpRevision();
    }

    uint8_t GetRed() const
    {
      return red_;
    }

    uint8_t GetGreen() const
    {
      return green_;
    }

    uint8_t GetBlue() const
    {
      return blue_;
    }

    float GetRedAsFloat() const
    {
      return static_cast<float>(red_) / 255.0f;
    }

    float GetGreenAsFloat() const
    {
      return static_cast<float>(green_) / 255.0f;
    }

    float GetBlueAsFloat() const
    {
      return static_cast<float>(blue_) / 255.0f;
    }
  };
}
