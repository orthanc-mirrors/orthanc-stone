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


#include "OrthancSlicesLoader.h"

#include "MessagingToolbox.h"

#include <Core/Endianness.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/JpegReader.h>
#include <Core/Images/PngReader.h>
#include <Core/Compression/GzipCompressor.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>

#include <boost/lexical_cast.hpp>



/**
 * TODO This is a SLOW implementation of base64 decoding, because
 * "Orthanc::Toolbox::DecodeBase64()" does not work properly with
 * WASM. UNDERSTAND WHY.
 * https://stackoverflow.com/a/34571089/881731
 **/
static std::string base64_decode(const std::string &in)
{
  std::string out;

  std::vector<int> T(256,-1);
  for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i; 

  int val=0, valb=-8;
  for (unsigned char c : in) {
    if (T[c] == -1) break;
    val = (val<<6) + T[c];
    valb += 6;
    if (valb>=0) {
      out.push_back(char((val>>valb)&0xFF));
      valb-=8;
    }
  }
  return out;
}



namespace OrthancStone
{
  class OrthancSlicesLoader::Operation : public Orthanc::IDynamicObject
  {
  private:
    Mode               mode_;
    unsigned int       frame_;
    unsigned int       sliceIndex_;
    const Slice*       slice_;
    std::string        instanceId_;
    SliceImageQuality  quality_;

    Operation(Mode mode) :
      mode_(mode)
    {
    }

  public:
    Mode GetMode() const
    {
      return mode_;
    }

    SliceImageQuality GetQuality() const
    {
      assert(mode_ == Mode_LoadImage ||
             mode_ == Mode_LoadRawImage);
      return quality_;
    }

    unsigned int GetSliceIndex() const
    {
      assert(mode_ == Mode_LoadImage ||
             mode_ == Mode_LoadRawImage);
      return sliceIndex_;
    }

    const Slice& GetSlice() const
    {
      assert(mode_ == Mode_LoadImage ||
             mode_ == Mode_LoadRawImage);
      assert(slice_ != NULL);
      return *slice_;
    }

    unsigned int GetFrame() const
    {
      assert(mode_ == Mode_FrameGeometry);
      return frame_;
    }
      
    const std::string& GetInstanceId() const
    {
      assert(mode_ == Mode_FrameGeometry ||
             mode_ == Mode_InstanceGeometry);
      return instanceId_;
    }
      
    static Operation* DownloadSeriesGeometry()
    {
      return new Operation(Mode_SeriesGeometry);
    }

    static Operation* DownloadInstanceGeometry(const std::string& instanceId)
    {
      std::auto_ptr<Operation> operation(new Operation(Mode_InstanceGeometry));
      operation->instanceId_ = instanceId;
      return operation.release();
    }

    static Operation* DownloadFrameGeometry(const std::string& instanceId,
                                            unsigned int frame)
    {
      std::auto_ptr<Operation> operation(new Operation(Mode_FrameGeometry));
      operation->instanceId_ = instanceId;
      operation->frame_ = frame;
      return operation.release();
    }

    static Operation* DownloadSliceImage(unsigned int  sliceIndex,
                                         const Slice&  slice,
                                         SliceImageQuality quality)
    {
      std::auto_ptr<Operation> tmp(new Operation(Mode_LoadImage));
      tmp->sliceIndex_ = sliceIndex;
      tmp->slice_ = &slice;
      tmp->quality_ = quality;
      return tmp.release();
    }

    static Operation* DownloadSliceRawImage(unsigned int  sliceIndex,
                                            const Slice&  slice)
    {
      std::auto_ptr<Operation> tmp(new Operation(Mode_LoadRawImage));
      tmp->sliceIndex_ = sliceIndex;
      tmp->slice_ = &slice;
      tmp->quality_ = SliceImageQuality_Full;
      return tmp.release();
    }
  };
    

  class OrthancSlicesLoader::WebCallback : public IWebService::ICallback
  {
  private:
    OrthancSlicesLoader&  that_;

  public:
    WebCallback(OrthancSlicesLoader&  that) :
      that_(that)
    {
    }

