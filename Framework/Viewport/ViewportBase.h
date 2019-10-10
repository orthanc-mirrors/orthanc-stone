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

#include "IViewport.h"

#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  class ViewportBase : public IViewport
  {
  private:
    std::string                 identifier_;
    boost::shared_ptr<Scene2D>  scene_;

  public:
    ViewportBase(const std::string& identifier);

    ViewportBase(const std::string& identifier,
                 boost::shared_ptr<Scene2D>& scene);

    virtual Scene2D& GetScene() ORTHANC_OVERRIDE
    {
      return *scene_;
    }

    virtual const std::string& GetCanvasIdentifier() const ORTHANC_OVERRIDE
    {
      return identifier_;
    }

    virtual ScenePoint2D GetPixelCenterCoordinates(int x, int y) ORTHANC_OVERRIDE;
  };
}
