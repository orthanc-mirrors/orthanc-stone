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


#include "Scene2D.h"

#include <Core/OrthancException.h>


namespace OrthancStone
{
  Scene2D::Scene2D(const Scene2D& other) :
    sceneToCanvas_(other.sceneToCanvas_),
    canvasToScene_(other.canvasToScene_)
  {
    for (Content::const_iterator it = other.content_.begin();
         it != other.content_.end(); ++it)
    {
      content_[it->first] = it->second->Clone();
    }
  }

    
  Scene2D::~Scene2D()
  {
    for (Content::iterator it = content_.begin(); 
         it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }
  }


  void Scene2D::SetLayer(int depth,
                         ISceneLayer* layer)  // Takes ownership
  {
    std::auto_ptr<ISceneLayer> protection(layer);

    if (layer == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    Content::iterator found = content_.find(depth);

    if (found == content_.end())
    {
      content_[depth] = protection.release();
    }
    else
    {
      assert(found->second != NULL);
      delete found->second;
      found->second = protection.release();
    }
  }


  void Scene2D::DeleteLayer(int depth)
  {
    Content::iterator found = content_.find(depth);

    if (found != content_.end())
    {
      assert(found->second != NULL);
      delete found->second;
      content_.erase(found);
    }    
  }

  
  void Scene2D::Apply(IVisitor& visitor) const
  {
    for (Content::const_iterator it = content_.begin(); 
         it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      visitor.Visit(*it->second, it->first);
    }
  }


  void Scene2D::SetSceneToCanvasTransform(const AffineTransform2D& transform)
  {
    // Make sure the transform is invertible before making any change
    AffineTransform2D inverse = AffineTransform2D::Invert(transform);

    sceneToCanvas_ = transform;
    canvasToScene_ = inverse;
  }


  void Scene2D::FitContent(unsigned int canvasWidth,
                           unsigned int canvasHeight)
  {
    Extent2D extent;

    for (Content::const_iterator it = content_.begin(); 
         it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      
      Extent2D tmp;
      if (it->second->GetBoundingBox(tmp))
      {
        extent.Union(tmp);
      }
    }

    if (!extent.IsEmpty())
    {
      double zoomX = static_cast<double>(canvasWidth) / extent.GetWidth();
      double zoomY = static_cast<double>(canvasHeight) / extent.GetHeight();

      double zoom = std::min(zoomX, zoomY);
      if (LinearAlgebra::IsCloseToZero(zoom))
      {
        zoom = 1;
      }

      double panX = extent.GetCenterX();
      double panY = extent.GetCenterY();

      // Bring the center of the scene to (0,0)
      AffineTransform2D t1 = AffineTransform2D::CreateOffset(-panX, -panY);
      
      // Scale the scene
      AffineTransform2D t2 = AffineTransform2D::CreateScaling(zoom, zoom);

      SetSceneToCanvasTransform(AffineTransform2D::Combine(t2, t1));
    }
  }
}