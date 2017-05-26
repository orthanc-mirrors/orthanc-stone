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


#include "SlicedVolumeBase.h"

namespace OrthancStone
{
  namespace
  {
    struct GeometryReadyFunctor
    {
      void operator() (ISlicedVolume::IObserver& observer,
                       const ISlicedVolume& source)
      {
        observer.NotifyGeometryReady(source);
      }
    };

    struct GeometryErrorFunctor
    {
      void operator() (ISlicedVolume::IObserver& observer,
                       const ISlicedVolume& source)
      {
        observer.NotifyGeometryError(source);
      }
    };

    struct ContentChangeFunctor
    {
      void operator() (ISlicedVolume::IObserver& observer,
                       const ISlicedVolume& source)
      {
        observer.NotifyContentChange(source);
      }
    };

    struct SliceChangeFunctor
    {
      size_t sliceIndex_;
      const Slice& slice_;

      SliceChangeFunctor(size_t sliceIndex,
                         const Slice& slice) :
        sliceIndex_(sliceIndex),
        slice_(slice)
      {
      }

      void operator() (ISlicedVolume::IObserver& observer,
                       const ISlicedVolume& source)
      {
        observer.NotifySliceChange(source, sliceIndex_, slice_);
      }
    };
  }

  void SlicedVolumeBase::NotifyGeometryReady()
  {
    GeometryReadyFunctor functor;
    observers_.Notify(this, functor);
  }
      
  void SlicedVolumeBase::NotifyGeometryError()
  {
    GeometryErrorFunctor functor;
    observers_.Notify(this, functor);
  }
    
  void SlicedVolumeBase::NotifyContentChange()
  {
    ContentChangeFunctor functor;
    observers_.Notify(this, functor);
  }

  void SlicedVolumeBase::NotifySliceChange(size_t sliceIndex,
                                           const Slice& slice)
  {
    SliceChangeFunctor functor(sliceIndex, slice);
    observers_.Notify(this, functor);
  }
}
