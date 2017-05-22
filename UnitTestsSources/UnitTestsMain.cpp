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


#include "gtest/gtest.h"

#include "../Resources/Orthanc/Core/Logging.h"
#include "../Framework/Toolbox/OrthancWebService.h"
#include "../Framework/Layers/OrthancFrameLayerSource.h"
#include "../Framework/Widgets/LayerWidget.h"


#include "../Resources/Orthanc/Core/Images/PngReader.h"
#include "../Framework/Toolbox/MessagingToolbox.h"
#include "../Framework/Toolbox/DicomFrameConverter.h"

#include <boost/lexical_cast.hpp>

namespace OrthancStone
{
  class Slice
  {
  private:
    enum Type
    {
      Type_Invalid,
      Type_OrthancInstance
      // TODO A slice could come from some DICOM file (URL)
    };

    Type                 type_;
    std::string          orthancInstanceId_;
    unsigned int         frame_;
    SliceGeometry        geometry_;
    double               pixelSpacingX_;
    double               pixelSpacingY_;
    double               thickness_;
    unsigned int         width_;
    unsigned int         height_;
    DicomFrameConverter  converter_;
      
  public:
    Slice() : type_(Type_Invalid)
    {        
    }

    bool ParseOrthancFrame(const OrthancPlugins::IDicomDataset& dataset,
                           const std::string& instanceId,
                           unsigned int frame)
    {
      OrthancPlugins::DicomDatasetReader reader(dataset);

      unsigned int frameCount;
      if (!reader.GetUnsignedIntegerValue(frameCount, OrthancPlugins::DICOM_TAG_NUMBER_OF_FRAMES))
      {
        frameCount = 1;   // Assume instance with one frame
      }

      if (frame >= frameCount)
      {
        return false;
      }

      if (!reader.GetDoubleValue(thickness_, OrthancPlugins::DICOM_TAG_SLICE_THICKNESS))
      {
        thickness_ = 100.0 * std::numeric_limits<double>::epsilon();
      }

      GeometryToolbox::GetPixelSpacing(pixelSpacingX_, pixelSpacingY_, dataset);

      std::string position, orientation;
      if (dataset.GetStringValue(position, OrthancPlugins::DICOM_TAG_IMAGE_POSITION_PATIENT) &&
          dataset.GetStringValue(orientation, OrthancPlugins::DICOM_TAG_IMAGE_ORIENTATION_PATIENT) &&
          reader.GetUnsignedIntegerValue(width_, OrthancPlugins::DICOM_TAG_COLUMNS) &&
          reader.GetUnsignedIntegerValue(height_, OrthancPlugins::DICOM_TAG_ROWS))
      {
        orthancInstanceId_ = instanceId;
        frame_ = frame;
        geometry_ = SliceGeometry(position, orientation);
        converter_.ReadParameters(dataset);

        type_ = Type_OrthancInstance;
        return true;
      }
      else
      {
        return false;
      }
    }

    bool IsOrthancInstance() const
    {
      return type_ == Type_OrthancInstance;
    }

    const std::string GetOrthancInstanceId() const
    {
      if (type_ != Type_OrthancInstance)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
        
      return orthancInstanceId_;
    }

    unsigned int GetFrame() const
    {
      if (type_ == Type_Invalid)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
        
      return frame_;
    }

    const SliceGeometry& GetGeometry() const
    {
      if (type_ == Type_Invalid)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
        
      return geometry_;
    }

    double GetThickness() const
    {
      if (type_ == Type_Invalid)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
        
      return thickness_;
    }

    double GetPixelSpacingX() const
    {
      if (type_ == Type_Invalid)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
        
      return pixelSpacingX_;
    }

    double GetPixelSpacingY() const
    {
      if (type_ == Type_Invalid)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
        
      return pixelSpacingY_;
    }

    unsigned int GetWidth() const
    {
      if (type_ == Type_Invalid)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
        
      return width_;
    }

    unsigned int GetHeight() const
    {
      if (type_ == Type_Invalid)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
        
      return height_;
    }

    const DicomFrameConverter& GetConverter() const
    {
      if (type_ == Type_Invalid)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
        
      return converter_;
    }
  };


  class SliceSorter : public boost::noncopyable
  {
  private:
    class SliceWithDepth : public boost::noncopyable
    {
    private:
      Slice   slice_;
      double  depth_;

    public:
      SliceWithDepth(const Slice& slice) :
        slice_(slice),
        depth_(0)
      {
      }

