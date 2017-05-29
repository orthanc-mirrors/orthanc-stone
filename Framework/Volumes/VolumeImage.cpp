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


#include "VolumeImage.h"

#include "../../Resources/Orthanc/Core/Logging.h"
#include "../Layers/FrameRenderer.h"

namespace OrthancStone
{
  void VolumeImage::StoreUpdateTime()
  {
    lastUpdate_ = MessagingToolbox::Timestamp();
  }


  void VolumeImage::NotifyChange(bool force)
  {
    bool go = false;

    if (force)
    {
      go = true;
    }
    else
    {
      // Don't notify the observers more than 5 times per second
      MessagingToolbox::Timestamp now;
      go = (now.GetMillisecondsSince(lastUpdate_) > 200);
    }

    if (go)
    {
      StoreUpdateTime();
      observers_.NotifyChange(this);
    }
  }


  void VolumeImage::LoadThread(VolumeImage* that)
  {
    while (that->continue_)
    {
      bool complete = false;
      bool done = that->policy_->DownloadStep(complete);

      if (complete)
      {
        that->loadingComplete_ = true;
      }

      if (done)
      {
        break;
      }
      else
      {
        that->NotifyChange(false);
      }
    }

    that->NotifyChange(true);
  }


  VolumeImage::VolumeImage(ISeriesLoader* loader) :   // Takes ownership
    loader_(loader),
    threads_(1),
    started_(false),
    continue_(false),
    loadingComplete_(false)
  {
    if (loader == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    const size_t depth = loader_->GetGeometry().GetSliceCount();
      
    if (depth < 2)
    {
      // Empty or flat series
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    // TODO Check pixel spacing, slice thickness, and windowing are
    // constant across slices
    referenceDataset_.reset(loader->DownloadDicom(0));

    double spacingZ;

    {
      // Project the origin of the first and last slices onto the normal
      const SliceGeometry& s1 = loader_->GetGeometry().GetSlice(0);
      const SliceGeometry& s2 = loader_->GetGeometry().GetSlice(depth - 1);
      const Vector& normal = loader_->GetGeometry().GetNormal();

      double p1 = boost::numeric::ublas::inner_prod(s1.GetOrigin(), normal);
      double p2 = boost::numeric::ublas::inner_prod(s2.GetOrigin(), normal);

      spacingZ = fabs(p2 - p1) / static_cast<double>(depth);

      // TODO Check that all slices are evenly distributed
    }      

    buffer_.reset(new ImageBuffer3D(loader_->GetPixelFormat(), 
                                    loader_->GetWidth(), 
                                    loader_->GetHeight(),
                                    depth));
    buffer_->Clear();
    buffer_->SetAxialGeometry(loader_->GetGeometry().GetSlice(0));

    double spacingX, spacingY;
    GeometryToolbox::GetPixelSpacing(spacingX, spacingY, *referenceDataset_);
    buffer_->SetVoxelDimensions(spacingX, spacingY, spacingZ);

    // These 3 values are only used to speed up the LayerFactory
    axialGeometry_.reset(buffer_->GetGeometry(VolumeProjection_Axial));
    coronalGeometry_.reset(buffer_->GetGeometry(VolumeProjection_Coronal));
    sagittalGeometry_.reset(buffer_->GetGeometry(VolumeProjection_Sagittal));
  }


  VolumeImage::~VolumeImage()
  {
    Stop();
      
    for (size_t i = 0; i < threads_.size(); i++)
    {
      if (threads_[i] != NULL)
      {
        delete threads_[i];
      }
    }
  }


  void VolumeImage::SetDownloadPolicy(IDownloadPolicy* policy)   // Takes ownership
  {
    if (started_)
    {
      LOG(ERROR) << "Cannot change the number of threads after a call to Start()";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    policy_.reset(policy);
  }


  void VolumeImage::SetThreadCount(size_t count)
  {
    if (started_)
    {
      LOG(ERROR) << "Cannot change the number of threads after a call to Start()";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    if (count <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    threads_.resize(count);
  }


  void VolumeImage::Register(IObserver& observer)
  {
    observers_.Register(observer);
  }


  void VolumeImage::Unregister(IObserver& observer)
  {
    observers_.Unregister(observer);
  }


  void VolumeImage::Start()
  {
    if (started_)
    {
      LOG(ERROR) << "Cannot call Start() twice";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    started_ = true;
    StoreUpdateTime();

    if (policy_.get() != NULL &&
        threads_.size() > 0)
    {
      continue_ = true;
      policy_->Initialize(*buffer_, *loader_);

      for (size_t i = 0; i < threads_.size(); i++)
      {
        assert(threads_[i] == NULL);
        threads_[i] = new boost::thread(LoadThread, this);
      }
    }
  }


  void VolumeImage::Stop()
  {
    if (!started_)
    {
      LOG(ERROR) << "Cannot call Stop() without calling Start() beforehand";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    if (continue_)
    {
      continue_ = false;

      for (size_t i = 0; i < threads_.size(); i++)
      {
        if (threads_[i]->joinable())
        {
          threads_[i]->join();
        }
      }

      assert(policy_.get() != NULL);
      policy_->Finalize();
    }
  }


  ParallelSlices* VolumeImage::GetGeometry(VolumeProjection projection,
                                           bool reverse)
  {
    std::auto_ptr<ParallelSlices> slices(buffer_->GetGeometry(projection));

    if (reverse)
    {
      return slices->Reverse();
    }
    else
    {
      return slices.release();
    }
  }


  bool VolumeImage::DetectProjection(VolumeProjection& projection,
                                     bool& reverse,
                                     const SliceGeometry& viewportSlice)
  {
    if (GeometryToolbox::IsParallelOrOpposite(reverse, viewportSlice.GetNormal(), axialGeometry_->GetNormal()))
    {
      projection = VolumeProjection_Axial;
      return true;
    }
    else if (GeometryToolbox::IsParallelOrOpposite(reverse, viewportSlice.GetNormal(), sagittalGeometry_->GetNormal()))
    {
      projection = VolumeProjection_Sagittal;
      return true;
    }
    else if (GeometryToolbox::IsParallelOrOpposite(reverse, viewportSlice.GetNormal(), coronalGeometry_->GetNormal()))
    {
      projection = VolumeProjection_Coronal;
      return true;
    }
    else
    {
      return false;
    }
  }


  const ParallelSlices& VolumeImage::GetGeometryInternal(VolumeProjection projection)
  {
    switch (projection)
    {
      case VolumeProjection_Axial:
        return *axialGeometry_;

      case VolumeProjection_Sagittal:
        return *sagittalGeometry_;

      case VolumeProjection_Coronal:
        return *coronalGeometry_;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  bool VolumeImage::LayerFactory::GetExtent(double& x1,
                                            double& y1,
                                            double& x2,
                                            double& y2,
                                            const SliceGeometry& viewportSlice)
  {
    VolumeProjection projection;
    bool reverse;

    if (that_.buffer_->GetWidth() == 0 ||
        that_.buffer_->GetHeight() == 0 ||
        that_.buffer_->GetDepth() == 0 ||
        !that_.DetectProjection(projection, reverse, viewportSlice))
    {
      return false;
    }
    else
    {
      Vector spacing = that_.GetVoxelDimensions(projection);

      unsigned int width, height;
      that_.buffer_->GetSliceSize(width, height, projection);

      // As the slices of the volumic image are arranged in a box,
      // we only consider one single reference slice (the one with index 0).
      const SliceGeometry& volumeSlice = that_.GetGeometryInternal(projection).GetSlice(0);

      return FrameRenderer::ComputeFrameExtent(x1, y1, x2, y2, 
                                               viewportSlice, volumeSlice,
                                               width, height,
                                               spacing[0], spacing[1]);
    }
  }


  ILayerRenderer* VolumeImage::LayerFactory::CreateLayerRenderer(const SliceGeometry& viewportSlice)
  {
    VolumeProjection projection;
    bool reverse;
    
    if (that_.buffer_->GetWidth() == 0 ||
        that_.buffer_->GetHeight() == 0 ||
        that_.buffer_->GetDepth() == 0 ||
        !that_.DetectProjection(projection, reverse, viewportSlice))
    {
      return NULL;
    }

    const ParallelSlices& geometry = that_.GetGeometryInternal(projection);

    size_t closest;
    double distance;

    const Vector spacing = that_.GetVoxelDimensions(projection);
    const double sliceThickness = spacing[2];

    if (geometry.ComputeClosestSlice(closest, distance, viewportSlice.GetOrigin()) &&
        distance <= sliceThickness / 2.0)
    {
      bool isFullQuality;

      if (projection == VolumeProjection_Axial &&
          that_.policy_.get() != NULL)
      {
        isFullQuality = that_.policy_->IsFullQualityAxial(closest);
      }
      else
      {
        isFullQuality = that_.IsLoadingComplete();
      }

      std::auto_ptr<Orthanc::Image> frame;
      SliceGeometry frameSlice = geometry.GetSlice(closest);

      {
        ImageBuffer3D::SliceReader reader(*that_.buffer_, projection, closest);
        frame.reset(Orthanc::Image::Clone(reader.GetAccessor()));
      }

      return FrameRenderer::CreateRenderer(frame.release(), 
                                           frameSlice, 
                                           *that_.referenceDataset_, 
                                           spacing[0], spacing[1],
                                           isFullQuality);
    }
    else
    {
      return NULL;
    }
  }
}
