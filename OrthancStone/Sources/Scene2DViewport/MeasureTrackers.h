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

#include "IFlexiblePointerTracker.h"
#include "../Scene2D/Scene2D.h"
#include "../Scene2D/PointerEvent.h"

#include "MeasureTool.h"
#include "MeasureCommands.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <vector>

namespace OrthancStone
{
  class CreateMeasureTracker : public IFlexiblePointerTracker
  {
  public:
    virtual void Cancel() ORTHANC_OVERRIDE;
    virtual bool IsAlive() const ORTHANC_OVERRIDE;
  protected:
    CreateMeasureTracker(boost::shared_ptr<IViewport> viewport);

    ~CreateMeasureTracker();
  
  protected:
    boost::shared_ptr<CreateMeasureCommand>         command_;
    boost::shared_ptr<IViewport>          viewport_;
    bool                            alive_;

  private:
    bool                            commitResult_;
  };

  class EditMeasureTracker : public IFlexiblePointerTracker
  {
  public:
    virtual void Cancel() ORTHANC_OVERRIDE;
    virtual bool IsAlive() const ORTHANC_OVERRIDE;
  protected:
    EditMeasureTracker(boost::shared_ptr<IViewport> viewport, const PointerEvent& e);

    ~EditMeasureTracker();

  protected:
    boost::shared_ptr<EditMeasureCommand> command_;
    boost::shared_ptr<IViewport>   viewport_;
    bool                                  alive_;
    
    ScenePoint2D                          GetOriginalClickPosition() const
    {
      return originalClickPosition_;
    }
  private:
    ScenePoint2D                          originalClickPosition_;
    bool                                  commitResult_;
  };
}

