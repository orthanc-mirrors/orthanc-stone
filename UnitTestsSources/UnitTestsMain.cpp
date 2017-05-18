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


#include "../Framework/Toolbox/MessagingToolbox.h"
#include "../Framework/Toolbox/DicomFrameConverter.h"

namespace OrthancStone
{
  class SeriesLoader :
    public IWebService::ICallback // TODO move to PImpl
  {
  public:
    class ICallback : public boost::noncopyable
    {
    public:
      virtual ~ICallback()
      {
      }

      virtual void NotifyGeometryReady(const SeriesLoader& series) = 0;

      virtual void NotifyGeometryError(const SeriesLoader& series) = 0;
    };
    
  private:
    class Slice : public boost::noncopyable
    {
    private:
      std::string          instanceId_;
      SliceGeometry        geometry_;
      double               thickness_;
      unsigned int         width_;
      unsigned int         height_;
      double               projectionAlongNormal_;
      DicomFrameConverter  converter_;
      
    public:
      Slice(const std::string& instanceId,
            const std::string& imagePositionPatient,
            const std::string& imageOrientationPatient,
            double thickness,
            unsigned int width,
            unsigned int height,
            const OrthancPlugins::IDicomDataset& dataset) :
        instanceId_(instanceId),
        geometry_(imagePositionPatient, imageOrientationPatient),
        thickness_(thickness),
        width_(width),
        height_(height)
      {
        converter_.ReadParameters(dataset);
      }

      const std::string GetInstanceId() const
      {
        return instanceId_;
      }

      const SliceGeometry& GetGeometry() const
      {
        return geometry_;
      }

      double GetThickness() const
      {
        return thickness_;
      }

      unsigned int GetWidth() const
      {
        return width_;
      }

      unsigned int GetHeight() const
      {
        return height_;
      }

      const DicomFrameConverter& GetConverter() const
      {
        return converter_;
      }

      void SetNormal(const Vector& normal)
      {
        projectionAlongNormal_ = boost::numeric::ublas::inner_prod(geometry_.GetOrigin(), normal);
      }

      double GetProjectionAlongNormal() const
      {
        return projectionAlongNormal_;
      }
    };


    class SetOfSlices : public boost::noncopyable
    {
    private:
      std::vector<Slice*>  slices_;

      struct Comparator
      {
        bool operator() (const Slice* const a,
                         const Slice* const b) const
        {
          return a->GetProjectionAlongNormal() < b->GetProjectionAlongNormal();
        }
      };

    public:
      ~SetOfSlices()
      {
        for (size_t i = 0; i < slices_.size(); i++)
        {
          assert(slices_[i] != NULL);
          delete slices_[i];
        }
      }

      void Reserve(size_t size)
      {
        slices_.reserve(size);
      }

      void AddSlice(const std::string& instanceId,
                    const Json::Value& value)
      {
        OrthancPlugins::FullOrthancDataset dataset(value);
        OrthancPlugins::DicomDatasetReader reader(dataset);

        std::string position, orientation;
        double thickness;
        unsigned int width, height;
        if (dataset.GetStringValue(position, OrthancPlugins::DICOM_TAG_IMAGE_POSITION_PATIENT) &&
            dataset.GetStringValue(orientation, OrthancPlugins::DICOM_TAG_IMAGE_ORIENTATION_PATIENT) &&
            reader.GetDoubleValue(thickness, OrthancPlugins::DICOM_TAG_SLICE_THICKNESS) &&
            reader.GetUnsignedIntegerValue(width, OrthancPlugins::DICOM_TAG_COLUMNS) &&
            reader.GetUnsignedIntegerValue(height, OrthancPlugins::DICOM_TAG_ROWS));
        {
          slices_.push_back(new Slice(instanceId, position, orientation,
                                      thickness, width, height, dataset));
        }       
      }

      size_t GetSliceCount() const
      {
        return slices_.size();
      }

      const Slice& GetSlice(size_t index) const
      {
        assert(slices_[index] != NULL);
        return *slices_[index];
      }
      
      void Sort(const Vector& normal)
      {
        for (size_t i = 0; i < slices_.size(); i++)
        {
          slices_[i]->SetNormal(normal);
        }

        Comparator comparator;
        std::sort(slices_.begin(), slices_.end(), comparator);
      }


      void SelectNormal(Vector& normal) const
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

        if (!found)
        {
          LOG(ERROR) << "Cannot select a normal that is shared by most of the slices of this series";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);          
        }
      }


