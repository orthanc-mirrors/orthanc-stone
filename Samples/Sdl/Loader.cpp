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


#include "../../Framework/Toolbox/DicomInstanceParameters.h"
#include "../../Framework/Oracle/ThreadedOracle.h"
#include "../../Framework/Oracle/GetOrthancWebViewerJpegCommand.h"
#include "../../Framework/Oracle/GetOrthancImageCommand.h"
#include "../../Framework/Oracle/OrthancRestApiCommand.h"
#include "../../Framework/Oracle/SleepOracleCommand.h"
#include "../../Framework/Oracle/OracleCommandExceptionMessage.h"
#include "../../Framework/Messages/IMessageEmitter.h"
#include "../../Framework/Oracle/OracleCommandWithPayload.h"
#include "../../Framework/Oracle/IOracle.h"

// From Stone
#include "../../Framework/Loaders/BasicFetchingItemsSorter.h"
#include "../../Framework/Loaders/BasicFetchingStrategy.h"
#include "../../Framework/Scene2D/Scene2D.h"
#include "../../Framework/StoneInitialization.h"
#include "../../Framework/Toolbox/GeometryToolbox.h"
#include "../../Framework/Toolbox/SlicesSorter.h"
#include "../../Framework/Volumes/ImageBuffer3D.h"
#include "../../Framework/Volumes/VolumeImageGeometry.h"

// From Orthanc framework
#include <Core/Compression/GzipCompressor.h>
#include <Core/Compression/ZlibCompressor.h>
#include <Core/DicomFormat/DicomArray.h>
#include <Core/DicomFormat/DicomImageInformation.h>
#include <Core/HttpClient.h>
#include <Core/IDynamicObject.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Logging.h>
#include <Core/MultiThreading/SharedMessageQueue.h>
#include <Core/OrthancException.h>
#include <Core/SystemToolbox.h>
#include <Core/Toolbox.h>

#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>

#include <list>
#include <stdio.h>



namespace OrthancStone
{
  class DicomVolumeImage : public boost::noncopyable
  {
  private:
    std::auto_ptr<ImageBuffer3D>           image_;
    std::auto_ptr<VolumeImageGeometry>     geometry_;
    std::vector<DicomInstanceParameters*>  slices_;
    uint64_t                               revision_;
    std::vector<uint64_t>                  slicesRevision_;
    std::vector<unsigned int>              slicesQuality_;

    void CheckSlice(size_t index,
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

    
    void CheckVolume() const
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


    void Clear()
    {
      image_.reset();
      geometry_.reset();
      
      for (size_t i = 0; i < slices_.size(); i++)
      {
        assert(slices_[i] != NULL);
        delete slices_[i];
      }

      slices_.clear();
      slicesRevision_.clear();
      slicesQuality_.clear();
    }


    void CheckSliceIndex(size_t index) const
    {
      assert(slices_.size() == image_->GetDepth() &&
             slices_.size() == slicesRevision_.size());

      if (!HasGeometry())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else if (index >= slices_.size())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

    
  public:
    DicomVolumeImage()
    {
    }

    ~DicomVolumeImage()
    {
      Clear();
    }

    // WARNING: The payload of "slices" must be of class "DicomInstanceParameters"
    void SetGeometry(SlicesSorter& slices)
    {
      Clear();
      
      if (!slices.Sort())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                        "Cannot sort the 3D slices of a DICOM series");          
      }

      geometry_.reset(new VolumeImageGeometry);

      if (slices.GetSlicesCount() == 0)
      {
        // Empty volume
        image_.reset(new ImageBuffer3D(Orthanc::PixelFormat_Grayscale8, 0, 0, 0,
                                       false /* don't compute range */));
      }
      else
      {
        slices_.reserve(slices.GetSlicesCount());
        slicesRevision_.resize(slices.GetSlicesCount(), 0);
        slicesQuality_.resize(slices.GetSlicesCount(), 0);

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

        image_.reset(new ImageBuffer3D(parameters.GetExpectedPixelFormat(),
                                       parameters.GetImageInformation().GetWidth(),
                                       parameters.GetImageInformation().GetHeight(),
                                       slices.GetSlicesCount(), false /* don't compute range */));      

        geometry_->SetSize(image_->GetWidth(), image_->GetHeight(), image_->GetDepth());
        geometry_->SetAxialGeometry(slices.GetSliceGeometry(0));
        geometry_->SetVoxelDimensions(parameters.GetPixelSpacingX(),
                                      parameters.GetPixelSpacingY(), spacingZ);
      }
      
      image_->Clear();

      revision_++;
    }

