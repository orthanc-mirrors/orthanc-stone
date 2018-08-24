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

#include "WidgetBase.h"

namespace OrthancStone
{
  class CairoWidget : public WidgetBase
  {
  private:
    CairoSurface   surface_;

  protected:
    virtual bool RenderCairo(CairoContext& context) = 0;
    
    virtual void RenderMouseOverCairo(CairoContext& context,
                                      int x,
                                      int y) = 0;
    
  public:
    CairoWidget(const std::string& name);

    virtual void SetSize(unsigned int width,
                         unsigned int height);

    virtual bool Render(Orthanc::ImageAccessor& target);

    virtual void RenderMouseOver(Orthanc::ImageAccessor& target,
                                 int x,
                                 int y);  
  };
}
