/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include "MacroSceneLayer.h"

#include <OrthancException.h>

namespace OrthancStone
{
  void MacroSceneLayer::Clear()
  {
    for (size_t i = 0; i < layers_.size(); i++)
    {
      assert(layers_[i] != NULL);
      delete layers_[i];
    }

    layers_.clear();
    BumpRevision();
  }

  
  void MacroSceneLayer::AddLayer(ISceneLayer* layer)
  {
    if (layer == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      layers_.push_back(layer);
      BumpRevision();
    }
  }

  
  const ISceneLayer& MacroSceneLayer::GetLayer(size_t i) const
  {
    if (i >= layers_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(layers_[i] != NULL);
      return *layers_[i];
    }
  }


  ISceneLayer* MacroSceneLayer::Clone() const
  {
    std::unique_ptr<MacroSceneLayer> copy(new MacroSceneLayer);

    copy->Reserve(layers_.size());

    for (size_t i = 0; i < layers_.size(); i++)
    {
      assert(layers_[i] != NULL);
      copy->layers_.push_back(layers_[i]->Clone());
    }

    return copy.release();
  }
  

  void MacroSceneLayer::GetBoundingBox(Extent2D& target) const
  {
    target.Clear();

    for (size_t i = 0; i < layers_.size(); i++)
    {
      assert(layers_[i] != NULL);

      Extent2D subextent;
      layers_[i]->GetBoundingBox(subextent);
      target.Union(subextent);
    }
  }
}