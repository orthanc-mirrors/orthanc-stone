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

#include "RadiographyTextLayer.h"

#include "Core/OrthancException.h"
#include "RadiographyScene.h"
#include "../Toolbox/TextRenderer.h"

namespace OrthancStone
{
  std::map<std::string, Orthanc::EmbeddedResources::FileResourceId> RadiographyTextLayer::fonts_;

  void RadiographyTextLayer::SetText(const std::string& utf8,
                                     const std::string& font,
                                     unsigned int fontSize,
                                     uint8_t foregroundGreyLevel)
  {
    if (fonts_.find(font) == fonts_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls, "The font has not been registered");
    }

    text_ = utf8;
    font_ = font;
    fontSize_ = fontSize;
    foregroundGreyLevel_ = foregroundGreyLevel;

    SetAlpha(TextRenderer::Render(fonts_[font_],
                                  fontSize_,
                                  text_));

    SetForegroundValue(foregroundGreyLevel * 256.0f);
  }

}
