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
  void LayerSourceBase::NotifyGeometryReady()
  {
    if (!started_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    if (observer_ != NULL)
    {
      observer_->NotifyGeometryReady(*this);
    }
  }  
    
  void LayerSourceBase::NotifySourceChange()
  {
    if (!started_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    if (observer_ != NULL)
    {
      observer_->NotifySourceChange(*this);
    }
  }

  void LayerSourceBase::NotifySliceChange(const SliceGeometry& slice)
  {
    if (!started_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    if (observer_ != NULL)
    {
      observer_->NotifySliceChange(*this, slice);
    }
  }

  void LayerSourceBase::NotifyLayerReady(ILayerRenderer* layer,
                                         const SliceGeometry& viewportSlice)
  {
    std::auto_ptr<ILayerRenderer> tmp(layer);
    
    if (!started_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    if (layer == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    
    if (observer_ != NULL)
    {
      observer_->NotifyLayerReady(tmp.release(), *this, viewportSlice);
    }
  }

  void LayerSourceBase::NotifyLayerError(const SliceGeometry& viewportSlice)
  {
    if (!started_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    if (observer_ != NULL)
    {
      observer_->NotifyLayerError(*this, viewportSlice);
    }
  }

  void LayerSourceBase::SetObserver(IObserver& observer)
  {
    if (started_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

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

  void LayerSourceBase::Start()
  {
    if (started_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      started_ = true;
      StartInternal();
    }
  }
}
