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

#include "RadiographyAlphaLayer.h"

namespace OrthancStone
{
  class RadiographyScene;

  class RadiographyTextLayer : public RadiographyAlphaLayer
  {
  private:
    std::string                 text_;
    std::string                 font_;
    unsigned int                fontSize_;
    uint8_t                     foregroundGreyLevel_;

    static std::map<std::string, Orthanc::EmbeddedResources::FileResourceId>  fonts_;
  public:
    RadiographyTextLayer(const RadiographyScene& scene) :
      RadiographyAlphaLayer(scene)
    {
    }

    void SetText(const std::string& utf8, const std::string& font, unsigned int fontSize, uint8_t foregroundGreyLevel);

    const std::string& GetText() const
    {
      return text_;
    }

    const std::string& GetFont() const
    {
      return font_;
    }

    unsigned int GetFontSize() const
    {
      return fontSize_;
    }

    uint8_t GetForegroundGreyLevel() const
    {
      return foregroundGreyLevel_;
    }

    static void RegisterFont(const std::string& name, Orthanc::EmbeddedResources::FileResourceId fontResourceId)
    {
      fonts_[name] = fontResourceId;
    }
  };
}
