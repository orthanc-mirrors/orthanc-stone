/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "SiblingSliceLocationFactory.h"

#include "LineLayerRenderer.h"

namespace OrthancStone
{
  SiblingSliceLocationFactory::SiblingSliceLocationFactory(LayeredSceneWidget& owner,
                                                           LayeredSceneWidget& sibling) :
    owner_(owner),
    sibling_(sibling),
    hasLayerIndex_(false)
  {
    style_.SetColor(0, 255, 0);
    slice_ = sibling.GetSlice();
    sibling_.Register(*this);
  }


  void SiblingSliceLocationFactory::NotifySliceChange(const LayeredSceneWidget& source,
                                                      const SliceGeometry& slice)
  {
    if (&source == &sibling_)
    {
      SetSlice(slice);
    }
  }


  void SiblingSliceLocationFactory::SetLayerIndex(size_t layerIndex)
  {
    boost::mutex::scoped_lock lock(mutex_);
    hasLayerIndex_ = true;
    layerIndex_ = layerIndex;
  }


  void SiblingSliceLocationFactory::SetStyle(const RenderStyle& style)
  {
    boost::mutex::scoped_lock lock(mutex_);
    style_ = style;
  }


  RenderStyle SiblingSliceLocationFactory::GetRenderStyle()
  {
    boost::mutex::scoped_lock lock(mutex_);
    return style_;
  }


  void SiblingSliceLocationFactory::SetSlice(const SliceGeometry& slice)
  {
    boost::mutex::scoped_lock lock(mutex_);
    slice_ = slice;

    if (hasLayerIndex_)
    {
      owner_.InvalidateLayer(layerIndex_);
    }
  }


  ILayerRenderer* SiblingSliceLocationFactory::CreateLayerRenderer(const SliceGeometry& viewportSlice)
  {
    Vector p, d;
    RenderStyle style;

    {
      boost::mutex::scoped_lock lock(mutex_);

      style = style_;

      // Compute the line of intersection between the two slices
      if (!GeometryToolbox::IntersectTwoPlanes(p, d, 
                                               slice_.GetOrigin(), slice_.GetNormal(),
                                               viewportSlice.GetOrigin(), viewportSlice.GetNormal()))
      {
        // The two slice are parallel, don't try and display the intersection
        return NULL;
      }
    }

    double x1, y1, x2, y2;
    viewportSlice.ProjectPoint(x1, y1, p);
    viewportSlice.ProjectPoint(x2, y2, p + 1000.0 * d);

    double sx1, sy1, sx2, sy2;
    owner_.GetView().GetSceneExtent(sx1, sy1, sx2, sy2);
        
    if (GeometryToolbox::ClipLineToRectangle(x1, y1, x2, y2, 
                                             x1, y1, x2, y2,
                                             sx1, sy1, sx2, sy2))
    {
      std::auto_ptr<ILayerRenderer> layer(new LineLayerRenderer(x1, y1, x2, y2));
      layer->SetLayerStyle(style);
      return layer.release();
    }
    else
    {
      // Parallel slices
      return NULL;
    }
  }


  ISliceableVolume& SiblingSliceLocationFactory::GetSourceVolume() const
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }


  void SiblingSliceLocationFactory::Configure(LayeredSceneWidget& a,
                                              LayeredSceneWidget& b)
  {
    {
      size_t layerIndex;
      ILayerRendererFactory& factory = a.AddLayer(layerIndex, new SiblingSliceLocationFactory(a, b));
      dynamic_cast<SiblingSliceLocationFactory&>(factory).SetLayerIndex(layerIndex);
    }

    {
      size_t layerIndex;
      ILayerRendererFactory& factory = b.AddLayer(layerIndex, new SiblingSliceLocationFactory(b, a));
      dynamic_cast<SiblingSliceLocationFactory&>(factory).SetLayerIndex(layerIndex);
    }
  }
}
