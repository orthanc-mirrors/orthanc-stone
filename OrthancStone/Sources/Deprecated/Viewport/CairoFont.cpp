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


#include "CairoFont.h"

#include <Logging.h>
#include <OrthancException.h>

namespace Deprecated
{
  CairoFont::CairoFont(const char* family,
                       cairo_font_slant_t slant,
                       cairo_font_weight_t weight)
  {
    font_ = cairo_toy_font_face_create(family, slant, weight);
    if (font_ == NULL)
    {
      LOG(ERROR) << "Unknown font: " << family;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
    }
  }


  CairoFont::~CairoFont()
  {
    if (font_ != NULL)
    {
      cairo_font_face_destroy(font_);
    }
  }


  void CairoFont::Draw(OrthancStone::CairoContext& context,
                       const std::string& text,
                       double size)
  {
    if (size <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    cairo_t* cr = context.GetObject();
    cairo_set_font_face(cr, font_);
    cairo_set_font_size(cr, size);
    cairo_show_text(cr, text.c_str());    
  }
}