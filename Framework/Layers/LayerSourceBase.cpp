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


#include "LayerSourceBase.h"

#include "../../Resources/Orthanc/Core/OrthancException.h"

namespace OrthancStone
{
  void LayerSourceBase::NotifyContentChange()
  {
    if (observer_ != NULL)
    {
      observer_->NotifyContentChange(*this);
    }
  }

  void LayerSourceBase::NotifySliceChange(const Slice& slice)
  {
    if (observer_ != NULL)
    {
      observer_->NotifySliceChange(*this, slice);
    }
  }

  void LayerSourceBase::NotifyLayerReady(ILayerRenderer* layer,
                                         const Slice& slice)
  {
    std::auto_ptr<ILayerRenderer> tmp(layer);
    
    if (layer == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    
    if (observer_ != NULL)
    {
      observer_->NotifyLayerReady(tmp.release(), *this, slice);
    }
  }

  void LayerSourceBase::NotifyLayerError(const SliceGeometry& slice)
  {
    if (observer_ != NULL)
    {
      observer_->NotifyLayerError(*this, slice);
    }
  }

  void LayerSourceBase::SetObserver(IObserver& observer)
  {
    if (observer_ == NULL)
    {
      observer_ = &observer;
    }
    else
    {
      // Cannot add more than one observer
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
}
