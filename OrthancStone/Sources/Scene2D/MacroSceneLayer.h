/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2021 Osimis S.A., Belgium
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


#pragma once

#include "ISceneLayer.h"

#include <Compatibility.h>  // For ORTHANC_OVERRIDE

#include <vector>

namespace OrthancStone
{
  /**
   * A "macro layer" is a group of sublayers that are handled as a
   * whole.
   **/
  class MacroSceneLayer : public ISceneLayer
  {
  private:
    std::vector<ISceneLayer*>  layers_;
    uint64_t                   revision_;

  protected:
    void BumpRevision()
    {
      // this is *not* thread-safe!!!  => (SJO) no problem, Stone assumes mono-threading
      revision_++;
    }

  public:
    MacroSceneLayer() :
      revision_(0)
    {
    }

    virtual ~MacroSceneLayer()
    {
      Clear();
    }

    void Clear();      

    void Reserve(size_t size)
    {
      layers_.reserve(size);
    }

    void AddLayer(ISceneLayer* layer);  // takes ownership

    size_t GetSize() const
    {
      return layers_.size();
    }

    const ISceneLayer& GetLayer(size_t i) const;

    virtual ISceneLayer* Clone() const ORTHANC_OVERRIDE;

    virtual Type GetType() const ORTHANC_OVERRIDE
    {
      return Type_Macro;
    }

    virtual void GetBoundingBox(Extent2D& target) const ORTHANC_OVERRIDE;

    virtual uint64_t GetRevision() const ORTHANC_OVERRIDE
    {
      return revision_;
    }
  };
}
