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

#include "ISceneLayer.h"
#include "../Toolbox/AffineTransform2D.h"

#include <map>

namespace OrthancStone
{
  class Scene2D : public boost::noncopyable
  {
  public:
    class IVisitor : public boost::noncopyable
    {
    public:
      virtual ~IVisitor()
      {
      }

      virtual void Visit(const ISceneLayer& layer,
                         int depth) = 0;
    };

  private:
    typedef std::map<int, ISceneLayer*>  Content;

    Content    content_;

    AffineTransform2D  sceneToCanvas_;
    AffineTransform2D  canvasToScene_;

    Scene2D(const Scene2D& other);
    
  public:
    Scene2D()
    {
    }
    
    ~Scene2D();

    Scene2D* Clone() const
    {
      return new Scene2D(*this);
    }

    void SetLayer(int depth,
                  ISceneLayer* layer);  // Takes ownership

    void DeleteLayer(int depth);

    void Apply(IVisitor& visitor) const;

    const AffineTransform2D& GetSceneToCanvasTransform() const
    {
      return sceneToCanvas_;
    }

    const AffineTransform2D& GetCanvasToSceneTransform() const
    {
      return canvasToScene_;
    }

    void SetSceneToCanvasTransform(const AffineTransform2D& transform);

    void FitContent(unsigned int canvasWidth,
                    unsigned int canvasHeight);
  };
}