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

#include <SDL_render.h>
#include <SDL_video.h>
#include <boost/noncopyable.hpp>

namespace OrthancStone
{
  class SdlWindow : public boost::noncopyable
  {
  private:
    SDL_Window    *window_;
    SDL_Renderer  *renderer_;
    bool           maximized_;

  public:
    SdlWindow(const char* title,
              unsigned int width,
              unsigned int height,
              bool enableOpenGl);

    ~SdlWindow();

    SDL_Window *GetObject() const
    {
      return window_;
    }

    unsigned int GetWidth() const;

    unsigned int GetHeight() const;

    void Render(SDL_Surface* surface);

    void ToggleMaximize();

    static void GlobalInitialize();

    static void GlobalFinalize();
  };
}

#endif
