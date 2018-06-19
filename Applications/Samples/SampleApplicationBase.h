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

#include "../../Applications/Sdl/BasicSdlApplication.h"
#include "../../Framework/Viewport/WidgetViewport.h"
#include "SampleApplicationContext.h"

namespace OrthancStone
{
  namespace Samples
  {

#ifdef ORTHANC_ENABLE_SDL
    class SampleSdlApplicationBase : public BasicSdlApplication {
    protected:
      std::unique_ptr<SampleApplicationContext> context_;
    public:
      BasicApplicationContext& CreateApplicationContext(Orthanc::WebServiceParameters& orthanc, WidgetViewport* centralViewport) {
        context_.reset(new SampleApplicationContext(orthanc, centralViewport));

        return *context_;
      }
    };

    typedef SampleSdlApplicationBase SampleApplicationBase_;
#else

#endif

    class SampleApplicationBase : public SampleApplicationBase_
    {
    public:
      virtual std::string GetTitle() const
      {
        return "Stone of Orthanc - Sample";
      }
    };


  }
}