      void SetNormal(const Vector& normal)
      {
        depth_ = boost::numeric::ublas::inner_prod
          (slice_.GetGeometry().GetOrigin(), normal);
      }

      double GetDepth() const
      {
        return depth_;
      }

      const Slice& GetSlice() const
      {
        return slice_;
      }
    };

    struct Comparator
    {
      bool operator() (const SliceWithDepth* const& a,
                       const SliceWithDepth* const& b) const
      {
        return a->GetDepth() < b->GetDepth();
      }
    };

    typedef std::vector<SliceWithDepth*>  Slices;

    Slices  slices_;
    bool    hasNormal_;
    
  public:
    SliceSorter() : hasNormal_(false)
    {
    }

    ~SliceSorter()
    {
      for (size_t i = 0; i < slices_.size(); i++)
      {
        assert(slices_[i] != NULL);
        delete slices_[i];
      }
    }
    
    void Reserve(size_t count)
    {
      slices_.reserve(count);
    }

    void AddSlice(const Slice& slice)
    {
      slices_.push_back(new SliceWithDepth(slice));
    }

    size_t GetSliceCount() const
    {
      return slices_.size();
    }

    const Slice& GetSlice(size_t i) const
    {
      if (i >= slices_.size())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      assert(slices_[i] != NULL);
      return slices_[i]->GetSlice();
    }

    void SetNormal(const Vector& normal)
    {
      for (size_t i = 0; i < slices_.size(); i++)
      {
        slices_[i]->SetNormal(normal);
      }

      hasNormal_ = true;
    }
    
    void Sort()
    {
      if (!hasNormal_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      Comparator comparator;
      std::sort(slices_.begin(), slices_.end(), comparator);
    }

    void FilterNormal(const Vector& normal)
    {
      size_t pos = 0;

      for (size_t i = 0; i < slices_.size(); i++)
      {
        if (GeometryToolbox::IsParallel(normal, slices_[i]->GetSlice().GetGeometry().GetNormal()))
        {
          // This slice is compatible with the selected normal
          slices_[pos] = slices_[i];
          pos += 1;
        }
        else
        {
          delete slices_[i];
          slices_[i] = NULL;
        }
      }

      slices_.resize(pos);
    }
    
    bool SelectNormal(Vector& normal) const
    {
      std::vector<Vector>  normalCandidates;
      std::vector<unsigned int>  normalCount;

      bool found = false;

      for (size_t i = 0; !found && i < GetSliceCount(); i++)
      {
        const Vector& normal = GetSlice(i).GetGeometry().GetNormal();

        bool add = true;
        for (size_t j = 0; add && j < normalCandidates.size(); j++)  // (*)
        {
          if (GeometryToolbox::IsParallel(normal, normalCandidates[j]))
          {
            normalCount[j] += 1;
            add = false;
          }
        }

        if (add)
        {
          if (normalCount.size() > 2)
          {
            // To get linear-time complexity in (*). This heuristics
            // allows the series to have one single frame that is
            // not parallel to the others (such a frame could be a
            // generated preview)
            found = false;
          }
          else
          {
            normalCandidates.push_back(normal);
            normalCount.push_back(1);
          }
        }
      }

      for (size_t i = 0; !found && i < normalCandidates.size(); i++)
      {
        unsigned int count = normalCount[i];
        if (count == GetSliceCount() ||
            count + 1 == GetSliceCount())
        {
          normal = normalCandidates[i];
          found = true;
        }
      }

      return found;
    }
  };



