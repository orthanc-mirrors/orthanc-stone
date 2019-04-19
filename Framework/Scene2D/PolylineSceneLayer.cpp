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


#include "PolylineSceneLayer.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  ISceneLayer* PolylineSceneLayer::Clone() const
  {
    std::auto_ptr<PolylineSceneLayer> cloned(new PolylineSceneLayer);
    cloned->Copy(*this);
    return cloned.release();
  }
    

  void PolylineSceneLayer::SetThickness(double thickness)
  {
    if (thickness <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      thickness_ = thickness;
    }
  }


  void PolylineSceneLayer::Copy(const PolylineSceneLayer& from)
  {
    SetColor(from.GetRed(), from.GetGreen(), from.GetBlue());
    chains_ = from.chains_;
    closed_ = from.closed_;
    thickness_ = from.thickness_;
  }

  
  void PolylineSceneLayer::Reserve(size_t countChains)
  {
    chains_.reserve(countChains);
    closed_.reserve(countChains);
  }

  
  void PolylineSceneLayer::AddChain(const Chain& chain,
                                    bool isClosed)
  {
    if (!chain.empty())
    {
      chains_.push_back(chain);
      closed_.push_back(isClosed);
    }
  }


  const PolylineSceneLayer::Chain& PolylineSceneLayer::GetChain(size_t i) const
  {
    if (i < chains_.size())
    {
      return chains_[i];
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }

  
  bool PolylineSceneLayer::IsClosedChain(size_t i) const
  {
    if (i < closed_.size())
    {
      return closed_[i];
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  bool PolylineSceneLayer::GetBoundingBox(Extent2D& target) const
  {
    target.Reset();

    for (size_t i = 0; i < chains_.size(); i++)
    {
      for (size_t j = 0; j < chains_[i].size(); j++)
      {
        const ScenePoint2D& p = chains_[i][j];
        target.AddPoint(p.GetX(), p.GetY());
      }
    }

    return true;
  }
}
