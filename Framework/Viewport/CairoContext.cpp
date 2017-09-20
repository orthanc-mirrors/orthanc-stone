/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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


#include "CairoContext.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  CairoContext::CairoContext(CairoSurface& surface)
  {
    context_ = cairo_create(surface.GetObject());
    if (!context_)
    {
      LOG(ERROR) << "Cannot create Cairo drawing context";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  CairoContext::~CairoContext()
  {
    if (context_ != NULL)
    {
      cairo_destroy(context_);
      context_ = NULL;
    }
  }


  void CairoContext::SetSourceColor(uint8_t red,
                                    uint8_t green,
                                    uint8_t blue)
  {
    cairo_set_source_rgb(context_,
                         static_cast<float>(red) / 255.0f,
                         static_cast<float>(green) / 255.0f,
                         static_cast<float>(blue) / 255.0f);
  }
}
