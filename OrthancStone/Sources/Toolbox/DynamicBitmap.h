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


#pragma once

#include <Compatibility.h>
#include <IDynamicObject.h>
#include <Images/ImageAccessor.h>

#include <memory>

namespace OrthancStone
{
  class DynamicBitmap : public Orthanc::IDynamicObject
  {
  private:
    std::unique_ptr<Orthanc::ImageAccessor>  bitmap_;

  public:
    DynamicBitmap(const Orthanc::ImageAccessor& bitmap);

    const Orthanc::ImageAccessor& GetBitmap() const
    {
      return *bitmap_;
    }
  };
}