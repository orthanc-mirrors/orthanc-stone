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

// From Stone
#include "../../Framework/Loaders/BasicFetchingItemsSorter.h"
#include "../../Framework/Loaders/BasicFetchingStrategy.h"
#include "../../Framework/Scene2D/CairoCompositor.h"
#include "../../Framework/Scene2D/Scene2D.h"
#include "../../Framework/Scene2D/PolylineSceneLayer.h"
#include "../../Framework/Scene2D/LookupTableTextureSceneLayer.h"
#include "../../Framework/StoneInitialization.h"
#include "../../Framework/Toolbox/GeometryToolbox.h"
#include "../../Framework/Toolbox/SlicesSorter.h"
#include "../../Framework/Toolbox/DicomStructureSet.h"
#include "../../Framework/Volumes/ImageBuffer3D.h"
#include "../../Framework/Volumes/VolumeImageGeometry.h"
#include "../../Framework/Volumes/VolumeReslicer.h"

// From Orthanc framework
#include <Core/DicomFormat/DicomArray.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>
#include <Core/Endianness.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/SystemToolbox.h>
#include <Core/Toolbox.h>


#include <EmbeddedResources.h>


namespace OrthancStone
{
  // Application-configurable, can be shared between 3D/2D
  class ILayerStyleConfigurator
  {
  public:
    virtual ~ILayerStyleConfigurator()
    {
    }
    
    virtual uint64_t GetRevision() const = 0;
    
    virtual TextureBaseSceneLayer* CreateTextureFromImage(const Orthanc::ImageAccessor& image) const = 0;

    virtual TextureBaseSceneLayer* CreateTextureFromDicom(const Orthanc::ImageAccessor& frame,
                                                          const DicomInstanceParameters& parameters) const = 0;

    virtual void ApplyStyle(ISceneLayer& layer) const = 0;
  };



  class LookupTableStyleConfigurator : public ILayerStyleConfigurator
  {
  private:
    uint64_t     revision_;
    bool         hasLut_;
    std::string  lut_;
    bool         hasRange_;
    float        minValue_;
    float        maxValue_;
    
  public:
    LookupTableStyleConfigurator() :
      revision_(0),
      hasLut_(false),
      hasRange_(false)
    {
    }

    void SetLookupTable(Orthanc::EmbeddedResources::FileResourceId resource)
    {
      hasLut_ = true;
      Orthanc::EmbeddedResources::GetFileResource(lut_, resource);
    }

    void SetLookupTable(const std::string& lut)
    {
      hasLut_ = true;
      lut_ = lut;
    }

    void SetRange(float minValue,
                  float maxValue)
    {
      if (minValue > maxValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        hasRange_ = true;
        minValue_ = minValue;
        maxValue_ = maxValue;
      }
    }

    virtual uint64_t GetRevision() const
    {
      return revision_;
    }
    
    virtual TextureBaseSceneLayer* CreateTextureFromImage(const Orthanc::ImageAccessor& image) const
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    virtual TextureBaseSceneLayer* CreateTextureFromDicom(const Orthanc::ImageAccessor& frame,
                                                          const DicomInstanceParameters& parameters) const
    {
      return parameters.CreateLookupTableTexture(frame);
    }

    virtual void ApplyStyle(ISceneLayer& layer) const
    {
      LookupTableTextureSceneLayer& l = dynamic_cast<LookupTableTextureSceneLayer&>(layer);
      
      if (hasLut_)
      {
        l.SetLookupTable(lut_);
      }

      if (hasRange_)
      {
        l.SetRange(minValue_, maxValue_);
      }
      else
      {
        l.FitRange();
      }
    }
  };


  class GrayscaleStyleConfigurator : public ILayerStyleConfigurator
  {
  private:
    uint64_t revision_;
    
  public:
    GrayscaleStyleConfigurator() :
      revision_(0)
    {
    }

    virtual uint64_t GetRevision() const
    {
      return revision_;
    }
    
    virtual TextureBaseSceneLayer* CreateTextureFromImage(const Orthanc::ImageAccessor& image) const
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    virtual TextureBaseSceneLayer* CreateTextureFromDicom(const Orthanc::ImageAccessor& frame,
                                                          const DicomInstanceParameters& parameters) const
    {
      return parameters.CreateTexture(frame);
    }

    virtual void ApplyStyle(ISceneLayer& layer) const
    {
    }
  };


  class IVolumeSlicer : public boost::noncopyable
  {
  public:
    class IExtractedSlice : public boost::noncopyable
    {
    public:
      virtual ~IExtractedSlice()
      {
      }

      virtual bool IsValid() = 0;

      // Must be a cheap call
      virtual uint64_t GetRevision() = 0;

      // This call can take some time
      virtual ISceneLayer* CreateSceneLayer(const ILayerStyleConfigurator* configurator,  // possibly absent
                                            const CoordinateSystem3D& cuttingPlane) = 0;
    };

    
    class InvalidSlice : public IExtractedSlice
    {
    public:
      virtual bool IsValid()
      {
        return false;
      }

      virtual uint64_t GetRevision()
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      virtual ISceneLayer* CreateSceneLayer(const ILayerStyleConfigurator* configurator,
                                            const CoordinateSystem3D& cuttingPlane)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    };


    virtual ~IVolumeSlicer()
    {
    }

    virtual IExtractedSlice* ExtractSlice(const CoordinateSystem3D& cuttingPlane) = 0;
  };



