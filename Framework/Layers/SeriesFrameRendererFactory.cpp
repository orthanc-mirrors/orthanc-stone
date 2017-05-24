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


#include "SeriesFrameRendererFactory.h"

#include "FrameRenderer.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/Toolbox.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/OrthancPluginException.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/DicomDatasetReader.h"


namespace OrthancStone
{
  void SeriesFrameRendererFactory::ReadCurrentFrameDataset(size_t frame)
  {
    if (currentDataset_.get() != NULL &&
        (fast_ || currentFrame_ == frame))
    {
      // The frame has not changed since the previous call, no need to
      // update the DICOM dataset
      return; 
    }
      
    currentDataset_.reset(loader_->DownloadDicom(frame));
    currentFrame_ = frame;

    if (currentDataset_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  void SeriesFrameRendererFactory::GetCurrentPixelSpacing(double& spacingX,
                                                          double& spacingY) const
  {
    if (currentDataset_.get() == NULL)
    {
      // There was no previous call "ReadCurrentFrameDataset()"
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    
    GeometryToolbox::GetPixelSpacing(spacingX, spacingY, *currentDataset_);
  }


  double SeriesFrameRendererFactory::GetCurrentSliceThickness() const
  {
    if (currentDataset_.get() == NULL)
    {
      // There was no previous call "ReadCurrentFrameDataset()"
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    
    try
    {
      OrthancPlugins::DicomDatasetReader reader(*currentDataset_);

      double thickness;
      if (reader.GetDoubleValue(thickness, OrthancPlugins::DICOM_TAG_SLICE_THICKNESS))
      {
        return thickness;
      }
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
    }

    // Some arbitrary large slice thickness
    return std::numeric_limits<double>::infinity();
  }


  SeriesFrameRendererFactory::SeriesFrameRendererFactory(ISeriesLoader* loader,   // Takes ownership
                                                         bool fast) :
    loader_(loader),
    currentFrame_(0),
    fast_(fast)
  {
    if (loader == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  bool SeriesFrameRendererFactory::GetExtent(double& x1,
                                             double& y1,
                                             double& x2,
                                             double& y2,
                                             const SliceGeometry& viewportSlice)
  {
    if (currentDataset_.get() == NULL)
    {
      // There has been no previous call to
      // "CreateLayerRenderer". Read some arbitrary DICOM frame, the
      // one at the middle of the series.
      unsigned int depth = loader_->GetGeometry().GetSliceCount();
      ReadCurrentFrameDataset(depth / 2);
    }

    double spacingX, spacingY;
    GetCurrentPixelSpacing(spacingX, spacingY);

    return FrameRenderer::ComputeFrameExtent(x1, y1, x2, y2, 
                                             viewportSlice, 
                                             loader_->GetGeometry().GetSlice(0), 
                                             loader_->GetWidth(), 
                                             loader_->GetHeight(),
                                             spacingX, spacingY);
  }


  ILayerRenderer* SeriesFrameRendererFactory::CreateLayerRenderer(const SliceGeometry& viewportSlice)
  {
    size_t closest;
    double distance;

    bool isOpposite;
    if (!GeometryToolbox::IsParallelOrOpposite(isOpposite, loader_->GetGeometry().GetNormal(), viewportSlice.GetNormal()) ||
        !loader_->GetGeometry().ComputeClosestSlice(closest, distance, viewportSlice.GetOrigin()))
    {
      // Unable to compute the slice in the series that is the
      // closest to the slice displayed by the viewport
      return NULL;
    }

    ReadCurrentFrameDataset(closest);
    assert(currentDataset_.get() != NULL);
        
    double spacingX, spacingY;
    GetCurrentPixelSpacing(spacingX, spacingY);

    if (distance <= GetCurrentSliceThickness() / 2.0)
    {
      SliceGeometry frameSlice(*currentDataset_);
      return FrameRenderer::CreateRenderer(loader_->DownloadFrame(closest), 
                                           frameSlice,
                                           *currentDataset_, 
                                           spacingX, spacingY,
                                           true);
    }
    else
    {
      // The closest slice of the series is too far away from the
      // slice displayed by the viewport
      return NULL;
    }
  }


  ISliceableVolume& SeriesFrameRendererFactory::GetSourceVolume() const
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
  }
}
