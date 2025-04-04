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

#include "CreateLineMeasureCommand.h"

#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

namespace OrthancStone
{
  CreateLineMeasureCommand::CreateLineMeasureCommand(
    boost::weak_ptr<IViewport> viewport,
    ScenePoint2D           point)
    : CreateMeasureCommand(viewport)
    , measureTool_(LineMeasureTool::Create(viewport))
  {
    
    std::unique_ptr<IViewport::ILock> lock(GetViewportLock());
    ViewportController& controller = lock->GetController();
    controller.AddMeasureTool(measureTool_);
    measureTool_->Set(point, point);
    lock->Invalidate();
  }

  void CreateLineMeasureCommand::SetEnd(ScenePoint2D scenePos)
  {
    measureTool_->SetEnd(scenePos);
  }
}
