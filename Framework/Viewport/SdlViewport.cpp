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

#include "SdlViewport.h"

#include <Core/OrthancException.h>

#include <boost/make_shared.hpp>

namespace OrthancStone
{
  SdlViewport::SdlViewport(const char* title,
                           unsigned int width,
                           unsigned int height,
                           bool allowDpiScaling) :
    ViewportBase(title),
    context_(title, width, height, allowDpiScaling),
    compositor_(context_, GetScene())
  {
  }

  SdlViewport::SdlViewport(const char* title,
                           unsigned int width,
                           unsigned int height,
                           boost::shared_ptr<Scene2D>& scene,
                           bool allowDpiScaling) :
    ViewportBase(title, scene),
    context_(title, width, height, allowDpiScaling),
    compositor_(context_, GetScene())
  {
  }
}