  class OrthancSliceLoader :
    public IWebService::ICallback // TODO move to PImpl
  {
  public:
    class ICallback : public boost::noncopyable
    {
    public:
      virtual ~ICallback()
      {
      }

      virtual void NotifyGeometryReady(const OrthancSliceLoader& loader) = 0;

      virtual void NotifyGeometryError(const OrthancSliceLoader& loader) = 0;

      virtual void NotifySliceImageReady(const OrthancSliceLoader& loader,
                                         unsigned int sliceIndex,
                                         const Slice& slice,
                                         Orthanc::ImageAccessor* image) = 0;

      virtual void NotifySliceImageError(const OrthancSliceLoader& loader,
                                         unsigned int sliceIndex,
                                         const Slice& slice) = 0;
    };
    
  private:
    enum State
    {
      State_Error,
      State_Initialization,
      State_LoadingGeometry,
      State_GeometryReady
    };
    
    enum Mode
    {
      Mode_SeriesGeometry,
      Mode_LoadImage
    };

    class Operation : public Orthanc::IDynamicObject
    {
    private:
      Mode          mode_;
      unsigned int  sliceIndex_;
      const Slice*  slice_;

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
      
      static Operation* DownloadSeriesGeometry()
      {
        return new Operation(Mode_SeriesGeometry);
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
    
    ICallback&    callback_;
    IWebService&  orthanc_;
    State         state_;
    SliceSorter   slices_;


    void ParseSeriesGeometry(const void* answer,
                             size_t size)
    {
      Json::Value series;
      if (!MessagingToolbox::ParseJson(series, answer, size) ||
          series.type() != Json::objectValue)
      {
        callback_.NotifyGeometryError(*this);
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

      if (ok)
      {
        LOG(INFO) << "Loaded a series with " << slices_.GetSliceCount() << " slice(s)";
        callback_.NotifyGeometryReady(*this);
      }
      else
      {
        LOG(ERROR) << "This series is empty";
        callback_.NotifyGeometryError(*this);
      }
    }


    void ParseSliceImage(const Operation& operation,
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
        callback_.NotifySliceImageReady(*this, operation.GetSliceIndex(),
                                        operation.GetSlice(), image.release());
      }
      else
      {
        callback_.NotifySliceImageError(*this, operation.GetSliceIndex(),
                                        operation.GetSlice());
      }
    }
    
    
  public:
    OrthancSliceLoader(ICallback& callback,
                 IWebService& orthanc) :
      callback_(callback),
      orthanc_(orthanc),
      state_(State_Initialization)
    {
    }

    void ScheduleLoadSeries(const std::string& seriesId)
    {
      if (state_ != State_Initialization)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        state_ = State_LoadingGeometry;
        std::string uri = "/series/" + seriesId + "/instances-tags";
        orthanc_.ScheduleGetRequest(*this, uri, Operation::DownloadSeriesGeometry());
      }
    }

    size_t GetSliceCount() const
    {
      if (state_ != State_GeometryReady)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      return slices_.GetSliceCount();
    }

    const Slice& GetSlice(size_t index) const
    {
      if (state_ != State_GeometryReady)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      return slices_.GetSlice(index);
    }

    void ScheduleLoadSliceImage(size_t index)
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

        orthanc_.ScheduleGetRequest(*this, uri, Operation::DownloadSliceImage(index, slice));
      }
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
          ParseSeriesGeometry(answer, answerSize);
          state_ = State_GeometryReady;
          break;

        case Mode_LoadImage:
          ParseSliceImage(*operation, answer, answerSize);
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
          callback_.NotifyGeometryError(*this);
          state_ = State_Error;
          break;

        case Mode_LoadImage:
          callback_.NotifySliceImageError(*this, operation->GetSliceIndex(), operation->GetSlice());
          break;
          
        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  };


  class Tata : public OrthancSliceLoader::ICallback
  {
  public:
    virtual void NotifyGeometryReady(const OrthancSliceLoader& loader)
    {
      printf("Done\n");
    }

    virtual void NotifyGeometryError(const OrthancSliceLoader& loader)
    {
      printf("Error\n");
    }

    virtual void NotifySliceImageReady(const OrthancSliceLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice,
                                       Orthanc::ImageAccessor* image)
    {
      std::auto_ptr<Orthanc::ImageAccessor> tmp(image);
      printf("Slice OK\n");
    }

    virtual void NotifySliceImageError(const OrthancSliceLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice)
    {
      printf("ERROR 2\n");
    }
  };
}


TEST(Toto, Tutu)
{
  Orthanc::WebServiceParameters web;
  OrthancStone::OrthancWebService orthanc(web);

  OrthancStone::Tata tata;
  OrthancStone::OrthancSliceLoader loader(tata, orthanc);
  //loader.ScheduleLoadSeries("c1c4cb95-05e3bd11-8da9f5bb-87278f71-0b2b43f5");
  loader.ScheduleLoadSeries("67f1b334-02c16752-45026e40-a5b60b6b-030ecab5");

  printf(">> %d\n", loader.GetSliceCount());
  loader.ScheduleLoadSliceImage(31);
}



int main(int argc, char **argv)
{
  Orthanc::Logging::Initialize();
  Orthanc::Logging::EnableInfoLevel(true);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  Orthanc::Logging::Finalize();

  return result;
}
