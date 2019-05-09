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

#include "ColorSceneLayer.h"
#include "ScenePoint2D.h"

#include <vector>

namespace OrthancStone
{
  class PolylineSceneLayer : public ColorSceneLayer
  {
  public:
    typedef std::vector<ScenePoint2D>  Chain;

  private:
    std::vector<Chain>  chains_;
    std::vector<bool>   closed_;
    double              thickness_;

  public:
    PolylineSceneLayer() :
      thickness_(1.0)
    {
    }

    virtual ISceneLayer* Clone() const;

    void SetThickness(double thickness);

    double GetThickness() const
    {
      return thickness_;
    }

    void Copy(const PolylineSceneLayer& from);

    void Reserve(size_t countChains);

    void AddChain(const Chain& chain,
                  bool isClosed);

    void ClearAllChains();

    size_t GetChainsCount() const
    {
      return chains_.size();
    }

    const Chain& GetChain(size_t i) const;

    bool IsClosedChain(size_t i) const;

    virtual Type GetType() const
    {
      return Type_Polyline;
    }

    virtual bool GetBoundingBox(Extent2D& target) const;
   
  };
}
