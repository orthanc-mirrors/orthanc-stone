/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/

#include "DefaultViewportInteractor.h"

#include "../Scene2D/GrayscaleWindowingSceneTracker.h"
#include "../Scene2D/MagnifyingGlassTracker.h"
#include "../Scene2D/PanSceneTracker.h"
#include "../Scene2D/PinchZoomTracker.h"
#include "../Scene2D/RotateSceneTracker.h"
#include "../Scene2D/ZoomSceneTracker.h"
#include "../Scene2DViewport/ViewportController.h"

#include <OrthancException.h>

#include <emscripten.h>

namespace OrthancStone
{
  IFlexiblePointerTracker* DefaultViewportInteractor::CreateTrackerInternal(
    boost::weak_ptr<IViewport> viewport,
    MouseAction action,
    const PointerEvent& event,
    unsigned int viewportWidth,
    unsigned int viewportHeight)
  {
    switch (action)
    {
      case MouseAction_None:
        return NULL;

      case MouseAction_Rotate:
        return new RotateSceneTracker(viewport, event);

      case MouseAction_GrayscaleWindowing:
      {
        boost::shared_ptr<IViewport> v(viewport.lock());
        if (v == NULL)
        {
          return NULL;
        }
        else
        {
          std::unique_ptr<IViewport::ILock> lock(v->Lock());
          if (lock->GetController().GetScene().HasLayer(windowingLayer_) &&
              lock->GetController().GetScene().GetLayer(windowingLayer_).GetType() == ISceneLayer::Type_FloatTexture)
          {
            return new GrayscaleWindowingSceneTracker(
              viewport, windowingLayer_, event, viewportWidth, viewportHeight);
          }
          else
          {
            // Don't create the tracker if the layer is not a float texture
            return NULL;
          }
        }
      }

      case MouseAction_Pan:
        return new PanSceneTracker(viewport, event);
      
      case MouseAction_Zoom:
        return new ZoomSceneTracker(viewport, event, viewportHeight);
      
      case MouseAction_MagnifyingGlass:
        return new MagnifyingGlassTracker(viewport, event);

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  IFlexiblePointerTracker* DefaultViewportInteractor::CreateTracker(
    boost::weak_ptr<IViewport>  viewport,
    const PointerEvent&         event,
    unsigned int                viewportWidth,
    unsigned int                viewportHeight)
  {
    MouseAction action;
    
    switch (event.GetMouseButton())
    {
      case MouseButton_Left:
        action = leftButtonAction_;
        break;

      case MouseButton_Middle:
        action = middleButtonAction_;
        break;
      
      case MouseButton_Right:
        action = rightButtonAction_;
        break;

      case MouseButton_None:
        if (event.GetPositionsCount() == 1 ||
            event.GetPositionsCount() == 2)
        {
          return new PinchZoomTracker(viewport, event);
        }
        else
        {
          return NULL;
        }

      default:
        return NULL;
    }

    return CreateTrackerInternal(viewport, action, event, viewportWidth, viewportHeight);
  }


  void DefaultViewportInteractor::HandleMouseHover(IViewport& viewport,
                                                   const PointerEvent& event)
  {
    // "HasMouseOver()" returns "false"
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }
}
