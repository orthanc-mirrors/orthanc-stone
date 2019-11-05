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


#include "OrthancSeriesVolumeProgressiveLoader.h"

#include "../Toolbox/GeometryToolbox.h"
#include "../Volumes/DicomVolumeImageMPRSlicer.h"
#include "BasicFetchingItemsSorter.h"
#include "BasicFetchingStrategy.h"

#include <Core/Images/ImageProcessing.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  class OrthancSeriesVolumeProgressiveLoader::ExtractedSlice : public DicomVolumeImageMPRSlicer::Slice
  {
  private:
    const OrthancSeriesVolumeProgressiveLoader&  that_;

  public:
    ExtractedSlice(const OrthancSeriesVolumeProgressiveLoader& that,
                   const CoordinateSystem3D& plane) :
      DicomVolumeImageMPRSlicer::Slice(*that.volume_, plane),
      that_(that)
    {
      if (IsValid())
      {
        if (GetProjection() == VolumeProjection_Axial)
        {
          // For coronal and sagittal projections, we take the global
          // revision of the volume because even if a single slice changes,
          // this means the projection will yield a different result --> 
          // we must increase the revision as soon as any slice changes 
          SetRevision(that_.seriesGeometry_.GetSliceRevision(GetSliceIndex()));
        }
      
        if (that_.strategy_.get() != NULL &&
            GetProjection() == VolumeProjection_Axial)
        {
          that_.strategy_->SetCurrent(GetSliceIndex());
        }
      }
    }
  };

    
    
  void OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::CheckSlice(size_t index,
                                                                        const DicomInstanceParameters& reference) const
  {
    const DicomInstanceParameters& slice = *slices_[index];
      
    if (!GeometryToolbox::IsParallel(
          reference.GetGeometry().GetNormal(),
          slice.GetGeometry().GetNormal()))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadGeometry,
                                      "A slice in the volume image is not parallel to the others");
    }

    if (reference.GetExpectedPixelFormat() != slice.GetExpectedPixelFormat())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat,
                                      "The pixel format changes across the slices of the volume image");
    }

    if (reference.GetImageInformation().GetWidth() != slice.GetImageInformation().GetWidth() ||
        reference.GetImageInformation().GetHeight() != slice.GetImageInformation().GetHeight())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize,
                                      "The width/height of slices are not constant in the volume image");
    }

    if (!LinearAlgebra::IsNear(reference.GetPixelSpacingX(), slice.GetPixelSpacingX()) ||
        !LinearAlgebra::IsNear(reference.GetPixelSpacingY(), slice.GetPixelSpacingY()))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadGeometry,
                                      "The pixel spacing of the slices change across the volume image");
    }
  }

    
  void OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::CheckVolume() const
  {
    for (size_t i = 0; i < slices_.size(); i++)
    {
      assert(slices_[i] != NULL);
      if (slices_[i]->GetImageInformation().GetNumberOfFrames() != 1)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadGeometry,
                                        "This class does not support multi-frame images");
      }
    }

    if (slices_.size() != 0)
    {
      const DicomInstanceParameters& reference = *slices_[0];

      for (size_t i = 1; i < slices_.size(); i++)
      {
        CheckSlice(i, reference);
      }
    }
  }


  void OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::Clear()
  {
    for (size_t i = 0; i < slices_.size(); i++)
    {
      assert(slices_[i] != NULL);
      delete slices_[i];
    }

    slices_.clear();
    slicesRevision_.clear();
  }


  void OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::CheckSliceIndex(size_t index) const
  {
    if (!HasGeometry())
    {
      LOG(ERROR) << "OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::CheckSliceIndex(size_t index): (!HasGeometry())";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else if (index >= slices_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(slices_.size() == GetImageGeometry().GetDepth() &&
             slices_.size() == slicesRevision_.size());
    }
  }


  // WARNING: The payload of "slices" must be of class "DicomInstanceParameters"
  // (called with the slices created in LoadGeometry)
  void OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::ComputeGeometry(SlicesSorter& slices)
  {
    Clear();
      
    if (!slices.Sort())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                      "Cannot sort the 3D slices of a DICOM series");          
    }

    if (slices.GetSlicesCount() == 0)
    {
      geometry_.reset(new VolumeImageGeometry);
    }
    else
    {
      slices_.reserve(slices.GetSlicesCount());
      slicesRevision_.resize(slices.GetSlicesCount(), 0);

      for (size_t i = 0; i < slices.GetSlicesCount(); i++)
      {
        const DicomInstanceParameters& slice =
          dynamic_cast<const DicomInstanceParameters&>(slices.GetSlicePayload(i));
        slices_.push_back(new DicomInstanceParameters(slice));
      }

      CheckVolume();

      const double spacingZ = slices.ComputeSpacingBetweenSlices();
      LOG(INFO) << "Computed spacing between slices: " << spacingZ << "mm";
      
      const DicomInstanceParameters& parameters = *slices_[0];

      geometry_.reset(new VolumeImageGeometry);
      geometry_->SetSizeInVoxels(parameters.GetImageInformation().GetWidth(),
                         parameters.GetImageInformation().GetHeight(),
                         static_cast<unsigned int>(slices.GetSlicesCount()));
      geometry_->SetAxialGeometry(slices.GetSliceGeometry(0));
      geometry_->SetVoxelDimensions(parameters.GetPixelSpacingX(),
                                    parameters.GetPixelSpacingY(), spacingZ);
    }
  }


  const VolumeImageGeometry& OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::GetImageGeometry() const
  {
    if (!HasGeometry())
    {
      LOG(ERROR) << "OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::GetImageGeometry(): (!HasGeometry())";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      assert(slices_.size() == geometry_->GetDepth());
      return *geometry_;
    }
  }


  const DicomInstanceParameters& OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::GetSliceParameters(size_t index) const
  {
    CheckSliceIndex(index);
    return *slices_[index];
  }


  uint64_t OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::GetSliceRevision(size_t index) const
  {
    CheckSliceIndex(index);
    return slicesRevision_[index];
  }


  void OrthancSeriesVolumeProgressiveLoader::SeriesGeometry::IncrementSliceRevision(size_t index)
  {
    CheckSliceIndex(index);
    slicesRevision_[index] ++;
  }


  static unsigned int GetSliceIndexPayload(const OracleCommandBase& command)
  {
    return dynamic_cast< const Orthanc::SingleValueObject<unsigned int>& >(command.GetPayload()).GetValue();
  }


  void OrthancSeriesVolumeProgressiveLoader::ScheduleNextSliceDownload()
  {
    assert(strategy_.get() != NULL);
      
    unsigned int sliceIndex, quality;
      
    if (strategy_->GetNext(sliceIndex, quality))
    {
      assert(quality <= BEST_QUALITY);

      const DicomInstanceParameters& slice = seriesGeometry_.GetSliceParameters(sliceIndex);
          
      const std::string& instance = slice.GetOrthancInstanceIdentifier();
      if (instance.empty())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      std::auto_ptr<OracleCommandBase> command;
        
      if (quality == BEST_QUALITY)
      {
        std::auto_ptr<GetOrthancImageCommand> tmp(new GetOrthancImageCommand);
        // TODO: review the following comment. 
        // - Commented out by bgo on 2019-07-19 | reason: Alain has seen cases 
        //   where gzipping the uint16 image took 11 sec to produce 5mb. 
        //   The unzipped request was much much faster.
        // - Re-enabled on 2019-07-30. Reason: in Web Assembly, the browser 
        //   does not use the Accept-Encoding header and always requests
        //   compression. Furthermore, NOT 
        tmp->SetHttpHeader("Accept-Encoding", "gzip");
        tmp->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
        tmp->SetInstanceUri(instance, slice.GetExpectedPixelFormat());
        tmp->SetExpectedPixelFormat(slice.GetExpectedPixelFormat());
        command.reset(tmp.release());
      }
      else
      {
        std::auto_ptr<GetOrthancWebViewerJpegCommand> tmp(new GetOrthancWebViewerJpegCommand);
        // TODO: review the following comment. Commented out by bgo on 2019-07-19
        // (gzip for jpeg seems overkill)
        //tmp->SetHttpHeader("Accept-Encoding", "gzip");
        tmp->SetInstance(instance);
        tmp->SetQuality((quality == 0 ? 50 : 90));
        tmp->SetExpectedPixelFormat(slice.GetExpectedPixelFormat());
        command.reset(tmp.release());
      }

      command->AcquirePayload(new Orthanc::SingleValueObject<unsigned int>(sliceIndex));

      boost::shared_ptr<IObserver> observer(GetSharedObserver());
      oracle_.Schedule(observer, command.release());
    }
    else
    {
      // loading is finished!
      volumeImageReadyInHighQuality_ = true;
      BroadcastMessage(OrthancSeriesVolumeProgressiveLoader::VolumeImageReadyInHighQuality(*this));
    }
  }