  // This class combines a 3D image buffer, a 3D volume geometry and
  // information about the DICOM parameters of the series.
  class DicomVolumeImage : public boost::noncopyable
  {
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, GeometryReadyMessage, DicomVolumeImage);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, ContentUpdatedMessage, DicomVolumeImage);

  private:
    uint64_t                                revision_;
    std::auto_ptr<VolumeImageGeometry>      geometry_;
    std::auto_ptr<ImageBuffer3D>            image_;
    std::auto_ptr<DicomInstanceParameters>  parameters_;

    void CheckHasGeometry() const
    {
      if (!HasGeometry())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }
    
  public:
    DicomVolumeImage() :
      revision_(0)
    {
    }

    void IncrementRevision()
    {
      revision_ ++;
    }

    void Initialize(const VolumeImageGeometry& geometry,
                    Orthanc::PixelFormat format)
    {
      geometry_.reset(new VolumeImageGeometry(geometry));
      image_.reset(new ImageBuffer3D(format, geometry_->GetWidth(), geometry_->GetHeight(),
                                     geometry_->GetDepth(), false /* don't compute range */));

      revision_ ++;
    }

    void SetDicomParameters(const DicomInstanceParameters& parameters)
    {
      parameters_.reset(parameters.Clone());
      revision_ ++;
    }
    
    uint64_t GetRevision() const
    {
      return revision_;
    }

    bool HasGeometry() const
    {
      return (geometry_.get() != NULL &&
              image_.get() != NULL);
    }

    ImageBuffer3D& GetPixelData()
    {
      CheckHasGeometry();
      return *image_;
    }

    const ImageBuffer3D& GetPixelData() const
    {
      CheckHasGeometry();
      return *image_;
    }

    const VolumeImageGeometry& GetGeometry() const
    {
      CheckHasGeometry();
      return *geometry_;
    }

    bool HasDicomParameters() const
    {
      return parameters_.get() != NULL;
    }      

    const DicomInstanceParameters& GetDicomParameters() const
    {
      if (HasDicomParameters())
      {
        return *parameters_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }      
    }
  };



  class DicomVolumeImageMPRSlicer : public IVolumeSlicer
  {
  public:
    class Slice : public IExtractedSlice
    {
    private:
      const DicomVolumeImage&  volume_;
      bool                 valid_;
      VolumeProjection     projection_;
      unsigned int         sliceIndex_;

      void CheckValid() const
      {
        if (!valid_)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
        }
      }

    protected:
      // Can be overloaded in subclasses
      virtual uint64_t GetRevisionInternal(VolumeProjection projection,
                                           unsigned int sliceIndex) const
      {
        return volume_.GetRevision();
      }

    public:
      Slice(const DicomVolumeImage& volume,
            const CoordinateSystem3D& cuttingPlane) :
        volume_(volume)
      {
        valid_ = (volume_.HasDicomParameters() &&
                  volume_.GetGeometry().DetectSlice(projection_, sliceIndex_, cuttingPlane));
      }

      VolumeProjection GetProjection() const
      {
        CheckValid();
        return projection_;
      }

      unsigned int GetSliceIndex() const
      {
        CheckValid();
        return sliceIndex_;
      }

      virtual bool IsValid()
      {
        return valid_;
      }

      virtual uint64_t GetRevision()
      {
        CheckValid();
        return GetRevisionInternal(projection_, sliceIndex_);
      }

      virtual ISceneLayer* CreateSceneLayer(const ILayerStyleConfigurator* configurator,
                                            const CoordinateSystem3D& cuttingPlane)
      {
        CheckValid();

        if (configurator == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer,
                                          "A style configurator is mandatory for textures");
        }

        std::auto_ptr<TextureBaseSceneLayer> texture;
        
        {
          const DicomInstanceParameters& parameters = volume_.GetDicomParameters();
          ImageBuffer3D::SliceReader reader(volume_.GetPixelData(), projection_, sliceIndex_);
          texture.reset(dynamic_cast<TextureBaseSceneLayer*>
                        (configurator->CreateTextureFromDicom(reader.GetAccessor(), parameters)));
        }

        const CoordinateSystem3D& system = volume_.GetGeometry().GetProjectionGeometry(projection_);
      
        double x0, y0, x1, y1;
        cuttingPlane.ProjectPoint(x0, y0, system.GetOrigin());
        cuttingPlane.ProjectPoint(x1, y1, system.GetOrigin() + system.GetAxisX());
        texture->SetOrigin(x0, y0);

        double dx = x1 - x0;
        double dy = y1 - y0;
        if (!LinearAlgebra::IsCloseToZero(dx) ||
            !LinearAlgebra::IsCloseToZero(dy))
        {
          texture->SetAngle(atan2(dy, dx));
        }
        
        Vector tmp = volume_.GetGeometry().GetVoxelDimensions(projection_);
        texture->SetPixelSpacing(tmp[0], tmp[1]);

        return texture.release();

#if 0
        double w = texture->GetTexture().GetWidth() * tmp[0];
        double h = texture->GetTexture().GetHeight() * tmp[1];
        printf("%.1f %.1f %.1f => %.1f %.1f => %.1f %.1f\n",
               system.GetOrigin() [0],
               system.GetOrigin() [1],
               system.GetOrigin() [2],
               x0, y0, x0 + w, y0 + h);

        std::auto_ptr<PolylineSceneLayer> toto(new PolylineSceneLayer);

        PolylineSceneLayer::Chain c;
        c.push_back(ScenePoint2D(x0, y0));
        c.push_back(ScenePoint2D(x0 + w, y0));
        c.push_back(ScenePoint2D(x0 + w, y0 + h));
        c.push_back(ScenePoint2D(x0, y0 + h));
      
        toto->AddChain(c, true);

        return toto.release();
#endif
      }
    };

  private:
    boost::shared_ptr<DicomVolumeImage>  volume_;

  public:
    DicomVolumeImageMPRSlicer(const boost::shared_ptr<DicomVolumeImage>& volume) :
      volume_(volume)
    {
    }

    virtual IExtractedSlice* ExtractSlice(const CoordinateSystem3D& cuttingPlane)
    {
      if (volume_->HasGeometry())
      {
        return new Slice(*volume_, cuttingPlane);
      }
      else
      {
        return new IVolumeSlicer::InvalidSlice;
      }
    }
  };

  



  class OrthancSeriesVolumeProgressiveLoader : 
    public IObserver,
    public IObservable,
    public IVolumeSlicer
  {
  private:
    static const unsigned int LOW_QUALITY = 0;
    static const unsigned int MIDDLE_QUALITY = 1;
    static const unsigned int BEST_QUALITY = 2;
    
    
    // Helper class internal to OrthancSeriesVolumeProgressiveLoader
    class SeriesGeometry : public boost::noncopyable
    {
    private:
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
        for (size_t i = 0; i < slices_.size(); i++)
        {
          assert(slices_[i] != NULL);
          delete slices_[i];
        }

        slices_.clear();
        slicesRevision_.clear();
      }


      void CheckSliceIndex(size_t index) const
      {
        if (!HasGeometry())
        {
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

    
      std::auto_ptr<VolumeImageGeometry>     geometry_;
      std::vector<DicomInstanceParameters*>  slices_;
      std::vector<uint64_t>                  slicesRevision_;

    public:
      ~SeriesGeometry()
      {
        Clear();
      }

      // WARNING: The payload of "slices" must be of class "DicomInstanceParameters"
      void ComputeGeometry(SlicesSorter& slices)
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
          geometry_->SetSize(parameters.GetImageInformation().GetWidth(),
                             parameters.GetImageInformation().GetHeight(),
                             static_cast<unsigned int>(slices.GetSlicesCount()));
          geometry_->SetAxialGeometry(slices.GetSliceGeometry(0));
          geometry_->SetVoxelDimensions(parameters.GetPixelSpacingX(),
                                        parameters.GetPixelSpacingY(), spacingZ);
        }
      }

      bool HasGeometry() const
      {
        return geometry_.get() != NULL;
      }

      const VolumeImageGeometry& GetImageGeometry() const
      {
        if (!HasGeometry())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
        }
        else
        {
          assert(slices_.size() == geometry_->GetDepth());
          return *geometry_;
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

      void IncrementSliceRevision(size_t index)
      {
        CheckSliceIndex(index);
        slicesRevision_[index] ++;
      }
    };


    class ExtractedSlice : public DicomVolumeImageMPRSlicer::Slice
    {
    private:
      const OrthancSeriesVolumeProgressiveLoader&  that_;

    protected:
      virtual uint64_t GetRevisionInternal(VolumeProjection projection,
                                           unsigned int sliceIndex) const
      {
        if (projection == VolumeProjection_Axial)
        {
          return that_.seriesGeometry_.GetSliceRevision(sliceIndex);
        }
        else
        {
          // For coronal and sagittal projections, we take the global
          // revision of the volume
          return that_.volume_->GetRevision();
        }
      }

    public:
      ExtractedSlice(const OrthancSeriesVolumeProgressiveLoader& that,
                     const CoordinateSystem3D& plane) :
        DicomVolumeImageMPRSlicer::Slice(*that.volume_, plane),
        that_(that)
      {
        if (that_.strategy_.get() != NULL &&
            IsValid() &&
            GetProjection() == VolumeProjection_Axial)
        {
          that_.strategy_->SetCurrent(GetSliceIndex());
        }
      }
    };

    
    
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

        const DicomInstanceParameters& slice = seriesGeometry_.GetSliceParameters(sliceIndex);
          
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

        strategy_.reset(new BasicFetchingStrategy(sorter_->CreateSorter(slicesCount), BEST_QUALITY));
        
        assert(simultaneousDownloads_ != 0);
        for (unsigned int i = 0; i < simultaneousDownloads_; i++)
        {
          ScheduleNextSliceDownload();
        }
      }

      slicesQuality_.resize(slicesCount, 0);

      BroadcastMessage(DicomVolumeImage::GeometryReadyMessage(*volume_));
    }


    void SetSliceContent(unsigned int sliceIndex,
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


    void LoadBestQualitySliceContent(const GetOrthancImageCommand::SuccessMessage& message)
    {
      SetSliceContent(GetSliceIndexPayload(message.GetOrigin()), message.GetImage(), BEST_QUALITY);
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
      
      SetSliceContent(GetSliceIndexPayload(message.GetOrigin()), message.GetImage(), quality);
    }


    IOracle&        oracle_;
    bool            active_;
    unsigned int    simultaneousDownloads_;
    SeriesGeometry  seriesGeometry_;
    
    boost::shared_ptr<DicomVolumeImage>      volume_;
    std::auto_ptr<IFetchingItemsSorter::IFactory>  sorter_;
    std::auto_ptr<IFetchingStrategy>               strategy_;
    std::vector<unsigned int>   slicesQuality_;


  public:
    OrthancSeriesVolumeProgressiveLoader(const boost::shared_ptr<DicomVolumeImage>& volume,
                                         IOracle& oracle,
                                         IObservable& oracleObservable) :
      IObserver(oracleObservable.GetBroker()),
      IObservable(oracleObservable.GetBroker()),
      oracle_(oracle),
      active_(false),
      simultaneousDownloads_(4),
      volume_(volume),
      sorter_(new BasicFetchingItemsSorter::Factory)
    {
      oracleObservable.RegisterObserverCallback(
        new Callable<OrthancSeriesVolumeProgressiveLoader, OrthancRestApiCommand::SuccessMessage>
        (*this, &OrthancSeriesVolumeProgressiveLoader::LoadGeometry));

      oracleObservable.RegisterObserverCallback(
        new Callable<OrthancSeriesVolumeProgressiveLoader, GetOrthancImageCommand::SuccessMessage>
        (*this, &OrthancSeriesVolumeProgressiveLoader::LoadBestQualitySliceContent));

      oracleObservable.RegisterObserverCallback(
        new Callable<OrthancSeriesVolumeProgressiveLoader, GetOrthancWebViewerJpegCommand::SuccessMessage>
        (*this, &OrthancSeriesVolumeProgressiveLoader::LoadJpegSliceContent));
    }

    void SetSimultaneousDownloads(unsigned int count)
    {
      if (active_)
      {
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

    void LoadSeries(const std::string& seriesId)
    {
      if (active_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        active_ = true;

        std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
        command->SetUri("/series/" + seriesId + "/instances-tags");

        oracle_.Schedule(*this, command.release());
      }
    }


    virtual IExtractedSlice* ExtractSlice(const CoordinateSystem3D& cuttingPlane)
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
  };



  class LoaderStateMachine : public IObserver
  {
  protected:
    class State : public Orthanc::IDynamicObject
    {
    private:
      LoaderStateMachine&  that_;

    public:
      State(LoaderStateMachine& that) :
        that_(that)
      {
      }

      void Schedule(OracleCommandWithPayload* command)
      {
        that_.Schedule(command);
      }
      
      virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message) const
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
      
      virtual void Handle(const GetOrthancImageCommand::SuccessMessage& message) const
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
      
      virtual void Handle(const GetOrthancWebViewerJpegCommand::SuccessMessage& message) const
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    };

    void Schedule(OracleCommandWithPayload* command)
    {
      std::auto_ptr<OracleCommandWithPayload> protection(command);
      
      if (!command->HasPayload())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                        "The payload must contain the next state");
      }

      pendingCommands_.push_back(protection.release());
    }

    void Start()
    {
      if (active_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      for (size_t i = 0; i < simultaneousDownloads_; i++)
      {
        Step();
      }
    }

  private:
    void Step()
    {
      if (!pendingCommands_.empty())
      {
        oracle_.Schedule(*this, pendingCommands_.front());
        pendingCommands_.pop_front();
      }
    }
    
    void Handle(const OrthancRestApiCommand::SuccessMessage& message)
    {
      dynamic_cast<const State&>(message.GetOrigin().GetPayload()).Handle(message);
      Step();
    }

    void Handle(const GetOrthancImageCommand::SuccessMessage& message)
    {
      dynamic_cast<const State&>(message.GetOrigin().GetPayload()).Handle(message);
      Step();
    }

    void Handle(const GetOrthancWebViewerJpegCommand::SuccessMessage& message)
    {
      dynamic_cast<const State&>(message.GetOrigin().GetPayload()).Handle(message);
      Step();
    }

    typedef std::list<IOracleCommand*>  PendingCommands;

    IOracle&         oracle_;
    bool             active_;
    unsigned int     simultaneousDownloads_;
    PendingCommands  pendingCommands_;

  public:
    LoaderStateMachine(IOracle& oracle,
                       IObservable& oracleObservable) :
      IObserver(oracleObservable.GetBroker()),
      oracle_(oracle),
      active_(false),
      simultaneousDownloads_(4)
    {
      oracleObservable.RegisterObserverCallback(
        new Callable<LoaderStateMachine, OrthancRestApiCommand::SuccessMessage>
        (*this, &LoaderStateMachine::Handle));

      oracleObservable.RegisterObserverCallback(
        new Callable<LoaderStateMachine, GetOrthancImageCommand::SuccessMessage>
        (*this, &LoaderStateMachine::Handle));

      oracleObservable.RegisterObserverCallback(
        new Callable<LoaderStateMachine, GetOrthancWebViewerJpegCommand::SuccessMessage>
        (*this, &LoaderStateMachine::Handle));
    }

    virtual ~LoaderStateMachine()
    {
      for (PendingCommands::iterator it = pendingCommands_.begin();
           it != pendingCommands_.end(); ++it)
      {
        delete *it;
      }
    }

    virtual void SetSimultaneousDownloads(unsigned int count)
    {
      if (active_)
      {
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
  };



  class OrthancMultiframeVolumeLoader :
    public IObserver,
    public IObservable
  {
  private:
    class State : public Orthanc::IDynamicObject
    {
    private:
      OrthancMultiframeVolumeLoader&  that_;

    protected:
      void Schedule(OrthancRestApiCommand* command) const
      {
        that_.oracle_.Schedule(that_, command);
      }

      OrthancMultiframeVolumeLoader& GetTarget() const
      {
        return that_;
      }

    public:
      State(OrthancMultiframeVolumeLoader& that) :
        that_(that)
      {
      }
      
      virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message) const = 0;
    };
    
    void Handle(const OrthancRestApiCommand::SuccessMessage& message)
    {
      dynamic_cast<const State&>(message.GetOrigin().GetPayload()).Handle(message);
    }


    class LoadRTDoseGeometry : public State
    {
    private:
      std::auto_ptr<Orthanc::DicomMap>  dicom_;

    public:
      LoadRTDoseGeometry(OrthancMultiframeVolumeLoader& that,
                         Orthanc::DicomMap* dicom) :
        State(that),
        dicom_(dicom)
      {
        if (dicom == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
      }

      virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message) const
      {
        // Complete the DICOM tags with just-received "Grid Frame Offset Vector"
        std::string s = Orthanc::Toolbox::StripSpaces(message.GetAnswer());
        dicom_->SetValue(Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR, s, false);

        GetTarget().SetGeometry(*dicom_);
      }      
    };


    static std::string GetSopClassUid(const Orthanc::DicomMap& dicom)
    {
      std::string s;
      if (!dicom.CopyToString(s, Orthanc::DICOM_TAG_SOP_CLASS_UID, false))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                        "DICOM file without SOP class UID");
      }
      else
      {
        return s;
      }
    }
    

    class LoadGeometry : public State
    {
    public:
      LoadGeometry(OrthancMultiframeVolumeLoader& that) :
        State(that)
      {
      }
      
      virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message) const
      {
        Json::Value body;
        message.ParseJsonBody(body);
        
        if (body.type() != Json::objectValue)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
        }

        std::auto_ptr<Orthanc::DicomMap> dicom(new Orthanc::DicomMap);
        dicom->FromDicomAsJson(body);

        if (StringToSopClassUid(GetSopClassUid(*dicom)) == SopClassUid_RTDose)
        {
          // Download the "Grid Frame Offset Vector" DICOM tag, that is
          // mandatory for RT-DOSE, but is too long to be returned by default
          
          std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
          command->SetUri("/instances/" + GetTarget().GetInstanceId() + "/content/" +
                          Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR.Format());
          command->SetPayload(new LoadRTDoseGeometry(GetTarget(), dicom.release()));

          Schedule(command.release());
        }
        else
        {
          GetTarget().SetGeometry(*dicom);
        }
      }
    };



    class LoadTransferSyntax : public State
    {
    public:
      LoadTransferSyntax(OrthancMultiframeVolumeLoader& that) :
        State(that)
      {
      }
      
      virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message) const
      {
        GetTarget().SetTransferSyntax(message.GetAnswer());
      }
    };
   
    
    class LoadUncompressedPixelData : public State
    {
    public:
      LoadUncompressedPixelData(OrthancMultiframeVolumeLoader& that) :
        State(that)
      {
      }
      
      virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message) const
      {
        GetTarget().SetUncompressedPixelData(message.GetAnswer());
      }
    };
   
    

    boost::shared_ptr<DicomVolumeImage>   volume_;
    IOracle&     oracle_;
    bool         active_;
    std::string  instanceId_;
    std::string  transferSyntaxUid_;


    const std::string& GetInstanceId() const
    {
      if (active_)
      {
        return instanceId_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }


    void ScheduleFrameDownloads()
    {
      if (transferSyntaxUid_.empty() ||
          !volume_->HasGeometry())
      {
        return;
      }
      
      if (transferSyntaxUid_ == "1.2.840.10008.1.2" ||
          transferSyntaxUid_ == "1.2.840.10008.1.2.1" ||
          transferSyntaxUid_ == "1.2.840.10008.1.2.2")
      {
        std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
        command->SetHttpHeader("Accept-Encoding", "gzip");
        command->SetUri("/instances/" + instanceId_ + "/content/" +
                        Orthanc::DICOM_TAG_PIXEL_DATA.Format() + "/0");
        command->SetPayload(new LoadUncompressedPixelData(*this));
        oracle_.Schedule(*this, command.release());
      }
      else
      {
        throw Orthanc::OrthancException(
          Orthanc::ErrorCode_NotImplemented,
          "No support for multiframe instances with transfer syntax: " + transferSyntaxUid_);
      }
    }
      

    void SetTransferSyntax(const std::string& transferSyntax)
    {
      transferSyntaxUid_ = Orthanc::Toolbox::StripSpaces(transferSyntax);
      ScheduleFrameDownloads();
    }
    

    void SetGeometry(const Orthanc::DicomMap& dicom)
    {
      DicomInstanceParameters parameters(dicom);
      volume_->SetDicomParameters(parameters);
      
      Orthanc::PixelFormat format;
      if (!parameters.GetImageInformation().ExtractPixelFormat(format, true))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }

      double spacingZ;
      switch (parameters.GetSopClassUid())
      {
        case SopClassUid_RTDose:
          spacingZ = parameters.GetThickness();
          break;

        default:
          throw Orthanc::OrthancException(
            Orthanc::ErrorCode_NotImplemented,
            "No support for multiframe instances with SOP class UID: " + GetSopClassUid(dicom));
      }

      const unsigned int width = parameters.GetImageInformation().GetWidth();
      const unsigned int height = parameters.GetImageInformation().GetHeight();
      const unsigned int depth = parameters.GetImageInformation().GetNumberOfFrames();

      {
        VolumeImageGeometry geometry;
        geometry.SetSize(width, height, depth);
        geometry.SetAxialGeometry(parameters.GetGeometry());
        geometry.SetVoxelDimensions(parameters.GetPixelSpacingX(),
                                    parameters.GetPixelSpacingY(), spacingZ);
        volume_->Initialize(geometry, format);
      }

      volume_->GetPixelData().Clear();

      ScheduleFrameDownloads();

      BroadcastMessage(DicomVolumeImage::GeometryReadyMessage(*volume_));
    }


    ORTHANC_FORCE_INLINE
    static void CopyPixel(uint32_t& target,
                          const void* source)
    {
      // TODO - check alignement?
      target = le32toh(*reinterpret_cast<const uint32_t*>(source));
    }
      

    template <typename T>
    void CopyPixelData(const std::string& pixelData)
    {
      ImageBuffer3D& target = volume_->GetPixelData();
      
      const unsigned int bpp = target.GetBytesPerPixel();
      const unsigned int width = target.GetWidth();
      const unsigned int height = target.GetHeight();
      const unsigned int depth = target.GetDepth();

      if (pixelData.size() != bpp * width * height * depth)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                        "The pixel data has not the proper size");
      }

      if (pixelData.empty())
      {
        return;
      }

      const uint8_t* source = reinterpret_cast<const uint8_t*>(pixelData.c_str());

      for (unsigned int z = 0; z < depth; z++)
      {
        ImageBuffer3D::SliceWriter writer(target, VolumeProjection_Axial, z);

        assert (writer.GetAccessor().GetWidth() == width &&
                writer.GetAccessor().GetHeight() == height);

        for (unsigned int y = 0; y < height; y++)
        {
          assert(sizeof(T) == Orthanc::GetBytesPerPixel(target.GetFormat()));

          T* target = reinterpret_cast<T*>(writer.GetAccessor().GetRow(y));

          for (unsigned int x = 0; x < width; x++)
          {
            CopyPixel(*target, source);
            
            target ++;
            source += bpp;
          }
        }
      }
    }
    

    void SetUncompressedPixelData(const std::string& pixelData)
    {
      switch (volume_->GetPixelData().GetFormat())
      {
        case Orthanc::PixelFormat_Grayscale32:
          CopyPixelData<uint32_t>(pixelData);
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }

      volume_->IncrementRevision();

      BroadcastMessage(DicomVolumeImage::ContentUpdatedMessage(*volume_));
    }


  public:
    OrthancMultiframeVolumeLoader(const boost::shared_ptr<DicomVolumeImage>& volume,
                                  IOracle& oracle,
                                  IObservable& oracleObservable) :
      IObserver(oracleObservable.GetBroker()),
      IObservable(oracleObservable.GetBroker()),
      volume_(volume),
      oracle_(oracle),
      active_(false)
    {
      if (volume.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
      
      oracleObservable.RegisterObserverCallback(
        new Callable<OrthancMultiframeVolumeLoader, OrthancRestApiCommand::SuccessMessage>
        (*this, &OrthancMultiframeVolumeLoader::Handle));
    }


    void LoadInstance(const std::string& instanceId)
    {
      if (active_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        active_ = true;
        instanceId_ = instanceId;

        {
          std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
          command->SetHttpHeader("Accept-Encoding", "gzip");
          command->SetUri("/instances/" + instanceId + "/tags");
          command->SetPayload(new LoadGeometry(*this));
          oracle_.Schedule(*this, command.release());
        }

        {
          std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
          command->SetUri("/instances/" + instanceId + "/metadata/TransferSyntax");
          command->SetPayload(new LoadTransferSyntax(*this));
          oracle_.Schedule(*this, command.release());
        }
      }
    }
  };



  class VolumeImageReslicer : public IVolumeSlicer
  {
  private:
    class Slice : public IExtractedSlice
    {
    private:
      VolumeImageReslicer&  that_;
      CoordinateSystem3D    cuttingPlane_;
      
    public:
      Slice(VolumeImageReslicer& that,
            const CoordinateSystem3D& cuttingPlane) :
        that_(that),
        cuttingPlane_(cuttingPlane)
      {
      }
      
      virtual bool IsValid()
      {
        return true;
      }

      virtual uint64_t GetRevision()
      {
        return that_.volume_->GetRevision();
      }

      virtual ISceneLayer* CreateSceneLayer(const ILayerStyleConfigurator* configurator,  // possibly absent
                                            const CoordinateSystem3D& cuttingPlane)
      {
        VolumeReslicer& reslicer = that_.reslicer_;
        
        if (configurator == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                          "Must provide a layer style configurator");
        }
        
        reslicer.SetOutputFormat(that_.volume_->GetPixelData().GetFormat());
        reslicer.Apply(that_.volume_->GetPixelData(),
                       that_.volume_->GetGeometry(),
                       cuttingPlane);

        if (reslicer.IsSuccess())
        {
          std::auto_ptr<TextureBaseSceneLayer> layer
            (configurator->CreateTextureFromDicom(reslicer.GetOutputSlice(),
                                                  that_.volume_->GetDicomParameters()));
          if (layer.get() == NULL)
          {
            return NULL;
          }

          double s = reslicer.GetPixelSpacing();
          layer->SetPixelSpacing(s, s);
          layer->SetOrigin(reslicer.GetOutputExtent().GetX1() + 0.5 * s,
                           reslicer.GetOutputExtent().GetY1() + 0.5 * s);

          // TODO - Angle!!
                           
          return layer.release();
        }
        else
        {
          return NULL;
        }          
      }
    };
    
    boost::shared_ptr<DicomVolumeImage>  volume_;
    VolumeReslicer                       reslicer_;

  public:
    VolumeImageReslicer(const boost::shared_ptr<DicomVolumeImage>& volume) :
      volume_(volume)
    {
      if (volume.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    ImageInterpolation GetInterpolation() const
    {
      return reslicer_.GetInterpolation();
    }

    void SetInterpolation(ImageInterpolation interpolation)
    {
      reslicer_.SetInterpolation(interpolation);
    }

    bool IsFastMode() const
    {
      return reslicer_.IsFastMode();
    }

    void SetFastMode(bool fast)
    {
      reslicer_.EnableFastMode(fast);
    }
    
    virtual IExtractedSlice* ExtractSlice(const CoordinateSystem3D& cuttingPlane)
    {
      if (volume_->HasGeometry())
      {
        return new Slice(*this, cuttingPlane);
      }
      else
      {
        return new IVolumeSlicer::InvalidSlice;
      }
    }
  };



  class DicomStructureSetLoader :
    public IObserver,
    public IVolumeSlicer
  {
  private:
    std::auto_ptr<DicomStructureSet>  content_;
    IOracle&                          oracle_;
    bool                              active_;
    uint64_t                          revision_;
    std::string                       instanceId_;

    void Handle(const OrthancRestApiCommand::SuccessMessage& message)
    {
      assert(active_);

      {
        OrthancPlugins::FullOrthancDataset dicom(message.GetAnswer());
        content_.reset(new DicomStructureSet(dicom));
      }

      std::set<std::string> instances;
      content_->GetReferencedInstances(instances);

      for (std::set<std::string>::const_iterator
             it = instances.begin(); it != instances.end(); ++it)
      {
        printf("[%s]\n", it->c_str());
      }
    }

    
    class Slice : public IExtractedSlice
    {
    private:
      const DicomStructureSet&  content_;
      uint64_t                  revision_;
      bool                      isValid_;
      
    public:
      Slice(const DicomStructureSet& content,
            uint64_t revision,
            const CoordinateSystem3D& cuttingPlane) :
        content_(content),
        revision_(revision)
      {
        bool opposite;

        const Vector normal = content.GetNormal();
        isValid_ = (
          GeometryToolbox::IsParallelOrOpposite(opposite, normal, cuttingPlane.GetNormal()) ||
          GeometryToolbox::IsParallelOrOpposite(opposite, normal, cuttingPlane.GetAxisX()) ||
          GeometryToolbox::IsParallelOrOpposite(opposite, normal, cuttingPlane.GetAxisY()));
      }
      
      virtual bool IsValid()
      {
        return isValid_;
      }

      virtual uint64_t GetRevision()
      {
        return revision_;
      }

      virtual ISceneLayer* CreateSceneLayer(const ILayerStyleConfigurator* configurator,
                                            const CoordinateSystem3D& cuttingPlane)
      {
        assert(isValid_);

        std::auto_ptr<PolylineSceneLayer> layer(new PolylineSceneLayer);

        for (size_t i = 0; i < content_.GetStructuresCount(); i++)
        {
          std::vector< std::vector<DicomStructureSet::PolygonPoint> > polygons;
          
          if (content_.ProjectStructure(polygons, i, cuttingPlane))
          {
            printf(">> %d\n", polygons.size());
            
            for (size_t j = 0; j < polygons.size(); j++)
            {
              PolylineSceneLayer::Chain chain;
              chain.resize(polygons[j].size());
            
              for (size_t k = 0; k < polygons[i].size(); k++)
              {
                chain[k] = ScenePoint2D(polygons[j][k].first, polygons[j][k].second);
              }

              layer->AddChain(chain, true /* closed */);
            }
          }
        }

        printf("OK\n");

        return layer.release();
      }
    };
    
  public:
    DicomStructureSetLoader(IOracle& oracle,
                            IObservable& oracleObservable) :
      IObserver(oracleObservable.GetBroker()),
      oracle_(oracle),
      active_(false),
      revision_(0)
    {
      oracleObservable.RegisterObserverCallback(
        new Callable<DicomStructureSetLoader, OrthancRestApiCommand::SuccessMessage>
        (*this, &DicomStructureSetLoader::Handle));
    }
    
    
    void LoadInstance(const std::string& instanceId)
    {
      if (active_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        active_ = true;
        instanceId_ = instanceId;

        {
          std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
          command->SetHttpHeader("Accept-Encoding", "gzip");
          command->SetUri("/instances/" + instanceId + "/tags?ignore-length=3006-0050");
          oracle_.Schedule(*this, command.release());
        }
      }
    }

    virtual IExtractedSlice* ExtractSlice(const CoordinateSystem3D& cuttingPlane)
    {
      if (content_.get() == NULL)
      {
        // Geometry is not available yet
        return new IVolumeSlicer::InvalidSlice;
      }
      else
      {
        return new Slice(*content_, revision_, cuttingPlane);
      }
    }
  };



  class VolumeSceneLayerSource : public boost::noncopyable
  {
  private:
    Scene2D&                                scene_;
    int                                     layerDepth_;
    boost::shared_ptr<IVolumeSlicer>        slicer_;
    std::auto_ptr<ILayerStyleConfigurator>  configurator_;
    std::auto_ptr<CoordinateSystem3D>       lastPlane_;
    uint64_t                                lastRevision_;
    uint64_t                                lastConfiguratorRevision_;

    static bool IsSameCuttingPlane(const CoordinateSystem3D& a,
                                   const CoordinateSystem3D& b)
    {
      // TODO - What if the normal is reversed?
      double distance;
      return (CoordinateSystem3D::ComputeDistance(distance, a, b) &&
              LinearAlgebra::IsCloseToZero(distance));
    }

    void ClearLayer()
    {
      scene_.DeleteLayer(layerDepth_);
      lastPlane_.reset(NULL);
    }

  public:
    VolumeSceneLayerSource(Scene2D& scene,
                           int layerDepth,
                           const boost::shared_ptr<IVolumeSlicer>& slicer) :
      scene_(scene),
      layerDepth_(layerDepth),
      slicer_(slicer)
    {
      if (slicer == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    const IVolumeSlicer& GetSlicer() const
    {
      return *slicer_;
    }

    void RemoveConfigurator()
    {
      configurator_.reset();
      lastPlane_.reset();
    }

    void SetConfigurator(ILayerStyleConfigurator* configurator)  // Takes ownership
    {
      if (configurator == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      configurator_.reset(configurator);

      // Invalidate the layer
      lastPlane_.reset(NULL);
    }

    bool HasConfigurator() const
    {
      return configurator_.get() != NULL;
    }

    ILayerStyleConfigurator& GetConfigurator() const
    {
      if (configurator_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      
      return *configurator_;
    }

    void Update(const CoordinateSystem3D& plane)
    {
      assert(slicer_.get() != NULL);
      std::auto_ptr<IVolumeSlicer::IExtractedSlice> slice(slicer_->ExtractSlice(plane));

      if (slice.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);        
      }

      if (!slice->IsValid())
      {
        // The slicer cannot handle this cutting plane: Clear the layer
        ClearLayer();
      }
      else if (lastPlane_.get() != NULL &&
               IsSameCuttingPlane(*lastPlane_, plane) &&
               lastRevision_ == slice->GetRevision())
      {
        // The content of the slice has not changed: Don't update the
        // layer content, but possibly update its style

        if (configurator_.get() != NULL &&
            configurator_->GetRevision() != lastConfiguratorRevision_ &&
            scene_.HasLayer(layerDepth_))
        {
          configurator_->ApplyStyle(scene_.GetLayer(layerDepth_));
        }
      }
      else
      {
        // Content has changed: An update is needed
        lastPlane_.reset(new CoordinateSystem3D(plane));
        lastRevision_ = slice->GetRevision();

        std::auto_ptr<ISceneLayer> layer(slice->CreateSceneLayer(configurator_.get(), plane));
        if (layer.get() == NULL)
        {
          ClearLayer();
        }
        else
        {
          if (configurator_.get() != NULL)
          {
            lastConfiguratorRevision_ = configurator_->GetRevision();
            configurator_->ApplyStyle(*layer);
          }

          scene_.SetLayer(layerDepth_, layer.release());
        }
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
  OrthancStone::CoordinateSystem3D  plane_;
  OrthancStone::IOracle&            oracle_;
  OrthancStone::Scene2D             scene_;
  std::auto_ptr<OrthancStone::VolumeSceneLayerSource>  source1_, source2_, source3_;


  void Refresh()
  {
    if (source1_.get() != NULL)
    {
      source1_->Update(plane_);
    }
      
    if (source2_.get() != NULL)
    {
      source2_->Update(plane_);
    }

    if (source3_.get() != NULL)
    {
      source3_->Update(plane_);
    }

    scene_.FitContent(1024, 768);
      
    {
      OrthancStone::CairoCompositor compositor(scene_, 1024, 768);
      compositor.Refresh();
        
      Orthanc::ImageAccessor accessor;
      compositor.GetCanvas().GetReadOnlyAccessor(accessor);

      Orthanc::Image tmp(Orthanc::PixelFormat_RGB24, accessor.GetWidth(), accessor.GetHeight(), false);
      Orthanc::ImageProcessing::Convert(tmp, accessor);
        
      static unsigned int count = 0;
      char buf[64];
      sprintf(buf, "scene-%06d.png", count++);
        
      Orthanc::PngWriter writer;
      writer.WriteToFile(buf, tmp);
    }
  }

  
  void Handle(const OrthancStone::DicomVolumeImage::GeometryReadyMessage& message)
  {
    printf("Geometry ready\n");
    
    //plane_ = message.GetOrigin().GetGeometry().GetSagittalGeometry();
    plane_ = message.GetOrigin().GetGeometry().GetAxialGeometry();
    //plane_ = message.GetOrigin().GetGeometry().GetCoronalGeometry();
    plane_.SetOrigin(message.GetOrigin().GetGeometry().GetCoordinates(0.5f, 0.5f, 0.5f));

    Refresh();
  }
  
  
  void Handle(const OrthancStone::SleepOracleCommand::TimeoutMessage& message)
  {
    if (message.GetOrigin().HasPayload())
    {
      printf("TIMEOUT! %d\n", dynamic_cast<const Orthanc::SingleValueObject<unsigned int>& >(message.GetOrigin().GetPayload()).GetValue());
    }
    else
    {
      printf("TIMEOUT\n");

      Refresh();

      /**
       * The sleep() leads to a crash if the oracle is still running,
       * while this object is destroyed. Always stop the oracle before
       * destroying active objects.  (*)
       **/
      // boost::this_thread::sleep(boost::posix_time::seconds(2));

      oracle_.Schedule(*this, new OrthancStone::SleepOracleCommand(message.GetOrigin().GetDelay()));
    }
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
  Toto(OrthancStone::IOracle& oracle,
       OrthancStone::IObservable& oracleObservable) :
    IObserver(oracleObservable.GetBroker()),
    oracle_(oracle)
  {
    oracleObservable.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::SleepOracleCommand::TimeoutMessage>(*this, &Toto::Handle));

    oracleObservable.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::OrthancRestApiCommand::SuccessMessage>(*this, &Toto::Handle));

    oracleObservable.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::GetOrthancImageCommand::SuccessMessage>(*this, &Toto::Handle));

    oracleObservable.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::GetOrthancWebViewerJpegCommand::SuccessMessage>(*this, &Toto::Handle));

    oracleObservable.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::OracleCommandExceptionMessage>(*this, &Toto::Handle));
  }

  void SetReferenceLoader(OrthancStone::IObservable& loader)
  {
    loader.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::DicomVolumeImage::GeometryReadyMessage>(*this, &Toto::Handle));
  }

  void SetVolume1(int depth,
                  const boost::shared_ptr<OrthancStone::IVolumeSlicer>& volume,
                  OrthancStone::ILayerStyleConfigurator* style)
  {
    source1_.reset(new OrthancStone::VolumeSceneLayerSource(scene_, depth, volume));

    if (style != NULL)
    {
      source1_->SetConfigurator(style);
    }
  }

  void SetVolume2(int depth,
                  const boost::shared_ptr<OrthancStone::IVolumeSlicer>& volume,
                  OrthancStone::ILayerStyleConfigurator* style)
  {
    source2_.reset(new OrthancStone::VolumeSceneLayerSource(scene_, depth, volume));

    if (style != NULL)
    {
      source2_->SetConfigurator(style);
    }
  }

  void SetStructureSet(int depth,
                       const boost::shared_ptr<OrthancStone::DicomStructureSetLoader>& volume)
  {
    source3_.reset(new OrthancStone::VolumeSceneLayerSource(scene_, depth, volume));
  }
                       
};


void Run(OrthancStone::NativeApplicationContext& context,
         OrthancStone::ThreadedOracle& oracle)
{
  boost::shared_ptr<OrthancStone::DicomVolumeImage>  ct(new OrthancStone::DicomVolumeImage);
  boost::shared_ptr<OrthancStone::DicomVolumeImage>  dose(new OrthancStone::DicomVolumeImage);

  
  boost::shared_ptr<Toto> toto;
  boost::shared_ptr<OrthancStone::OrthancSeriesVolumeProgressiveLoader> ctLoader;
  boost::shared_ptr<OrthancStone::OrthancMultiframeVolumeLoader> doseLoader;
  boost::shared_ptr<OrthancStone::DicomStructureSetLoader>  rtstructLoader;

  {
    OrthancStone::NativeApplicationContext::WriterLock lock(context);
    toto.reset(new Toto(oracle, lock.GetOracleObservable()));
    ctLoader.reset(new OrthancStone::OrthancSeriesVolumeProgressiveLoader(ct, oracle, lock.GetOracleObservable()));
    doseLoader.reset(new OrthancStone::OrthancMultiframeVolumeLoader(dose, oracle, lock.GetOracleObservable()));
    rtstructLoader.reset(new OrthancStone::DicomStructureSetLoader(oracle, lock.GetOracleObservable()));
  }


  //toto->SetReferenceLoader(*ctLoader);
  toto->SetReferenceLoader(*doseLoader);


#if 1
  toto->SetVolume1(0, ctLoader, new OrthancStone::GrayscaleStyleConfigurator);
#else
  {
    boost::shared_ptr<OrthancStone::IVolumeSlicer> reslicer(new OrthancStone::VolumeImageReslicer(ct));
    toto->SetVolume1(0, reslicer, new OrthancStone::GrayscaleStyleConfigurator);
  }
#endif  
  

  {
    std::auto_ptr<OrthancStone::LookupTableStyleConfigurator> config(new OrthancStone::LookupTableStyleConfigurator);
    config->SetLookupTable(Orthanc::EmbeddedResources::COLORMAP_HOT);

    boost::shared_ptr<OrthancStone::DicomVolumeImageMPRSlicer> tmp(new OrthancStone::DicomVolumeImageMPRSlicer(dose));
    toto->SetVolume2(1, tmp, config.release());
  }

  toto->SetStructureSet(2, rtstructLoader);
  
  oracle.Schedule(*toto, new OrthancStone::SleepOracleCommand(100));

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
  //ctLoader->LoadSeries("cb3ea4d1-d08f3856-ad7b6314-74d88d77-60b05618");  // CT
  doseLoader->LoadInstance("41029085-71718346-811efac4-420e2c15-d39f99b6");  // RT-DOSE
  rtstructLoader->LoadInstance("83d9c0c3-913a7fee-610097d7-cbf0522d-fd75bee6");  // RT-STRUCT

  // 2015-01-28-Multiframe
  //doseLoader->LoadInstance("88f71e2a-5fad1c61-96ed14d6-5b3d3cf7-a5825279");  // Multiframe CT
  
  // Delphine
  //ctLoader->LoadSeries("5990e39c-51e5f201-fe87a54c-31a55943-e59ef80e");  // CT
  //ctLoader->LoadSeries("67f1b334-02c16752-45026e40-a5b60b6b-030ecab5");  // Lung 1/10mm


  {
    LOG(WARNING) << "...Waiting for Ctrl-C...";

    oracle.Start();

    Orthanc::SystemToolbox::ServerBarrier();

    /**
     * WARNING => The oracle must be stopped BEFORE the objects using
     * it are destroyed!!! This forces to wait for the completion of
     * the running callback methods. Otherwise, the callbacks methods
     * might still be running while their parent object is destroyed,
     * resulting in crashes. This is very visible if adding a sleep(),
     * as in (*).
     **/

    oracle.Stop();
  }
}



/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  OrthancStone::StoneInitialize();
  //Orthanc::Logging::EnableInfoLevel(true);

  try
  {
    OrthancStone::NativeApplicationContext context;

    OrthancStone::ThreadedOracle oracle(context);
    //oracle.SetThreadsCount(1);

    {
      Orthanc::WebServiceParameters p;
      //p.SetUrl("http://localhost:8043/");
      p.SetCredentials("orthanc", "orthanc");
      oracle.SetOrthancParameters(p);
    }

    //oracle.Start();

    Run(context, oracle);
    
    //oracle.Stop();
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  OrthancStone::StoneFinalize();

  return 0;
}
