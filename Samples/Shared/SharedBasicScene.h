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

#include <boost/shared_ptr.hpp>
#include "../../Framework/Scene2DViewport/ViewportController.h"

extern const unsigned int BASIC_SCENE_FONT_SIZE;
extern const int BASIC_SCENE_LAYER_POSITION;

extern void PrepareScene(boost::shared_ptr<OrthancStone::ViewportController> controller);
extern void TakeScreenshot(const std::string& target,
                           const OrthancStone::Scene2D& scene,
                           unsigned int canvasWidth,
                           unsigned int canvasHeight);


#include "../../Applications/Generic/Scene2DInteractor.h"
#include "../../Framework/Scene2DViewport/IFlexiblePointerTracker.h"


class BasicScene2DInteractor : public OrthancStone::Scene2DInteractor
{
  boost::shared_ptr<OrthancStone::IFlexiblePointerTracker>  currentTracker_;
  bool                                                      showCursorInfo_;
public:
  BasicScene2DInteractor(boost::shared_ptr<OrthancStone::ViewportController> viewportController) :
    Scene2DInteractor(viewportController),
    showCursorInfo_(false)
  {}

  virtual bool OnMouseEvent(const OrthancStone::GuiAdapterMouseEvent& event, const OrthancStone::PointerEvent& pointerEvent) override;
  virtual bool OnKeyboardEvent(const OrthancStone::GuiAdapterKeyboardEvent& guiEvent);
  virtual bool OnWheelEvent(const OrthancStone::GuiAdapterWheelEvent& guiEvent);
};

