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
  void LayerSourceBase::NotifySourceChange()
  {
    if (observer_ != NULL)
    {
      observer_->NotifySourceChange(*this);
    }
  }

  void LayerSourceBase::NotifySliceChange(const SliceGeometry& slice)
  {
    if (observer_ != NULL)
    {
      observer_->NotifySliceChange(*this, slice);
    }
  }

  void LayerSourceBase::NotifyLayerReady(ILayerRenderer* layer,
                                         ILayerSource& source,
                                         const SliceGeometry& viewportSlice)
  {
    if (layer == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    
    if (observer_ != NULL)
    {
      observer_->NotifyLayerReady(layer, *this, viewportSlice);
    }
    else
    {
      delete layer;
    }
  }

  void LayerSourceBase::NotifyLayerError(ILayerSource& source,
                                         const SliceGeometry& viewportSlice)
  {
    if (observer_ != NULL)
    {
      observer_->NotifyLayerError(*this, viewportSlice);
    }
  }

  void LayerSourceBase::SetObserver(IObserver& observer)
  {
    observer_ = &observer;
  }
}
