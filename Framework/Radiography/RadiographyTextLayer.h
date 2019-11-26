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
    size_t                      fontSize_;
    uint8_t                     foreground_;

    static bool                                       fontHasBeenConfigured_;
    static Orthanc::EmbeddedResources::FileResourceId fontResourceId_;
  public:
    RadiographyTextLayer(MessageBroker& broker, const RadiographyScene& scene) :
      RadiographyAlphaLayer(broker, scene)
    {
    }

    void LoadText(const std::string& utf8, size_t fontSize, uint8_t foreground);

    const std::string& GetText() const
    {
      return text_;
    }

    const size_t& GetFontSize() const
    {
      return fontSize_;
    }

    const size_t& GetForeground() const
    {
      return foreground_;
    }

    static void SetFont(Orthanc::EmbeddedResources::FileResourceId fontResourceId)
    {
      fontResourceId_ = fontResourceId;
      fontHasBeenConfigured_ = true;
    }
  };
}
