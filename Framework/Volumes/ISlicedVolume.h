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

#include "../Messages/IObservable.h"
#include "../Toolbox/Slice.h"

namespace OrthancStone
{
  class ISlicedVolume : public IObservable
  {
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, ContentChangedMessage, ISlicedVolume);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, GeometryErrorMessage, ISlicedVolume);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, GeometryReadyMessage, ISlicedVolume);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, VolumeReadyMessage, ISlicedVolume);


    class SliceContentChangedMessage : public OriginMessage<ISlicedVolume>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      size_t        sliceIndex_;
      const Slice&  slice_;
      
    public:
      SliceContentChangedMessage(ISlicedVolume& origin,
                                 size_t sliceIndex,
                                 const Slice& slice) :
        OriginMessage(origin),
        sliceIndex_(sliceIndex),
        slice_(slice)
      {
      }

      size_t GetSliceIndex() const
      {
        return sliceIndex_;
      }

      const Slice& GetSlice() const
      {
        return slice_;
      }
    };


    ISlicedVolume(MessageBroker& broker) :
      IObservable(broker)
    {
    }
    
    virtual size_t GetSliceCount() const = 0;

    virtual const Slice& GetSlice(size_t slice) const = 0;
  };
}
