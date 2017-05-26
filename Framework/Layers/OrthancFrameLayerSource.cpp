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


#include "OrthancFrameLayerSource.h"

#include "FrameRenderer.h"
#include "../../Resources/Orthanc/Core/Images/PngReader.h"
#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../Toolbox/DicomFrameConverter.h"

#include <boost/lexical_cast.hpp>


// TODO REMOVE THIS
#include "../Widgets/LayerWidget.h"

namespace OrthancStone
{
  void OrthancFrameLayerSource::NotifyGeometryReady(const OrthancSlicesLoader& loader)
  {
#if 0
    if (loader.GetSliceCount() > 0)
    {
      // Make sure all the slices are parallel. TODO Alleviate this constraint
      for (size_t i = 1; i < loader.GetSliceCount(); i++)
      {
        if (!GeometryToolbox::IsParallel(loader.GetSlice(i).GetGeometry().GetNormal(),
                                         loader.GetSlice(0).GetGeometry().GetNormal()))
        {
          LayerSourceBase::NotifyGeometryError();
          return;
        }
      }
    }

    LayerSourceBase::NotifyGeometryReady();
#endif

    // TODO REMOVE THIS
    /*if (GetObserver() != NULL)
    {
      dynamic_cast<LayerWidget*>(GetObserver())->SetSlice(loader.GetSlice(0).GetGeometry());
      }*/
  }

  void OrthancFrameLayerSource::NotifyGeometryError(const OrthancSlicesLoader& loader)
  {
#if 0
    LayerSourceBase::NotifyGeometryError();
#endif
  }

  void OrthancFrameLayerSource::NotifySliceImageReady(const OrthancSlicesLoader& loader,
                                                      unsigned int sliceIndex,
                                                      const Slice& slice,
                                                      Orthanc::ImageAccessor* image)
  {
    LayerSourceBase::NotifyLayerReady(FrameRenderer::CreateRenderer(image, slice, true), slice);
  }

  void OrthancFrameLayerSource::NotifySliceImageError(const OrthancSlicesLoader& loader,
                                                      unsigned int sliceIndex,
                                                      const Slice& slice)
  {
    LayerSourceBase::NotifyLayerError(slice.GetGeometry());
  }

  OrthancFrameLayerSource::OrthancFrameLayerSource(IWebService& orthanc,
                                                   const std::string& instanceId,
                                                   unsigned int frame) :
    instanceId_(instanceId),
    frame_(frame),
    loader_(*this, orthanc)
  {
    loader_.ScheduleLoadInstance(instanceId_, frame_);
  }


  bool OrthancFrameLayerSource::GetExtent(double& x1,
                                          double& y1,
                                          double& x2,
                                          double& y2,
                                          const SliceGeometry& viewportSlice)
  {
    bool ok = false;

    if (loader_.IsGeometryReady())
    {
      double tx1, ty1, tx2, ty2;

      for (size_t i = 0; i < loader_.GetSliceCount(); i++)
      {
        if (FrameRenderer::ComputeFrameExtent(tx1, ty1, tx2, ty2, viewportSlice, loader_.GetSlice(i)))
        {
          if (ok)
          {
            x1 = std::min(x1, tx1);
            y1 = std::min(y1, ty1);
            x2 = std::min(x2, tx2);
            y2 = std::min(y2, ty2);
          }
          else
          {
            // This is the first slice parallel to the viewport
            x1 = tx1;
            y1 = ty1;
            x2 = tx2;
            y2 = ty2;
            ok = true;
          }
        }
      }
    }

    return ok;
  }

  
  void OrthancFrameLayerSource::ScheduleLayerCreation(const SliceGeometry& viewportSlice)
  {
    size_t index;

    if (loader_.IsGeometryReady())
    {
      if (loader_.LookupSlice(index, viewportSlice))
      {
        loader_.ScheduleLoadSliceImage(index);
      }
      else
      {
        LayerSourceBase::NotifyLayerError(viewportSlice);
      }
    }
  }
}
