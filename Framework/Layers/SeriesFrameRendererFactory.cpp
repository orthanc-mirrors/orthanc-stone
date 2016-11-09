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


#include "SeriesFrameRendererFactory.h"

#include "FrameRenderer.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"

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
    
    currentDataset_->GetPixelSpacing(spacingX, spacingY);
  }


  double SeriesFrameRendererFactory::GetCurrentSliceThickness() const
  {
    if (currentDataset_.get() == NULL)
    {
      // There was no previous call "ReadCurrentFrameDataset()"
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
      
    if (currentDataset_->HasTag(DICOM_TAG_SLICE_THICKNESS))
    {
      return currentDataset_->GetFloatValue(DICOM_TAG_SLICE_THICKNESS);
    }
    else
    {
      // Some arbitrary large slice thickness
      return std::numeric_limits<double>::infinity();
    }
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
                                           viewportSlice, 
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
