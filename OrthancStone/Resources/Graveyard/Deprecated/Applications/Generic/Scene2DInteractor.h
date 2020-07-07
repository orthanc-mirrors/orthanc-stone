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

#include "../../Framework/Scene2D/PointerEvent.h"
#include "../../Framework/Scene2DViewport/ViewportController.h"
//#include "../../Framework/Scene2D/Internals/CompositorHelper.h"
#include "GuiAdapter.h"


namespace OrthancStone
{

  class Scene2DInteractor
  {
  protected:
    boost::shared_ptr<ViewportController>       viewportController_;
//    boost::shared_ptr<ICompositor>              compositor_;

  public:
    Scene2DInteractor(boost::shared_ptr<ViewportController> viewportController) :
      viewportController_(viewportController)
    {}

//    void SetCompositor(boost::shared_ptr<ICompositor> compositor)
//    {
//      compositor_ = compositor;
//    }

    virtual bool OnMouseEvent(const GuiAdapterMouseEvent& guiEvent, const PointerEvent& pointerEvent) = 0; // returns true if it has handled the event
    virtual bool OnKeyboardEvent(const GuiAdapterKeyboardEvent& guiEvent) = 0; // returns true if it has handled the event
    virtual bool OnWheelEvent(const GuiAdapterWheelEvent& guiEvent) = 0; // returns true if it has handled the event

  };
}
