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

#include <boost/noncopyable.hpp>

namespace OrthancStone
{
  class IVolumeLoader : public boost::noncopyable
  {
  public:
    class IObserver : public boost::noncopyable
    {
    public:
      virtual ~IObserver()
      {
      }
      
      virtual void NotifyGeometryReady(const IVolumeLoader& loader) = 0;
      
      virtual void NotifyGeometryError(const IVolumeLoader& loader) = 0;
      
      // Triggered if the content of several slices in the loader has
      // changed
      virtual void NotifyContentChange(const IVolumeLoader& loader) = 0;
    };
    
    virtual ~IVolumeLoader()
    {
    }

    virtual void Register(IObserver& observer) = 0;
  };
}