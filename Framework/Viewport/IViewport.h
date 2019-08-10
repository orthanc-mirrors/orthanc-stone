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

#include "../Scene2D/ICompositor.h"
#include "../Scene2D/Scene2D.h"
#include "../Scene2D/ScenePoint2D.h"

namespace OrthancStone
{
  /**
   * Class that combines a Scene2D with a canvas where to draw the
   * scene. A call to "Refresh()" will update the content of the
   * canvas.
   **/  
  class IViewport : public boost::noncopyable
  {
  public:
    virtual ~IViewport()
    {
    }

    virtual Scene2D& GetScene() = 0;

    virtual void Refresh() = 0;

    virtual unsigned int GetCanvasWidth() const = 0;

    virtual unsigned int GetCanvasHeight() const = 0;

    virtual const std::string& GetCanvasIdentifier() const = 0;

    virtual ScenePoint2D GetPixelCenterCoordinates(int x, int y) const = 0;

#if ORTHANC_ENABLE_LOCALE == 1
    virtual void SetFont(size_t index,
      Orthanc::EmbeddedResources::FileResourceId resource,
      unsigned int fontSize,
      Orthanc::Encoding codepage) = 0;
#endif

  protected:
    virtual ICompositor* GetCompositor() = 0;

    virtual const ICompositor* GetCompositor() const
    {
      IViewport* self = const_cast<IViewport*>(this);
      return self->GetCompositor();
    }
  };
}
