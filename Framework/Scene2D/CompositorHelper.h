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

#include "Scene2D.h"

namespace OrthancStone
{
  class CompositorHelper : protected Scene2D::IVisitor
  {
  public:
    class ILayerRenderer : public boost::noncopyable
    {
    public:
      virtual ~ILayerRenderer()
      {
      }

      virtual void Render(const AffineTransform2D& transform) = 0;

      // "Update()" is only called if the type of the layer has not changed
      virtual void Update(const ISceneLayer& layer) = 0;
    };

    class IRendererFactory : public boost::noncopyable
    {
    public:
      virtual ~IRendererFactory()
      {
      }

      virtual ILayerRenderer* Create(const ISceneLayer& layer) = 0;
    };

  private:
    class Item;

    typedef std::map<int, Item*>  Content;

    Scene2D&           scene_;
    IRendererFactory&  factory_;
    Content            content_;
    AffineTransform2D  sceneTransform_;

  protected:
    virtual void Visit(const ISceneLayer& layer,
                       int depth);

  public:
    CompositorHelper(Scene2D& scene,
                     IRendererFactory& factory) :
      scene_(scene),
      factory_(factory)
    {
    }

    ~CompositorHelper();

    void Refresh(unsigned int canvasWidth,
                 unsigned int canvasHeight);
  };
}
