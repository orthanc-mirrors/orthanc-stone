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

#include "RadiographyTextLayer.h"

#include "Core/OrthancException.h"
#include "RadiographyScene.h"
#include "Framework/Toolbox/TextRenderer.h"

namespace OrthancStone
{
  bool RadiographyTextLayer::fontHasBeenConfigured_ = false;
  Orthanc::EmbeddedResources::FileResourceId RadiographyTextLayer::fontResourceId_;

  void RadiographyTextLayer::SetText(const std::string& utf8,
                                      unsigned int fontSize,
                                      uint8_t foreground)
  {
    if (!fontHasBeenConfigured_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls, "No font has been loaded");
    }

    text_ = utf8;
    fontSize_ = fontSize;
    foreground_ = foreground;

    SetAlpha(TextRenderer::Render(fontResourceId_,
                                  fontSize_,
                                  text_));

    SetForegroundValue(foreground * 256.0f);
  }

}
