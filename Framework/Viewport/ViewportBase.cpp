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

#include "ViewportBase.h"

#include <Core/OrthancException.h>

#include <boost/make_shared.hpp>

namespace OrthancStone
{
  ViewportBase::ViewportBase() :
    scene_(boost::make_shared<Scene2D>())
  {
  }

  
  ViewportBase::ViewportBase(boost::shared_ptr<Scene2D>& scene) :
    scene_(scene)
  {
    if (scene.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
  }
  

  ScenePoint2D ViewportBase::GetPixelCenterCoordinates(int x, int y)
  {
    if (HasCompositor())
    {
      const ICompositor& compositor = GetCompositor();
      return ScenePoint2D(
        static_cast<double>(x) + 0.5 - static_cast<double>(compositor.GetCanvasWidth()) / 2.0,
        static_cast<double>(y) + 0.5 - static_cast<double>(compositor.GetCanvasHeight()) / 2.0);
    }
    else
    {
      return ScenePoint2D(0, 0);
    }
  }
}
