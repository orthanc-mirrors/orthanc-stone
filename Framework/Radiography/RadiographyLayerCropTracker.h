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

#include "../Toolbox/UndoRedoStack.h"
#include "../Toolbox/ViewportGeometry.h"
#include "../Widgets/IWorldSceneMouseTracker.h"
#include "RadiographyScene.h"

namespace OrthancStone
{
  class RadiographyLayerCropTracker : public IWorldSceneMouseTracker
  {
  private:
    class UndoRedoCommand;

    UndoRedoStack&                   undoRedoStack_;
    RadiographyScene::LayerAccessor  accessor_;
    Corner                           corner_;
    unsigned int                     cropX_;
    unsigned int                     cropY_;
    unsigned int                     cropWidth_;
    unsigned int                     cropHeight_;

  public:
    RadiographyLayerCropTracker(UndoRedoStack& undoRedoStack,
                                RadiographyScene& scene,
                                const ViewportGeometry& view,
                                size_t layer,
                                double x,
                                double y,
                                Corner corner);

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
                           double sceneY);
  };
}