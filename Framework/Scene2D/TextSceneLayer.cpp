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


#include "TextSceneLayer.h"

namespace OrthancStone
{
  TextSceneLayer::TextSceneLayer(double x,
                                 double y,
                                 const std::string& utf8,
                                 size_t fontIndex,
                                 BitmapAnchor anchor,
                                 unsigned int border) :
    x_(x),
    y_(y),
    utf8_(utf8),
    fontIndex_(fontIndex),
    anchor_(anchor),
    border_(border)
  {
  }


  ISceneLayer* TextSceneLayer::Clone() const
  {
    std::auto_ptr<TextSceneLayer> cloned(new TextSceneLayer(x_, y_, utf8_, fontIndex_, anchor_, border_));
    cloned->SetColor(GetRed(), GetGreen(), GetBlue());
    return cloned.release();
  }
}
