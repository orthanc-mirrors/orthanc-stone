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

#if !defined(ORTHANC_SANDBOXED)
#  error The macro ORTHANC_SANDBOXED must be defined
#endif

#if ORTHANC_SANDBOXED == 1
#  error The class CairoFont cannot be used in sandboxed environments
#endif

#include "../../Wrappers/CairoContext.h"

namespace Deprecated
{
  class CairoFont : public boost::noncopyable
  {
  private:
    cairo_font_face_t*  font_;

  public:
    CairoFont(const char* family,
              cairo_font_slant_t slant,
              cairo_font_weight_t weight);

    ~CairoFont();

    void Draw(OrthancStone::CairoContext& context,
              const std::string& text,
              double size);
  };
}
