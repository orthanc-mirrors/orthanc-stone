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

#if ORTHANC_ENABLE_SDL == 1

#include "../../Framework/Messages/ObserverBase.h"
#include "../Generic/NativeStoneApplicationContext.h"
#include "SdlCairoSurface.h"

namespace OrthancStone
{
  class SdlEngine : public ObserverBase<SdlEngine>
  {
  private:
    SdlWindow&                window_;
    NativeStoneApplicationContext&  context_;
    SdlCairoSurface           surface_;
    bool                      viewportChanged_;

    void SetSize(unsigned int width,
                 unsigned int height);
    
    void RenderFrame();

    static KeyboardModifiers GetKeyboardModifiers(const uint8_t* keyboardState,
                                                  const int scancodeCount);

  public:
    SdlEngine(SdlWindow& window,
              NativeStoneApplicationContext& context);
  
    void OnViewportChanged(const Deprecated::IViewport::ViewportChangedMessage& message)
    {
      viewportChanged_ = true;
    }

    void Run();
  };
}

#endif