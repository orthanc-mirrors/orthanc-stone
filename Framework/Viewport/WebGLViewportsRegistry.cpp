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


#include "WebGLViewportsRegistry.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  void WebGLViewportsRegistry::LaunchTimer()
  {
    emscripten_set_timeout(OnTimeoutCallback, timeoutMS_, this);
  }

  
  void WebGLViewportsRegistry::OnTimeout()
  {
    for (Viewports::iterator it = viewports_.begin(); it != viewports_.end(); ++it)
    {
      if (it->second == NULL ||
          it->second->IsContextLost())
      {
        LOG(INFO) << "WebGL context lost for canvas: " << it->first;

        // Try and duplicate the HTML5 canvas in the DOM
        EM_ASM({
            var canvas = document.getElementById(UTF8ToString($0));
            if (canvas) {
              var parent = canvas.parentElement;
              if (parent) {
                var cloned = canvas.cloneNode(true /* deep copy */);
                parent.insertBefore(cloned, canvas);
                parent.removeChild(canvas);
              }
            }
          },
          it->first.c_str()  // $0 = ID of the canvas
          );

        // At this point, the old canvas is removed from the DOM and
        // replaced by a fresh one with the same ID: Recreate the
        // WebGL context on the new canvas
        std::auto_ptr<WebGLViewport> viewport;
          
        {
          std::auto_ptr<IViewport::ILock> lock(it->second->Lock());
          viewport.reset(new WebGLViewport(it->first, lock->GetController().GetScene()));
        }

        // Replace the old WebGL viewport by the new one
        delete it->second;
        it->second = viewport.release();

        // Tag the fresh canvas as needing a repaint
        {
          std::auto_ptr<IViewport::ILock> lock(it->second->Lock());
          lock->Invalidate();
        }
      }
    }
      
    LaunchTimer();
  }

    
  void WebGLViewportsRegistry::OnTimeoutCallback(void *userData)
  {
    WebGLViewportsRegistry& that = *reinterpret_cast<WebGLViewportsRegistry*>(userData);
    that.OnTimeout();
  }

    
  WebGLViewportsRegistry::WebGLViewportsRegistry(double timeoutMS) :
    timeoutMS_(timeoutMS)
  {
    if (timeoutMS <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }          
    
    LaunchTimer();
  }


  void WebGLViewportsRegistry::Add(const std::string& canvasId)
  {
    if (viewports_.find(canvasId) != viewports_.end())
    {
      LOG(ERROR) << "Canvas was already registered: " << canvasId;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      viewports_[canvasId] = new WebGLViewport(canvasId);
    }
  }

    
  void WebGLViewportsRegistry::Remove(const std::string& canvasId)
  {
    Viewports::iterator found = viewports_.find(canvasId);

    if (found == viewports_.end())
    {
      LOG(ERROR) << "Cannot remove unregistered canvas: " << canvasId;
    }
    else
    {
      if (found->second != NULL)
      {
        delete found->second;
      }

      viewports_.erase(found);
    }
  }

    
  void WebGLViewportsRegistry::Clear()
  {
    for (Viewports::iterator it = viewports_.begin(); it != viewports_.end(); ++it)
    {
      if (it->second != NULL)
      {
        delete it->second;
      }
    }

    viewports_.clear();
  }


  WebGLViewportsRegistry::Accessor::Accessor(WebGLViewportsRegistry& that,
                                             const std::string& canvasId) :
    that_(that)
  {
    Viewports::iterator viewport = that.viewports_.find(canvasId);
    if (viewport != that.viewports_.end() &&
        viewport->second != NULL)
    {
      lock_.reset(viewport->second->Lock());
    }
  }


  IViewport::ILock& WebGLViewportsRegistry::Accessor::GetViewport() const
  {
    if (IsValid())
    {
      return *lock_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
}
