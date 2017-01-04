/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "CairoWidget.h"

#include "../../Resources/Orthanc/Core/Images/ImageProcessing.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"

namespace OrthancStone
{
  static bool IsAligned(const Orthanc::ImageAccessor& target)
  {
    // TODO
    return true;
  }


  void CairoWidget::SetSize(unsigned int width,
                            unsigned int height)
  {
    surface_.SetSize(width, height);
  }
  

  bool CairoWidget::Render(Orthanc::ImageAccessor& target)
  {
    // Don't call the base class here, as
    // "ClearBackgroundCairo()" is a faster alternative

    if (IsAligned(target))
    {
      CairoSurface surface(target);
      CairoContext context(surface);
      ClearBackgroundCairo(context);
      return RenderCairo(context);
    }
    else
    {
      CairoContext context(surface_);
      ClearBackgroundCairo(context);

      if (RenderCairo(context))
      {
        Orthanc::ImageProcessing::Copy(target, surface_.GetAccessor());
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
      CairoSurface surface(target);
      CairoContext context(surface);
      RenderMouseOverCairo(context, x, y);
    }
    else
    {
      Orthanc::ImageAccessor accessor = surface_.GetAccessor();
      Orthanc::ImageProcessing::Copy(accessor, target);

      CairoContext context(surface_);
      RenderMouseOverCairo(context, x, y);

      Orthanc::ImageProcessing::Copy(target, accessor);
    }
  }
}