      void FilterNormal(const Vector& normal)
      {
        size_t pos = 0;

        for (size_t i = 0; i < slices_.size(); i++)
        {
          if (GeometryToolbox::IsParallel(normal, slices_[i]->GetGeometry().GetNormal()))
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
    };
    
    
    enum Mode
    {
      Mode_Geometry
    };

    class Operation : public Orthanc::IDynamicObject
    {
    private:
      Mode         mode_;
      unsigned int instance_;

      Operation(Mode mode) :
        mode_(mode)
      {
      }

    public:
      Mode GetMode() const
      {
        return mode_;
      }
      
      static Operation* DownloadGeometry()
      {
        return new Operation(Mode_Geometry);
      }
    };
    
    ICallback&    callback_;
    IWebService&  orthanc_;
    std::string   seriesId_;
    SetOfSlices   slices_;


    void ParseGeometry(const void* answer,
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
        slices_.AddSlice(instances[i], series[instances[i]]);
      }

      bool ok = false;
      
      if (slices_.GetSliceCount() > 0)
      {
        Vector normal;
        slices_.SelectNormal(normal);
        slices_.FilterNormal(normal);
        slices_.Sort(normal);
        ok = true;
      }

      if (ok)
      {
        printf("%d\n", slices_.GetSliceCount());
        callback_.NotifyGeometryReady(*this);
      }
      else
      {
        LOG(ERROR) << "This series is empty";
        callback_.NotifyGeometryError(*this);
        return;
      }
    }
    
    
  public:
    SeriesLoader(ICallback& callback,
                 IWebService& orthanc,
                 const std::string& seriesId) :
      callback_(callback),
      orthanc_(orthanc),
      seriesId_(seriesId)
    {
      std::string uri = "/series/" + seriesId + "/instances-tags";
      orthanc.ScheduleGetRequest(*this, uri, Operation::DownloadGeometry());
    }

    const std::string& GetSeriesId() const
    {
      return seriesId_;
    }

    virtual void NotifySuccess(const std::string& uri,
                               const void* answer,
                               size_t answerSize,
                               Orthanc::IDynamicObject* payload)
    {
      std::auto_ptr<Operation> operation(dynamic_cast<Operation*>(payload));

      switch (operation->GetMode())
      {
        case Mode_Geometry:
          ParseGeometry(answer, answerSize);
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
        case Mode_Geometry:
          callback_.NotifyGeometryError(*this);
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  };


  class Tata : public SeriesLoader::ICallback
  {
  public:
    virtual void NotifyGeometryReady(const SeriesLoader& series)
    {
      printf("Done %s\n", series.GetSeriesId().c_str());
    }

    virtual void NotifyGeometryError(const SeriesLoader& series)
    {
      printf("Error %s\n", series.GetSeriesId().c_str());
    }
  };
}


TEST(Toto, Tutu)
{
  Orthanc::WebServiceParameters web;
  OrthancStone::OrthancWebService orthanc(web);

  OrthancStone::Tata tata;
  //OrthancStone::SeriesLoader loader(tata, orthanc, "c1c4cb95-05e3bd11-8da9f5bb-87278f71-0b2b43f5");
  OrthancStone::SeriesLoader loader(tata, orthanc, "67f1b334-02c16752-45026e40-a5b60b6b-030ecab5");
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
