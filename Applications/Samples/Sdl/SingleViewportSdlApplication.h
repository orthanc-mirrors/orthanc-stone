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

#include "../../../OrthancStone/Sources/Platforms/Sdl/SdlViewport.h"
#include "../Common/ISingleViewportApplicationCore.h"


namespace OrthancStone
{
  class SingleViewportSdlApplication : public StoneApplication
  {
  public:
    class ICoreStartup : public boost::noncopyable
    {
    public:
      virtual ~ICoreStartup()
      {
      }

      virtual void Start(ISingleViewportApplicationCore& core) = 0;
    };

  private:
    boost::shared_ptr<ISingleViewportApplicationCore>  core_;
    boost::shared_ptr<SdlViewport>                     viewport_;
    std::unique_ptr<ICoreStartup>                      startup_;

  protected:
    virtual void RunInternal(const boost::shared_ptr<Context>& context) ORTHANC_OVERRIDE;

  public:
    SingleViewportSdlApplication(const Configuration& configuration,
                                 const boost::shared_ptr<ISingleViewportApplicationCore>& core,
                                 const std::string& title,
                                 unsigned int width,
                                 unsigned int height,
                                 bool useOpenGL);

    void SetCoreStartup(ICoreStartup* startup /* takes ownership */);
  };
}