    uint64_t GetRevision() const
    {
      return revision_;
    }

    bool HasGeometry() const
    {
      return (image_.get() != NULL &&
              geometry_.get() != NULL);
    }

    const ImageBuffer3D& GetImage() const
    {
      if (!HasGeometry())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        return *image_;
      }
    }

    const VolumeImageGeometry& GetGeometry() const
    {
      if (!HasGeometry())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        return *geometry_;
      }
    }

    size_t GetSlicesCount() const
    {
      if (!HasGeometry())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        return slices_.size();
      }
    }

    const DicomInstanceParameters& GetSliceParameters(size_t index) const
    {
      CheckSliceIndex(index);
      return *slices_[index];
    }

    uint64_t GetSliceRevision(size_t index) const
    {
      CheckSliceIndex(index);
      return slicesRevision_[index];
    }

    void SetSliceContent(size_t index,
                         const Orthanc::ImageAccessor& image,
                         unsigned int quality)
    {
      CheckSliceIndex(index);

      // If a better image quality is already available, don't update the content
      if (quality >= slicesQuality_[index])
      {
        {
          ImageBuffer3D::SliceWriter writer
            (*image_, VolumeProjection_Axial, index);
          Orthanc::ImageProcessing::Copy(writer.GetAccessor(), image);
        }
        
        revision_ ++;
        slicesRevision_[index] += 1;
      }
    }
  };



  class IDicomVolumeImageSource : public boost::noncopyable
  {
  public:
    virtual ~IDicomVolumeImageSource()
    {
    }

    virtual const DicomVolumeImage& GetVolume() const = 0;

    virtual void NotifyAxialSliceAccessed(unsigned int sliceIndex) = 0;
  };
  
  

  class VolumeSeriesOrthancLoader :
    public IObserver,
    public IDicomVolumeImageSource
  {
  private:
    static const unsigned int LOW_QUALITY = 0;
    static const unsigned int MIDDLE_QUALITY = 1;
    static const unsigned int BEST_QUALITY = 2;
    
    
    static unsigned int GetSliceIndexPayload(const OracleCommandWithPayload& command)
    {
      return dynamic_cast< const Orthanc::SingleValueObject<unsigned int>& >(command.GetPayload()).GetValue();
    }


    void ScheduleNextSliceDownload()
    {
      assert(strategy_.get() != NULL);
      
      unsigned int sliceIndex, quality;
      
      if (strategy_->GetNext(sliceIndex, quality))
      {
        assert(quality <= BEST_QUALITY);

        const DicomInstanceParameters& slice = volume_.GetSliceParameters(sliceIndex);
          
        const std::string& instance = slice.GetOrthancInstanceIdentifier();
        if (instance.empty())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        std::auto_ptr<OracleCommandWithPayload> command;
        
        if (quality == BEST_QUALITY)
        {
          std::auto_ptr<GetOrthancImageCommand> tmp(new GetOrthancImageCommand);
          tmp->SetHttpHeader("Accept-Encoding", "gzip");
          tmp->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
          tmp->SetInstanceUri(instance, slice.GetExpectedPixelFormat());          
          tmp->SetExpectedPixelFormat(slice.GetExpectedPixelFormat());
          command.reset(tmp.release());
        }
        else
        {
          std::auto_ptr<GetOrthancWebViewerJpegCommand> tmp(new GetOrthancWebViewerJpegCommand);
          tmp->SetHttpHeader("Accept-Encoding", "gzip");
          tmp->SetInstance(instance);
          tmp->SetQuality((quality == 0 ? 50 : 90));
          tmp->SetExpectedPixelFormat(slice.GetExpectedPixelFormat());
          command.reset(tmp.release());
        }

        command->SetPayload(new Orthanc::SingleValueObject<unsigned int>(sliceIndex));
        oracle_.Schedule(*this, command.release());
      }
    }


    void LoadGeometry(const OrthancRestApiCommand::SuccessMessage& message)
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

          CoordinateSystem3D geometry = instance->GetGeometry();
          slices.AddSlice(geometry, instance.release());
        }

        volume_.SetGeometry(slices);
      }

      if (volume_.GetSlicesCount() != 0)
      {
        strategy_.reset(new BasicFetchingStrategy(
                          new BasicFetchingItemsSorter(volume_.GetSlicesCount()), BEST_QUALITY));

        for (unsigned int i = 0; i < 4; i++)   // Schedule up to 4 simultaneous downloads (TODO - parameter)
        {
          ScheduleNextSliceDownload();
        }
      }
    }


    void LoadBestQualitySliceContent(const GetOrthancImageCommand::SuccessMessage& message)
    {      
      volume_.SetSliceContent(GetSliceIndexPayload(message.GetOrigin()),
                              message.GetImage(), BEST_QUALITY);

      ScheduleNextSliceDownload();
    }


    void LoadJpegSliceContent(const GetOrthancWebViewerJpegCommand::SuccessMessage& message)
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
      
      volume_.SetSliceContent(GetSliceIndexPayload(message.GetOrigin()), message.GetImage(), quality);

      ScheduleNextSliceDownload();
    }


    IOracle&          oracle_;
    bool              active_;
    DicomVolumeImage  volume_;
    
    std::auto_ptr<IFetchingStrategy>   strategy_;

  public:
    VolumeSeriesOrthancLoader(IOracle& oracle,
                              IObservable& oracleObservable) :
      IObserver(oracleObservable.GetBroker()),
      oracle_(oracle),
      active_(false)
    {
      oracleObservable.RegisterObserverCallback(
        new Callable<VolumeSeriesOrthancLoader, OrthancRestApiCommand::SuccessMessage>
        (*this, &VolumeSeriesOrthancLoader::LoadGeometry));

      oracleObservable.RegisterObserverCallback(
        new Callable<VolumeSeriesOrthancLoader, GetOrthancImageCommand::SuccessMessage>
        (*this, &VolumeSeriesOrthancLoader::LoadBestQualitySliceContent));

      oracleObservable.RegisterObserverCallback(
        new Callable<VolumeSeriesOrthancLoader, GetOrthancWebViewerJpegCommand::SuccessMessage>
        (*this, &VolumeSeriesOrthancLoader::LoadJpegSliceContent));
    }

    void LoadSeries(const std::string& seriesId)
    {
      if (active_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      active_ = true;

      std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
      command->SetUri("/series/" + seriesId + "/instances-tags");

      oracle_.Schedule(*this, command.release());
    }
    

    virtual const DicomVolumeImage& GetVolume() const
    {
      return volume_;
    }

    
    virtual void NotifyAxialSliceAccessed(unsigned int sliceIndex)
    {
      if (strategy_.get() == NULL)
      {
        // Should have called GetVolume().HasGeometry() before
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        strategy_->SetCurrent(sliceIndex);
      }
    }
  };