/**
   This is called in response to GET "/series/XXXXXXXXXXXXX/instances-tags"
*/
  void OrthancSeriesVolumeProgressiveLoader::LoadGeometry(const OrthancRestApiCommand::SuccessMessage& message)
  {
    Json::Value body;
    message.ParseJsonBody(body);
      
    if (body.type() != Json::objectValue)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
    }

    {
      Json::Value::Members instances = body.getMemberNames();

      SlicesSorter slices;
        
      for (size_t i = 0; i < instances.size(); i++)
      {
        Orthanc::DicomMap dicom;
        dicom.FromDicomAsJson(body[instances[i]]);

        std::auto_ptr<DicomInstanceParameters> instance(new DicomInstanceParameters(dicom));
        instance->SetOrthancInstanceIdentifier(instances[i]);

        // the 3D plane corresponding to the slice
        CoordinateSystem3D geometry = instance->GetGeometry();
        slices.AddSlice(geometry, instance.release());
      }

      seriesGeometry_.ComputeGeometry(slices);
    }

    size_t slicesCount = seriesGeometry_.GetImageGeometry().GetDepth();

    if (slicesCount == 0)
    {
      volume_->Initialize(seriesGeometry_.GetImageGeometry(), Orthanc::PixelFormat_Grayscale8);
    }
    else
    {
      const DicomInstanceParameters& parameters = seriesGeometry_.GetSliceParameters(0);
        
      volume_->Initialize(seriesGeometry_.GetImageGeometry(), parameters.GetExpectedPixelFormat());
      volume_->SetDicomParameters(parameters);
      volume_->GetPixelData().Clear();

      strategy_.reset(new BasicFetchingStrategy(sorter_->CreateSorter(static_cast<unsigned int>(slicesCount)), BEST_QUALITY));
        
      assert(simultaneousDownloads_ != 0);
      for (unsigned int i = 0; i < simultaneousDownloads_; i++)
      {
        ScheduleNextSliceDownload();
      }
    }

    slicesQuality_.resize(slicesCount, 0);

    BroadcastMessage(DicomVolumeImage::GeometryReadyMessage(*volume_));
  }


  void OrthancSeriesVolumeProgressiveLoader::SetSliceContent(unsigned int sliceIndex,
                                                             const Orthanc::ImageAccessor& image,
                                                             unsigned int quality)
  {
    assert(sliceIndex < slicesQuality_.size() &&
           slicesQuality_.size() == volume_->GetPixelData().GetDepth());
      
    if (quality >= slicesQuality_[sliceIndex])
    {
      {
        ImageBuffer3D::SliceWriter writer(volume_->GetPixelData(), VolumeProjection_Axial, sliceIndex);
        Orthanc::ImageProcessing::Copy(writer.GetAccessor(), image);
      }

      volume_->IncrementRevision();
      seriesGeometry_.IncrementSliceRevision(sliceIndex);
      slicesQuality_[sliceIndex] = quality;

      BroadcastMessage(DicomVolumeImage::ContentUpdatedMessage(*volume_));
    }

    ScheduleNextSliceDownload();
  }


  void OrthancSeriesVolumeProgressiveLoader::LoadBestQualitySliceContent(const GetOrthancImageCommand::SuccessMessage& message)
  {
    SetSliceContent(GetSliceIndexPayload(message.GetOrigin()), message.GetImage(), BEST_QUALITY);
  }


  void OrthancSeriesVolumeProgressiveLoader::LoadJpegSliceContent(const GetOrthancWebViewerJpegCommand::SuccessMessage& message)
  {
    unsigned int quality;
      
    switch (message.GetOrigin().GetQuality())
    {
      case 50:
        quality = LOW_QUALITY;
        break;

      case 90:
        quality = MIDDLE_QUALITY;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
      
    SetSliceContent(GetSliceIndexPayload(message.GetOrigin()), message.GetImage(), quality);
  }


  OrthancSeriesVolumeProgressiveLoader::OrthancSeriesVolumeProgressiveLoader(const boost::shared_ptr<DicomVolumeImage>& volume,
                                                                             IOracle& oracle,
                                                                             IObservable& oracleObservable) :
    oracle_(oracle),
    active_(false),
    simultaneousDownloads_(4),
    volume_(volume),
    sorter_(new BasicFetchingItemsSorter::Factory),
    volumeImageReadyInHighQuality_(false)
  {
    // TODO => Move this out of constructor
    Register<OrthancRestApiCommand::SuccessMessage>
      (oracleObservable, &OrthancSeriesVolumeProgressiveLoader::LoadGeometry);

    Register<GetOrthancImageCommand::SuccessMessage>
      (oracleObservable, &OrthancSeriesVolumeProgressiveLoader::LoadBestQualitySliceContent);

    Register<GetOrthancWebViewerJpegCommand::SuccessMessage>
      (oracleObservable, &OrthancSeriesVolumeProgressiveLoader::LoadJpegSliceContent);
  }

  OrthancSeriesVolumeProgressiveLoader::~OrthancSeriesVolumeProgressiveLoader()
  {
    LOG(TRACE) << "OrthancSeriesVolumeProgressiveLoader::~OrthancSeriesVolumeProgressiveLoader()";
  }

  void OrthancSeriesVolumeProgressiveLoader::SetSimultaneousDownloads(unsigned int count)
  {
    if (active_)
    {
      LOG(ERROR) << "OrthancSeriesVolumeProgressiveLoader::SetSimultaneousDownloads(): (active_)";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else if (count == 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);        
    }
    else
    {
      simultaneousDownloads_ = count;
    }
  }


  void OrthancSeriesVolumeProgressiveLoader::LoadSeries(const std::string& seriesId)
  {
//    LOG(TRACE) << "OrthancSeriesVolumeProgressiveLoader::LoadSeries seriesId=" << seriesId;
    if (active_)
    {
//      LOG(TRACE) << "OrthancSeriesVolumeProgressiveLoader::LoadSeries NOT ACTIVE! --> ERROR";
      LOG(ERROR) << "OrthancSeriesVolumeProgressiveLoader::LoadSeries(const std::string& seriesId): (active_)";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      active_ = true;

      std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
      command->SetUri("/series/" + seriesId + "/instances-tags");

//      LOG(TRACE) << "OrthancSeriesVolumeProgressiveLoader::LoadSeries about to call oracle_.Schedule";
      boost::shared_ptr<IObserver> observer(GetSharedObserver());
      oracle_.Schedule(observer, command.release());
//      LOG(TRACE) << "OrthancSeriesVolumeProgressiveLoader::LoadSeries called oracle_.Schedule";
    }
  }
  

  IVolumeSlicer::IExtractedSlice* 
  OrthancSeriesVolumeProgressiveLoader::ExtractSlice(const CoordinateSystem3D& cuttingPlane)
  {
    if (volume_->HasGeometry())
    {
      return new ExtractedSlice(*this, cuttingPlane);
    }
    else
    {
      return new IVolumeSlicer::InvalidSlice;
    }
  }
}
