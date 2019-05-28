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

#if ORTHANC_ENABLE_SDL == 1

#include "SdlWindow.h"
#include "../../Framework/Wrappers/CairoSurface.h"
#include "../../Framework/Deprecated/Viewport/IViewport.h"

#include <boost/thread/mutex.hpp>

namespace OrthancStone
{
  class SdlCairoSurface : public boost::noncopyable
  {
  private:
    std::auto_ptr<CairoSurface>  cairoSurface_;
    SdlWindow&                   window_;
    SDL_Surface*                 sdlSurface_;

  public:
    SdlCairoSurface(SdlWindow& window);

    ~SdlCairoSurface();

    void SetSize(unsigned int width,
                 unsigned int height);

    void Render(Deprecated::IViewport& viewport);
  };
}

#endif