#if 0
  void LoadInstance(const std::string& instanceId)
  {
    if (active_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    active_ = true;

    // Tag "3004-000c" is "Grid Frame Offset Vector", which is
    // mandatory to read RT DOSE, but is too long to be returned by default

    // TODO => Should be part of a second call if needed

    std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
    command->SetUri("/instances/" + instanceId + "/tags?ignore-length=3004-000c");
    command->SetPayload(new LoadInstanceGeometryHandler(*this));

    oracle_.Schedule(*this, command.release());
  }
#endif


  /*  class VolumeSlicerBase : public IVolumeSlicer
      {
      private:
      Scene2D&            scene_;
      int                               layerDepth_;
      bool                              first_;
      CoordinateSystem3D  lastPlane_;

      protected:
      bool HasViewportPlaneChanged(const CoordinateSystem3D& plane) const
      {
      if (first_ ||
      !LinearAlgebra::IsCloseToZero(
      boost::numeric::ublas::norm_2(lastPlane_.GetNormal() - plane.GetNormal())))
      {
      // This is the first rendering, or the plane has not the same orientation
      return false;
      }
      else
      {
      double offset1 = lastPlane_.ProjectAlongNormal(plane.GetOrigin());
      double offset2 = lastPlane_.ProjectAlongNormal(lastPlane_.GetOrigin());
      return LinearAlgebra::IsCloseToZero(offset2 - offset1);
      }
      }

      void SetLastViewportPlane(const CoordinateSystem3D& plane)
      {
      first_ = false;
      lastPlane_ = plane;
      }

      void SetLayer(ISceneLayer* layer)
      {
      scene_.SetLayer(layerDepth_, layer);
      }

      void DeleteLayer()
      {
      scene_.DeleteLayer(layerDepth_);
      }
    
      public:
      VolumeSlicerBase(Scene2D& scene,
      int layerDepth) :
      scene_(scene),
      layerDepth_(layerDepth),
      first_(true)
      {
      }
      };*/
  


  class IVolumeSlicer : public boost::noncopyable
  {
  public:
    virtual ~IVolumeSlicer()
    {
    }

    virtual void SetViewportPlane(const CoordinateSystem3D& plane) = 0;
  };


  class DicomVolumeMPRSlicer : public IVolumeSlicer
  {
  private:
    bool                      linearInterpolation_;
    Scene2D&                  scene_;
    int                       layerDepth_;
    IDicomVolumeImageSource&  source_;
    bool                      first_;
    VolumeProjection          lastProjection_;
    unsigned int              lastSliceIndex_;
    uint64_t                  lastSliceRevision_;

  public:
    DicomVolumeMPRSlicer(Scene2D& scene,
                         int layerDepth,
                         IDicomVolumeImageSource& source) :
      linearInterpolation_(false),
      scene_(scene),
      layerDepth_(layerDepth),
      source_(source),
      first_(true)
    {
    }

    void SetLinearInterpolation(bool enabled)
    {
      linearInterpolation_ = enabled;
    }

    bool IsLinearInterpolation() const
    {
      return linearInterpolation_;
    }
    
    virtual void SetViewportPlane(const CoordinateSystem3D& plane)
    {
      if (!source_.GetVolume().HasGeometry() ||
          source_.GetVolume().GetSlicesCount() == 0)
      {
        scene_.DeleteLayer(layerDepth_);
        return;
      }

      const VolumeImageGeometry& geometry = source_.GetVolume().GetGeometry();

      VolumeProjection projection;
      unsigned int sliceIndex;
      if (!geometry.DetectSlice(projection, sliceIndex, plane))
      {
        // The cutting plane is neither axial, nor coronal, nor
        // sagittal. Could use "VolumeReslicer" here.
        scene_.DeleteLayer(layerDepth_);
        return;
      }

      uint64_t sliceRevision;
      if (projection == VolumeProjection_Axial)
      {
        sliceRevision = source_.GetVolume().GetSliceRevision(sliceIndex);

        if (first_ ||
            lastSliceIndex_ != sliceIndex)
        {
          // Reorder the prefetching queue
          source_.NotifyAxialSliceAccessed(sliceIndex);
        }
      }
      else
      {
        // For coronal and sagittal projections, we take the global
        // revision of the volume
        sliceRevision = source_.GetVolume().GetRevision();
      }

      if (first_ ||
          lastProjection_ != projection ||
          lastSliceIndex_ != sliceIndex ||
          lastSliceRevision_ != sliceRevision)
      {
        // Either the viewport plane, or the content of the slice have not
        // changed since the last time the layer was set: Update is needed

        first_ = false;
        lastProjection_ = projection;
        lastSliceIndex_ = sliceIndex;
        lastSliceRevision_ = sliceRevision;

        std::auto_ptr<TextureBaseSceneLayer> texture;
        
        {
          const DicomInstanceParameters& parameters = source_.GetVolume().GetSliceParameters
            (projection == VolumeProjection_Axial ? sliceIndex : 0);

          ImageBuffer3D::SliceReader reader(source_.GetVolume().GetImage(), projection, sliceIndex);
          texture.reset(parameters.CreateTexture(reader.GetAccessor()));
        }

        const CoordinateSystem3D& system = geometry.GetProjectionGeometry(projection);

        double x0, y0, x1, y1;
        system.ProjectPoint(x0, y0, system.GetOrigin());
        system.ProjectPoint(x0, y0, system.GetOrigin() + system.GetAxisX());
        texture->SetOrigin(x0, y0);

        double dx = x1 - x0;
        double dy = y1 - y0;
        if (!LinearAlgebra::IsCloseToZero(dx) ||
            !LinearAlgebra::IsCloseToZero(dy))
        {
          texture->SetAngle(atan2(dy, dx));
        }
        
        Vector tmp;
        geometry.GetVoxelDimensions(projection);
        texture->SetPixelSpacing(tmp[0], tmp[1]);

        texture->SetLinearInterpolation(linearInterpolation_);
    
        scene_.SetLayer(layerDepth_, texture.release());    
      }
    }
  };


  class NativeApplicationContext : public IMessageEmitter
  {
  private:
    boost::shared_mutex            mutex_;
    MessageBroker    broker_;
    IObservable      oracleObservable_;

  public:
    NativeApplicationContext() :
      oracleObservable_(broker_)
    {
    }


    virtual void EmitMessage(const IObserver& observer,
                             const IMessage& message)
    {
      try
      {
        boost::unique_lock<boost::shared_mutex>  lock(mutex_);
        oracleObservable_.EmitMessage(observer, message);
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(ERROR) << "Exception while emitting a message: " << e.What();
      }
    }


    class ReaderLock : public boost::noncopyable
    {
    private:
      NativeApplicationContext&                that_;
      boost::shared_lock<boost::shared_mutex>  lock_;

    public:
      ReaderLock(NativeApplicationContext& that) : 
      that_(that),
      lock_(that.mutex_)
      {
      }
    };


    class WriterLock : public boost::noncopyable
    {
    private:
      NativeApplicationContext&                that_;
      boost::unique_lock<boost::shared_mutex>  lock_;

    public:
      WriterLock(NativeApplicationContext& that) : 
      that_(that),
      lock_(that.mutex_)
      {
      }

      MessageBroker& GetBroker() 
      {
        return that_.broker_;
      }

      IObservable& GetOracleObservable()
      {
        return that_.oracleObservable_;
      }
    };
  };
}



