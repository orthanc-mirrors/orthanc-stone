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

#include "../Toolbox/UndoRedoStack.h"
#include "../Toolbox/ViewportGeometry.h"
#include "../Widgets/IWorldSceneMouseTracker.h"
#include "RadiographyScene.h"

namespace OrthancStone
{
  class RadiographyLayerMaskTracker : public IWorldSceneMouseTracker
  {
  private:
    class UndoRedoCommand;

    UndoRedoStack&                   undoRedoStack_;
    RadiographyScene::LayerAccessor  accessor_;
    ControlPoint                     startSceneCp_;
    ControlPoint                     endSceneCp_;

  public:
    RadiographyLayerMaskTracker(UndoRedoStack& undoRedoStack,
                                RadiographyScene& scene,
                                const ViewportGeometry& view,
                                size_t layer,
                                const ControlPoint& startSceneControlPoint);

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom);

    virtual void MouseUp();

    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY,
                           const std::vector<Touch>& displayTouches,
                           const std::vector<Touch>& sceneTouches);
  };
}