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

#if ORTHANC_ENABLE_SDL == 1

#include "SdlBuffering.h"
#include "../BinarySemaphore.h"

#include <boost/thread.hpp>

namespace OrthancStone
{
  class SdlEngine : public IViewport::IChangeObserver
  {
  private:
    SdlWindow&        window_;
    IViewport&        viewport_;
    SdlBuffering      buffering_;
    boost::thread     renderThread_;
    bool              continue_;
    BinarySemaphore   renderFrame_;
    uint32_t          refreshEvent_;
    bool              viewportChanged_;

    void RenderFrame();

    static void RenderThread(SdlEngine* that);

    static KeyboardModifiers GetKeyboardModifiers(const uint8_t* keyboardState,
                                                  const int scancodeCount);

    void SetSize(unsigned int width,
                 unsigned int height);

    void Stop();

    void Refresh();

  public:
    SdlEngine(SdlWindow& window,
              IViewport& viewport);
  
    virtual ~SdlEngine();

    virtual void NotifyChange(const IViewport& viewport)
    {
      viewportChanged_ = true;
    }

    void Run();

    static void GlobalInitialize();

    static void GlobalFinalize();
  };
}

#endif