class Toto : public OrthancStone::IObserver
{
private:
  void Handle(const OrthancStone::SleepOracleCommand::TimeoutMessage& message)
  {
    printf("TIMEOUT! %d\n", dynamic_cast<const Orthanc::SingleValueObject<unsigned int>& >(message.GetOrigin().GetPayload()).GetValue());
  }

  void Handle(const OrthancStone::OrthancRestApiCommand::SuccessMessage& message)
  {
    Json::Value v;
    message.ParseJsonBody(v);

    printf("ICI [%s]\n", v.toStyledString().c_str());
  }

  void Handle(const OrthancStone::GetOrthancImageCommand::SuccessMessage& message)
  {
    printf("IMAGE %dx%d\n", message.GetImage().GetWidth(), message.GetImage().GetHeight());
  }

  void Handle(const OrthancStone::GetOrthancWebViewerJpegCommand::SuccessMessage& message)
  {
    printf("WebViewer %dx%d\n", message.GetImage().GetWidth(), message.GetImage().GetHeight());
  }

  void Handle(const OrthancStone::OracleCommandExceptionMessage& message)
  {
    printf("EXCEPTION: [%s] on command type %d\n", message.GetException().What(), message.GetCommand().GetType());

    switch (message.GetCommand().GetType())
    {
      case OrthancStone::IOracleCommand::Type_GetOrthancWebViewerJpeg:
        printf("URI: [%s]\n", dynamic_cast<const OrthancStone::GetOrthancWebViewerJpegCommand&>
               (message.GetCommand()).GetUri().c_str());
        break;
      
      default:
        break;
    }
  }

public:
  Toto(OrthancStone::IObservable& oracle) :
    IObserver(oracle.GetBroker())
  {
    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::SleepOracleCommand::TimeoutMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::OrthancRestApiCommand::SuccessMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::GetOrthancImageCommand::SuccessMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::GetOrthancWebViewerJpegCommand::SuccessMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::OracleCommandExceptionMessage>(*this, &Toto::Handle));
  }
};