    virtual void NotifySuccess(const std::string& uri,
                               const void* answer,
                               size_t answerSize,
                               Orthanc::IDynamicObject* payload)
    {
      std::auto_ptr<Operation> operation(dynamic_cast<Operation*>(payload));

      switch (operation->GetMode())
      {
        case Mode_SeriesGeometry:
          that_.ParseSeriesGeometry(answer, answerSize);
          break;

        case Mode_InstanceGeometry:
          that_.ParseInstanceGeometry(operation->GetInstanceId(), answer, answerSize);
          break;

        case Mode_FrameGeometry:
          that_.ParseFrameGeometry(operation->GetInstanceId(),
                                   operation->GetFrame(), answer, answerSize);
          break;

        case Mode_LoadImage:
          switch (operation->GetQuality())
          {
            case SliceImageQuality_Full:
              that_.ParseSliceImagePng(*operation, answer, answerSize);
              break;

            case SliceImageQuality_Jpeg50:
            case SliceImageQuality_Jpeg90:
            case SliceImageQuality_Jpeg95:
              that_.ParseSliceImageJpeg(*operation, answer, answerSize);
              break;

            default:
              throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
          }
                
          break;

        case Mode_LoadRawImage:
          that_.ParseSliceRawImage(*operation, answer, answerSize);
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }

    virtual void NotifyError(const std::string& uri,
                             Orthanc::IDynamicObject* payload)
    {
      std::auto_ptr<Operation> operation(dynamic_cast<Operation*>(payload));
      LOG(ERROR) << "Cannot download " << uri;

      switch (operation->GetMode())
      {
        case Mode_FrameGeometry:
        case Mode_SeriesGeometry:
          that_.userCallback_.NotifyGeometryError(that_);
          that_.state_ = State_Error;
          break;

        case Mode_LoadImage:
          that_.userCallback_.NotifySliceImageError(that_, operation->GetSliceIndex(),
                                                    operation->GetSlice(),
                                                    operation->GetQuality());
          break;
          
        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }     
  };


  
  void OrthancSlicesLoader::NotifySliceImageSuccess(const Operation& operation,
                                                    std::auto_ptr<Orthanc::ImageAccessor>& image) const
  {
    if (image.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      userCallback_.NotifySliceImageReady
        (*this, operation.GetSliceIndex(), operation.GetSlice(), image, operation.GetQuality());
    }
  }

  
  void OrthancSlicesLoader::NotifySliceImageError(const Operation& operation) const
  {
    userCallback_.NotifySliceImageError
      (*this, operation.GetSliceIndex(), operation.GetSlice(), operation.GetQuality());
  }


  void OrthancSlicesLoader::SortAndFinalizeSlices()
  {
    bool ok = false;
    
    if (slices_.GetSliceCount() > 0)
    {
      Vector normal;
      if (slices_.SelectNormal(normal))
      {
        slices_.FilterNormal(normal);
        slices_.SetNormal(normal);
        slices_.Sort();
        ok = true;
      }
    }

    state_ = State_GeometryReady;

    if (ok)
    {
      LOG(INFO) << "Loaded a series with " << slices_.GetSliceCount() << " slice(s)";
      userCallback_.NotifyGeometryReady(*this);
    }
    else
    {
      LOG(ERROR) << "This series is empty";
      userCallback_.NotifyGeometryError(*this);
    }
  }

  
  void OrthancSlicesLoader::ParseSeriesGeometry(const void* answer,
                                                size_t size)
  {
    Json::Value series;
    if (!MessagingToolbox::ParseJson(series, answer, size) ||
        series.type() != Json::objectValue)
    {
      userCallback_.NotifyGeometryError(*this);
      return;
    }

    Json::Value::Members instances = series.getMemberNames();

    slices_.Reserve(instances.size());

    for (size_t i = 0; i < instances.size(); i++)
    {
      OrthancPlugins::FullOrthancDataset dataset(series[instances[i]]);

      Orthanc::DicomMap dicom;
      MessagingToolbox::ConvertDataset(dicom, dataset);
      
      unsigned int frames;
      if (!dicom.ParseUnsignedInteger32(frames, Orthanc::DICOM_TAG_NUMBER_OF_FRAMES))
      {
        frames = 1;
      }

      for (unsigned int frame = 0; frame < frames; frame++)
      {
        std::auto_ptr<Slice> slice(new Slice);
        if (slice->ParseOrthancFrame(dicom, instances[i], frame))
        {
          slices_.AddSlice(slice.release());
        }
        else
        {
          LOG(WARNING) << "Skipping invalid frame " << frame << " within instance " << instances[i];
        }
      }
    }

    SortAndFinalizeSlices();
  }


  void OrthancSlicesLoader::ParseInstanceGeometry(const std::string& instanceId,
                                                  const void* answer,
                                                  size_t size)
  {
    Json::Value tags;
    if (!MessagingToolbox::ParseJson(tags, answer, size) ||
        tags.type() != Json::objectValue)
    {
      userCallback_.NotifyGeometryError(*this);
      return;
    }

    OrthancPlugins::FullOrthancDataset dataset(tags);

    Orthanc::DicomMap dicom;
    MessagingToolbox::ConvertDataset(dicom, dataset);
      
    unsigned int frames;
    if (!dicom.ParseUnsignedInteger32(frames, Orthanc::DICOM_TAG_NUMBER_OF_FRAMES))
    {
      frames = 1;
    }
    
    LOG(INFO) << "Instance " << instanceId << " contains " << frames << " frame(s)";

    for (unsigned int frame = 0; frame < frames; frame++)
    {
      std::auto_ptr<Slice> slice(new Slice);
      if (slice->ParseOrthancFrame(dicom, instanceId, frame))
      {
        slices_.AddSlice(slice.release());
      }
      else
      {
        LOG(WARNING) << "Skipping invalid multi-frame instance " << instanceId;
        userCallback_.NotifyGeometryError(*this);
        return;
      }
    }

    SortAndFinalizeSlices();
  }


  void OrthancSlicesLoader::ParseFrameGeometry(const std::string& instanceId,
                                               unsigned int frame,
                                               const void* answer,
                                               size_t size)
  {
    Json::Value tags;
    if (!MessagingToolbox::ParseJson(tags, answer, size) ||
        tags.type() != Json::objectValue)
    {
      userCallback_.NotifyGeometryError(*this);
      return;
    }

    OrthancPlugins::FullOrthancDataset dataset(tags);

    state_ = State_GeometryReady;

    Orthanc::DicomMap dicom;
    MessagingToolbox::ConvertDataset(dicom, dataset);

    std::auto_ptr<Slice> slice(new Slice);
    if (slice->ParseOrthancFrame(dicom, instanceId, frame))
    {
      LOG(INFO) << "Loaded instance " << instanceId;
      slices_.AddSlice(slice.release());
      userCallback_.NotifyGeometryReady(*this);
    }
    else
    {
      LOG(WARNING) << "Skipping invalid instance " << instanceId;
      userCallback_.NotifyGeometryError(*this);
    }
  }


  void OrthancSlicesLoader::ParseSliceImagePng(const Operation& operation,
                                               const void* answer,
                                               size_t size)
  {
    std::auto_ptr<Orthanc::ImageAccessor>  image;

    try
    {
      image.reset(new Orthanc::PngReader);
      dynamic_cast<Orthanc::PngReader&>(*image).ReadFromMemory(answer, size);
    }
    catch (Orthanc::OrthancException&)
    {
      NotifySliceImageError(operation);
      return;
    }

    if (image->GetWidth() != operation.GetSlice().GetWidth() ||
        image->GetHeight() != operation.GetSlice().GetHeight())
    {
      NotifySliceImageError(operation);
      return;
    }
      
    if (operation.GetSlice().GetConverter().GetExpectedPixelFormat() ==
        Orthanc::PixelFormat_SignedGrayscale16)
    {
      if (image->GetFormat() == Orthanc::PixelFormat_Grayscale16)
      {
        image->SetFormat(Orthanc::PixelFormat_SignedGrayscale16);
      }
      else
      {
        NotifySliceImageError(operation);
        return;
      }
    }

    NotifySliceImageSuccess(operation, image);
  } 

  
  void OrthancSlicesLoader::ParseSliceImageJpeg(const Operation& operation,
                                                const void* answer,
                                                size_t size)
  {
    Json::Value encoded;
    if (!MessagingToolbox::ParseJson(encoded, answer, size) ||
        encoded.type() != Json::objectValue ||
        !encoded.isMember("Orthanc") ||
        encoded["Orthanc"].type() != Json::objectValue)
    {
      NotifySliceImageError(operation);
      return;
    }

    Json::Value& info = encoded["Orthanc"];
    if (!info.isMember("PixelData") ||
        !info.isMember("Stretched") ||
        !info.isMember("Compression") ||
        info["Compression"].type() != Json::stringValue ||
        info["PixelData"].type() != Json::stringValue ||
        info["Stretched"].type() != Json::booleanValue ||
        info["Compression"].asString() != "Jpeg")
    {
      NotifySliceImageError(operation);
      return;
    }

    bool isSigned = false;
    bool isStretched = info["Stretched"].asBool();

    if (info.isMember("IsSigned"))
    {
      if (info["IsSigned"].type() != Json::booleanValue)
      {
        NotifySliceImageError(operation);
        return;
      }          
      else
      {
        isSigned = info["IsSigned"].asBool();
      }
    }

    std::auto_ptr<Orthanc::ImageAccessor> reader;

    {
      std::string jpeg;
      //Orthanc::Toolbox::DecodeBase64(jpeg, info["PixelData"].asString());
      jpeg = base64_decode(info["PixelData"].asString());

      try
      {
        reader.reset(new Orthanc::JpegReader);
        dynamic_cast<Orthanc::JpegReader&>(*reader).ReadFromMemory(jpeg);
      }
      catch (Orthanc::OrthancException&)
      {
        NotifySliceImageError(operation);
        return;
      }
    }

    Orthanc::PixelFormat expectedFormat =
      operation.GetSlice().GetConverter().GetExpectedPixelFormat();

    if (reader->GetFormat() == Orthanc::PixelFormat_RGB24)  // This is a color image
    {
      if (expectedFormat != Orthanc::PixelFormat_RGB24)
      {
        NotifySliceImageError(operation);
        return;
      }

      if (isSigned || isStretched)
      {
        NotifySliceImageError(operation);
        return;
      }
      else
      {
        NotifySliceImageSuccess(operation, reader);
        return;
      }
    }

    if (reader->GetFormat() != Orthanc::PixelFormat_Grayscale8)
    {
      NotifySliceImageError(operation);
      return;
    }

    if (!isStretched)
    {
      if (expectedFormat != reader->GetFormat())
      {
        NotifySliceImageError(operation);
        return;
      }
      else
      {
        NotifySliceImageSuccess(operation, reader);
        return;
      }
    }

    int32_t stretchLow = 0;
    int32_t stretchHigh = 0;

    if (!info.isMember("StretchLow") ||
        !info.isMember("StretchHigh") ||
        info["StretchLow"].type() != Json::intValue ||
        info["StretchHigh"].type() != Json::intValue)
    {
      NotifySliceImageError(operation);
      return;
    }

    stretchLow = info["StretchLow"].asInt();
    stretchHigh = info["StretchHigh"].asInt();

    if (stretchLow < -32768 ||
        stretchHigh > 65535 ||
        (stretchLow < 0 && stretchHigh > 32767))
    {
      // This range cannot be represented with a uint16_t or an int16_t
      NotifySliceImageError(operation);
      return;
    }

    // Decode a grayscale JPEG 8bpp image coming from the Web viewer
    std::auto_ptr<Orthanc::ImageAccessor> image
      (new Orthanc::Image(expectedFormat, reader->GetWidth(), reader->GetHeight(), false));

    float scaling = static_cast<float>(stretchHigh - stretchLow) / 255.0f;
    float offset = static_cast<float>(stretchLow) / scaling;
      
    Orthanc::ImageProcessing::Convert(*image, *reader);
    reader.reset(NULL);
    
    Orthanc::ImageProcessing::ShiftScale(*image, offset, scaling);

    NotifySliceImageSuccess(operation, image);
  }


  class StringImage :
    public Orthanc::ImageAccessor,
    public boost::noncopyable
  {
  private:
    std::string  buffer_;
    
  public:
    StringImage(Orthanc::PixelFormat format,
                unsigned int width,
                unsigned int height,
                std::string& buffer)
    {
      if (buffer.size() != Orthanc::GetBytesPerPixel(format) * width * height)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }

      buffer_.swap(buffer);  // The source buffer is now empty

      void* data = (buffer_.empty() ? NULL : &buffer_[0]);

      AssignWritable(format, width, height,
                     Orthanc::GetBytesPerPixel(format) * width, data);
    }
  };

  
  void OrthancSlicesLoader::ParseSliceRawImage(const Operation& operation,
                                               const void* answer,
                                               size_t size)
  {
    Orthanc::GzipCompressor compressor;

    std::string raw;
    compressor.Uncompress(raw, answer, size);
    
    const Orthanc::DicomImageInformation& info = operation.GetSlice().GetImageInformation();
    
    if (info.GetBitsAllocated() == 32 &&
        info.GetBitsStored() == 32 &&
        info.GetHighBit() == 31 &&
        info.GetChannelCount() == 1 &&
        !info.IsSigned() &&
        info.GetPhotometricInterpretation() == Orthanc::PhotometricInterpretation_Monochrome2 &&
        raw.size() == info.GetWidth() * info.GetHeight() * 4)
    {
      // This is the case of RT-DOSE (uint32_t values)
      
      std::auto_ptr<Orthanc::ImageAccessor> image
        (new StringImage(Orthanc::PixelFormat_Grayscale32, info.GetWidth(),
                         info.GetHeight(), raw));

      // TODO - Only for big endian
      for (unsigned int y = 0; y < image->GetHeight(); y++)
      {
        uint32_t *p = reinterpret_cast<uint32_t*>(image->GetRow(y));
        for (unsigned int x = 0; x < image->GetWidth(); x++, p++)
        {
          *p = le32toh(*p);
        }
      }

      NotifySliceImageSuccess(operation, image);
    }
    else if (info.GetBitsAllocated() == 16 &&
             info.GetBitsStored() == 16 &&
             info.GetHighBit() == 15 &&
             info.GetChannelCount() == 1 &&
             !info.IsSigned() &&
             info.GetPhotometricInterpretation() == Orthanc::PhotometricInterpretation_Monochrome2 &&
             raw.size() == info.GetWidth() * info.GetHeight() * 2)
    {
      std::auto_ptr<Orthanc::ImageAccessor> image
        (new StringImage(Orthanc::PixelFormat_Grayscale16, info.GetWidth(),
                         info.GetHeight(), raw));

      // TODO - Big endian ?

      NotifySliceImageSuccess(operation, image);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
        
  }


  OrthancSlicesLoader::OrthancSlicesLoader(ICallback& callback,
                                           IWebService& orthanc) :
    webCallback_(new WebCallback(*this)),
    userCallback_(callback),
    orthanc_(orthanc),
    state_(State_Initialization)
  {
  }

  
  void OrthancSlicesLoader::ScheduleLoadSeries(const std::string& seriesId)
  {
    if (state_ != State_Initialization)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      state_ = State_LoadingGeometry;
      std::string uri = "/series/" + seriesId + "/instances-tags";
      orthanc_.ScheduleGetRequest(*webCallback_, uri, Operation::DownloadSeriesGeometry());
    }
  }


