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

#include "../../Resources/Orthanc/Core/Images/PngReader.h"
#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/DicomDatasetReader.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/FullOrthancDataset.h"

#include <boost/lexical_cast.hpp>

namespace OrthancStone
{
  class OrthancSlicesLoader::Operation : public Orthanc::IDynamicObject
  {
  private:
    Mode          mode_;
    unsigned int  frame_;
    unsigned int  sliceIndex_;
    const Slice*  slice_;
    std::string   instanceId_;

    Operation(Mode mode) :
      mode_(mode)
    {
    }

  public:
    Mode GetMode() const
    {
      return mode_;
    }

    unsigned int GetSliceIndex() const
    {
      assert(mode_ == Mode_LoadImage);
      return sliceIndex_;
    }

    const Slice& GetSlice() const
    {
      assert(mode_ == Mode_LoadImage && slice_ != NULL);
      return *slice_;
    }

    unsigned int GetFrame() const
    {
      assert(mode_ == Mode_InstanceGeometry);
      return frame_;
    }
      
    const std::string& GetInstanceId() const
    {
      assert(mode_ == Mode_InstanceGeometry);
      return instanceId_;
    }
      
    static Operation* DownloadSeriesGeometry()
    {
      return new Operation(Mode_SeriesGeometry);
    }

    static Operation* DownloadInstanceGeometry(const std::string& instanceId,
                                               unsigned int frame)
    {
      std::auto_ptr<Operation> operation(new Operation(Mode_InstanceGeometry));
      operation->instanceId_ = instanceId;
      operation->frame_ = frame;
      return operation.release();
    }

    static Operation* DownloadSliceImage(unsigned int  sliceIndex,
                                         const Slice&  slice)
    {
      std::auto_ptr<Operation> tmp(new Operation(Mode_LoadImage));
      tmp->sliceIndex_ = sliceIndex;
      tmp->slice_ = &slice;
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
          that_.ParseInstanceGeometry(operation->GetInstanceId(),
                                      operation->GetFrame(), answer, answerSize);
          break;

        case Mode_LoadImage:
          that_.ParseSliceImage(*operation, answer, answerSize);
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
        case Mode_SeriesGeometry:
          that_.userCallback_.NotifyGeometryError(that_);
          that_.state_ = State_Error;
          break;

        case Mode_LoadImage:
          that_.userCallback_.NotifySliceImageError(that_, operation->GetSliceIndex(),
                                                    operation->GetSlice());
          break;
          
        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }     
  };

  
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

      Slice slice;
      if (slice.ParseOrthancFrame(dataset, instances[i], 0 /* todo */))
      {
        slices_.AddSlice(slice);
      }
      else
      {
        LOG(WARNING) << "Skipping invalid instance " << instances[i];
      }
    }

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


  void OrthancSlicesLoader::ParseInstanceGeometry(const std::string& instanceId,
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
      
    Slice slice;
    if (slice.ParseOrthancFrame(dataset, instanceId, frame))
    {
      LOG(INFO) << "Loaded instance " << instanceId;
      slices_.AddSlice(slice);
      userCallback_.NotifyGeometryReady(*this);
    }
    else
    {
      LOG(WARNING) << "Skipping invalid instance " << instanceId;
      userCallback_.NotifyGeometryError(*this);
    }
  }


  void OrthancSlicesLoader::ParseSliceImage(const Operation& operation,
                                            const void* answer,
                                            size_t size)
  {
    std::auto_ptr<Orthanc::PngReader>  image(new Orthanc::PngReader);
    image->ReadFromMemory(answer, size);

    bool ok = (image->GetWidth() == operation.GetSlice().GetWidth() ||
               image->GetHeight() == operation.GetSlice().GetHeight());
      
    if (ok &&
        operation.GetSlice().GetConverter().GetExpectedPixelFormat() ==
        Orthanc::PixelFormat_SignedGrayscale16)
    {
      if (image->GetFormat() == Orthanc::PixelFormat_Grayscale16)
      {
        image->SetFormat(Orthanc::PixelFormat_SignedGrayscale16);
      }
      else
      {
        ok = false;
      }
    }

    if (ok)
    {
      userCallback_.NotifySliceImageReady(*this, operation.GetSliceIndex(),
                                          operation.GetSlice(), image.release());
    }
    else
    {
      userCallback_.NotifySliceImageError(*this, operation.GetSliceIndex(),
                                          operation.GetSlice());
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


  void OrthancSlicesLoader::ScheduleLoadInstance(const std::string& instanceId,
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
        (*webCallback_, uri, Operation::DownloadInstanceGeometry(instanceId, frame));
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
                                        const SliceGeometry& plane) const
  {
    if (state_ != State_GeometryReady)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    return slices_.LookupSlice(index, plane);
  }
  

  void OrthancSlicesLoader::ScheduleLoadSliceImage(size_t index)
  {
    if (state_ != State_GeometryReady)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      const Slice& slice = GetSlice(index);

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

      orthanc_.ScheduleGetRequest(*webCallback_, uri, Operation::DownloadSliceImage(index, slice));
    }
  }
}
