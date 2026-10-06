/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include "../Scene2DViewport/OneGesturePointerTracker.h"
#include "../Toolbox/Windowing.h"
#include "../Viewport/IViewport.h"


namespace OrthancStone
{
  class GrayscaleWindowingSceneTracker : public OneGesturePointerTracker
  {
  public:
    class IObserver : public boost::noncopyable
    {
    public:
      virtual ~IObserver()
      {
      }

      virtual void HandleGrayscaleWindowingChange(const Windowing& windowing) = 0;
    };

    class IObserverFactory : public boost::noncopyable
    {
    public:
      virtual ~IObserverFactory()
      {
      }

      virtual IObserver* Create() = 0;
    };

  private:
    bool    active_;
    int     layerIndex_;
    double  normalization_;
    float   originalCenter_;
    float   originalWidth_;
    double  clickX_;
    double  clickY_;

    std::unique_ptr<IObserver>  observer_;

    bool SetWindowing(Scene2D& scene,
                      float center,
                      float width);
    
  public:
    GrayscaleWindowingSceneTracker(Scene2D& scene,
                                   int layerIndex,
                                   const PointerEvent& event,
                                   unsigned int canvasWidth,
                                   unsigned int canvasHeight);

    void SetObserver(IObserver* observer /* takes ownership */);

    virtual bool PointerMove(const PointerEvent& event,
                             Scene2D& scene) ORTHANC_OVERRIDE;
    
    virtual void Cancel(Scene2D& scene) ORTHANC_OVERRIDE;
  };
}