void Run(OrthancStone::NativeApplicationContext& context,
         OrthancStone::IOracle& oracle)
{
  std::auto_ptr<Toto> toto;
  std::auto_ptr<OrthancStone::VolumeSeriesOrthancLoader> loader1, loader2;

  {
    OrthancStone::NativeApplicationContext::WriterLock lock(context);
    toto.reset(new Toto(lock.GetOracleObservable()));
    loader1.reset(new OrthancStone::VolumeSeriesOrthancLoader(oracle, lock.GetOracleObservable()));
    loader2.reset(new OrthancStone::VolumeSeriesOrthancLoader(oracle, lock.GetOracleObservable()));
  }

  if (0)
  {
    Json::Value v = Json::objectValue;
    v["Level"] = "Series";
    v["Query"] = Json::objectValue;

    std::auto_ptr<OrthancStone::OrthancRestApiCommand>  command(new OrthancStone::OrthancRestApiCommand);
    command->SetMethod(Orthanc::HttpMethod_Post);
    command->SetUri("/tools/find");
    command->SetBody(v);

    oracle.Schedule(*toto, command.release());
  }
  
  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Jpeg)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/preview");
    oracle.Schedule(*toto, command.release());
  }
  
  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Png)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/preview");
    oracle.Schedule(*toto, command.release());
  }
  
  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Png)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
    oracle.Schedule(*toto, command.release());
  }
  
  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
    command->SetHttpHeader("Accept-Encoding", "gzip");
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
    oracle.Schedule(*toto, command.release());
  }
  
  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
    oracle.Schedule(*toto, command.release());
  }

  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancWebViewerJpegCommand>  command(new OrthancStone::GetOrthancWebViewerJpegCommand);
    command->SetHttpHeader("Accept-Encoding", "gzip");
    command->SetInstance("e6c7c20b-c9f65d7e-0d76f2e2-830186f2-3e3c600e");
    command->SetQuality(90);
    oracle.Schedule(*toto, command.release());
  }


  if (0)
  {
    for (unsigned int i = 0; i < 10; i++)
    {
      std::auto_ptr<OrthancStone::SleepOracleCommand> command(new OrthancStone::SleepOracleCommand(i * 1000));
      command->SetPayload(new Orthanc::SingleValueObject<unsigned int>(42 * i));
      oracle.Schedule(*toto, command.release());
    }
  }

  // 2017-11-17-Anonymized
  //loader1->LoadSeries("cb3ea4d1-d08f3856-ad7b6314-74d88d77-60b05618");  // CT
  //loader2->LoadInstance("41029085-71718346-811efac4-420e2c15-d39f99b6");  // RT-DOSE

  // Delphine
  //loader1->LoadSeries("5990e39c-51e5f201-fe87a54c-31a55943-e59ef80e");  // CT
  loader1->LoadSeries("67f1b334-02c16752-45026e40-a5b60b6b-030ecab5");  // Lung 1/10mm

  LOG(WARNING) << "...Waiting for Ctrl-C...";
  Orthanc::SystemToolbox::ServerBarrier();
  //boost::this_thread::sleep(boost::posix_time::seconds(1));
}



/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  OrthancStone::StoneInitialize();
  Orthanc::Logging::EnableInfoLevel(true);

  try
  {
    OrthancStone::NativeApplicationContext context;

    OrthancStone::ThreadedOracle oracle(context);

    {
      Orthanc::WebServiceParameters p;
      //p.SetUrl("http://localhost:8043/");
      p.SetCredentials("orthanc", "orthanc");
      oracle.SetOrthancParameters(p);
    }

    oracle.Start();

    Run(context, oracle);

    oracle.Stop();
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  OrthancStone::StoneFinalize();

  return 0;
}
