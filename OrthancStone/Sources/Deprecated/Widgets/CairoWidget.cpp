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


#include "CairoWidget.h"

#include <Images/ImageProcessing.h>
#include <OrthancException.h>

namespace Deprecated
{
  static bool IsAligned(const Orthanc::ImageAccessor& target)
  {
    // TODO
    return true;
  }

  CairoWidget::CairoWidget(const std::string& name) :
    WidgetBase(name)
  {
  }

  void CairoWidget::SetSize(unsigned int width,
                            unsigned int height)
  {
    surface_.SetSize(width, height, false /* no alpha */);
  }
  

  bool CairoWidget::Render(Orthanc::ImageAccessor& target)
  {
    // Don't call the base class here, as
    // "ClearBackgroundCairo()" is a faster alternative

    if (IsAligned(target))
    {
      OrthancStone::CairoSurface surface(target, false /* no alpha */);
      OrthancStone::CairoContext context(surface);
      ClearBackgroundCairo(context);
      return RenderCairo(context);
    }
    else
    {
      OrthancStone::CairoContext context(surface_);
      ClearBackgroundCairo(context);

      if (RenderCairo(context))
      {
        Orthanc::ImageAccessor surface;
        surface_.GetReadOnlyAccessor(surface);
        Orthanc::ImageProcessing::Copy(target, surface);
        return true;
      }
      else
      {
        return false;
      }
    }
  }


  void CairoWidget::RenderMouseOver(Orthanc::ImageAccessor& target,
                                    int x,
                                    int y)
  {
    if (IsAligned(target))
    {
      OrthancStone::CairoSurface surface(target, false /* no alpha */);
      OrthancStone::CairoContext context(surface);
      RenderMouseOverCairo(context, x, y);
    }
    else
    {
      Orthanc::ImageAccessor accessor;
      surface_.GetWriteableAccessor(accessor);
      Orthanc::ImageProcessing::Copy(accessor, target);

      OrthancStone::CairoContext context(surface_);
      RenderMouseOverCairo(context, x, y);

      Orthanc::ImageProcessing::Copy(target, accessor);
    }
  }
}