  void OrthancSlicesLoader::ScheduleLoadInstance(const std::string& instanceId)
  {
    if (state_ != State_Initialization)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      state_ = State_LoadingGeometry;

      // Tag "3004-000c" is "Grid Frame Offset Vector", which is
      // mandatory to read RT DOSE, but is too long to be returned by default
      std::string uri = "/instances/" + instanceId + "/tags?ignore-length=3004-000c";
      orthanc_.ScheduleGetRequest
        (*webCallback_, uri, Operation::DownloadInstanceGeometry(instanceId));
    }
  }
  

  void OrthancSlicesLoader::ScheduleLoadFrame(const std::string& instanceId,
                                              unsigned int frame)
  {
    if (state_ != State_Initialization)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      state_ = State_LoadingGeometry;
      std::string uri = "/instances/" + instanceId + "/tags";
      orthanc_.ScheduleGetRequest
        (*webCallback_, uri, Operation::DownloadFrameGeometry(instanceId, frame));
    }
  }
  

  bool OrthancSlicesLoader::IsGeometryReady() const
  {
    return state_ == State_GeometryReady;
  }


  size_t OrthancSlicesLoader::GetSliceCount() const
  {
    if (state_ != State_GeometryReady)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    return slices_.GetSliceCount();
  }

  
  const Slice& OrthancSlicesLoader::GetSlice(size_t index) const
  {
    if (state_ != State_GeometryReady)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    return slices_.GetSlice(index);
  }
  

  bool OrthancSlicesLoader::LookupSlice(size_t& index,
                                        const CoordinateSystem3D& plane) const
  {
    if (state_ != State_GeometryReady)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    return slices_.LookupSlice(index, plane);
  }
  

  void OrthancSlicesLoader::ScheduleSliceImagePng(const Slice& slice,
                                                  size_t index)
  {
    std::string uri = ("/instances/" + slice.GetOrthancInstanceId() + "/frames/" + 
                       boost::lexical_cast<std::string>(slice.GetFrame()));

    switch (slice.GetConverter().GetExpectedPixelFormat())
    {
      case Orthanc::PixelFormat_RGB24:
        uri += "/preview";
        break;

      case Orthanc::PixelFormat_Grayscale16:
        uri += "/image-uint16";
        break;

      case Orthanc::PixelFormat_SignedGrayscale16:
        uri += "/image-int16";
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    orthanc_.ScheduleGetRequest(*webCallback_, uri,
                                Operation::DownloadSliceImage(index, slice, SliceImageQuality_Full));
  }


  void OrthancSlicesLoader::ScheduleSliceImageJpeg(const Slice& slice,
                                                   size_t index,
                                                   SliceImageQuality quality)
  {
    unsigned int value;

    switch (quality)
    {
      case SliceImageQuality_Jpeg50:
        value = 50;
        break;
    
      case SliceImageQuality_Jpeg90:
        value = 90;
        break;
    
      case SliceImageQuality_Jpeg95:
        value = 95;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    
    // This requires the official Web viewer plugin to be installed!
    std::string uri = ("/web-viewer/instances/jpeg" + 
                       boost::lexical_cast<std::string>(value) + 
                       "-" + slice.GetOrthancInstanceId() + "_" + 
                       boost::lexical_cast<std::string>(slice.GetFrame()));

    orthanc_.ScheduleGetRequest(*webCallback_, uri,
                                Operation::DownloadSliceImage(index, slice, quality));
  }



  void OrthancSlicesLoader::ScheduleLoadSliceImage(size_t index,
                                                   SliceImageQuality quality)
  {
    if (state_ != State_GeometryReady)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    const Slice& slice = GetSlice(index);

    if (slice.HasOrthancDecoding())
    {
      if (quality == SliceImageQuality_Full)
      {
        ScheduleSliceImagePng(slice, index);
      }
      else
      {
        ScheduleSliceImageJpeg(slice, index, quality);
      }
    }
    else
    {
      std::string uri = ("/instances/" + slice.GetOrthancInstanceId() + "/frames/" + 
                         boost::lexical_cast<std::string>(slice.GetFrame()) + "/raw.gz");
      orthanc_.ScheduleGetRequest(*webCallback_, uri,
                                  Operation::DownloadSliceRawImage(index, slice));
    }
  }
}
