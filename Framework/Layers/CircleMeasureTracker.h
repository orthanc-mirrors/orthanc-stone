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

#include "../Widgets/IWorldSceneMouseTracker.h"

#include "../Viewport/IStatusBar.h"
#include "../Toolbox/CoordinateSystem3D.h"

#include <Core/Images/Font.h>

namespace Deprecated
{
  class CircleMeasureTracker : public IWorldSceneMouseTracker
  {
  private:
    IStatusBar*           statusBar_;
    OrthancStone::CoordinateSystem3D    slice_;
    double                x1_;
    double                y1_;
    double                x2_;
    double                y2_;
    uint8_t               color_[3];
    const Orthanc::Font&  font_;

  public:
    CircleMeasureTracker(IStatusBar* statusBar,
                         const OrthancStone::CoordinateSystem3D& slice,
                         double x, 
                         double y,
                         uint8_t red,
                         uint8_t green,
                         uint8_t blue,
                         const Orthanc::Font& font);
    
    virtual bool HasRender() const
    {
      return true;
    }

    virtual void Render(OrthancStone::CairoContext& context,
                        double zoom);
    
    double GetRadius() const;  // In millimeters

    std::string FormatRadius() const;

    virtual void MouseUp()
    {
      // Possibly create a new landmark "volume" with the circle in subclasses
    }

    virtual void MouseMove(int displayX,
                           int displayY,
                           double x,
                           double y,
                           const std::vector<Touch>& displayTouches,
                           const std::vector<Touch>& sceneTouches);
  };
}
