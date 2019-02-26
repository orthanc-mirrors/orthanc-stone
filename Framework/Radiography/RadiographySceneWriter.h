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

#include "RadiographyScene.h"
#include "RadiographyAlphaLayer.h"
#include "RadiographyDicomLayer.h"
#include "RadiographyTextLayer.h"
#include "RadiographyMaskLayer.h"
#include <json/value.h>

namespace OrthancStone
{
  class RadiographyScene;

  class RadiographySceneWriter : public boost::noncopyable
  {

  public:
    RadiographySceneWriter()
    {
    }

    void Write(Json::Value& output, const RadiographyScene& scene);

  private:
    void WriteLayer(Json::Value& output, const RadiographyLayer& layer);
    void WriteLayer(Json::Value& output, const RadiographyDicomLayer& layer);
    void WriteLayer(Json::Value& output, const RadiographyTextLayer& layer);
    void WriteLayer(Json::Value& output, const RadiographyAlphaLayer& layer);
    void WriteLayer(Json::Value& output, const RadiographyMaskLayer& layer);
  };
}
