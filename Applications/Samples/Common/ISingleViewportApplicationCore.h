/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

#include "../../../OrthancStone/Sources/StoneApplication.h"
#include "../../../OrthancStone/Sources/Viewport/IViewport.h"
#include "../../../OrthancStone/Sources/Viewport/IViewportInteractor.h"


namespace OrthancStone
{
  class ISingleViewportApplicationCore : public boost::noncopyable
  {
  public:
    virtual ~ISingleViewportApplicationCore()
    {
    }

    virtual void CreateComponents(const boost::shared_ptr<StoneApplication::Context>& context,
                                  const boost::shared_ptr<IViewport>& viewport) = 0;

    virtual IViewportInteractor* CreateMouseInteractor() = 0;

    virtual bool HandleKeyDown(const IEnvironment::ILock& environmentLock,
                               char key) = 0;

    virtual void Render(const IEnvironment::ILock& environmentLock,
                        IViewport::ILock& viewportLock) = 0;
  };
}
