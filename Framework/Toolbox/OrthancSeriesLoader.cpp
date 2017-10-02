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


#include "OrthancSeriesLoader.h"

#include "../Toolbox/MessagingToolbox.h"

#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>

#include "DicomFrameConverter.h"

namespace OrthancStone
{
  class OrthancSeriesLoader::Slice : public boost::noncopyable
  {
  private:
    std::string         instanceId_;
    CoordinateSystem3D  geometry_;
    double              projectionAlongNormal_;

  public:
    Slice(const std::string& instanceId,
          const std::string& imagePositionPatient,
          const std::string& imageOrientationPatient) :
      instanceId_(instanceId),
      geometry_(imagePositionPatient, imageOrientationPatient)
    {
    }

    const std::string GetInstanceId() const
    {
      return instanceId_;
    }

    const CoordinateSystem3D& GetGeometry() const
    {
      return geometry_;
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


  class OrthancSeriesLoader::SetOfSlices : public boost::noncopyable
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
                  const std::string& imagePositionPatient,
                  const std::string& imageOrientationPatient)
    {
      slices_.push_back(new Slice(instanceId, imagePositionPatient, imageOrientationPatient));
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

    void LoadSeriesFast(OrthancPlugins::IOrthancConnection&  orthanc,
                        const std::string& series)
    {
      // Retrieve the orientation of this series
      Json::Value info;
      MessagingToolbox::RestApiGet(info, orthanc, "/series/" + series);

      if (info.type() != Json::objectValue ||
          !info.isMember("MainDicomTags") ||
          info["MainDicomTags"].type() != Json::objectValue ||
          !info["MainDicomTags"].isMember("ImageOrientationPatient") ||
          info["MainDicomTags"]["ImageOrientationPatient"].type() != Json::stringValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      std::string imageOrientationPatient = info["MainDicomTags"]["ImageOrientationPatient"].asString();


      // Retrieve the Orthanc ID of all the instances of this series
      Json::Value instances;
      MessagingToolbox::RestApiGet(instances, orthanc, "/series/" + series + "/instances");

      if (instances.type() != Json::arrayValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      if (instances.size() == 0)
      {
        LOG(ERROR) << "This series is empty";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
      }


      // Retrieve the DICOM tags of all the instances
      std::vector<std::string> instancesId;

      instancesId.resize(instances.size());
      Reserve(instances.size());

      for (Json::Value::ArrayIndex i = 0; i < instances.size(); i++)
      {
        if (instances[i].type() != Json::objectValue ||
            !instances[i].isMember("ID") ||
            !instances[i].isMember("MainDicomTags") ||
            instances[i]["ID"].type() != Json::stringValue ||
            instances[i]["MainDicomTags"].type() != Json::objectValue ||
            !instances[i]["MainDicomTags"].isMember("ImagePositionPatient") ||
            instances[i]["MainDicomTags"]["ImagePositionPatient"].type() != Json::stringValue)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
        else
        {
          instancesId[i] = instances[i]["ID"].asString();
          AddSlice(instancesId[i], 
                   instances[i]["MainDicomTags"]["ImagePositionPatient"].asString(),
                   imageOrientationPatient);
        }
      }

      assert(GetSliceCount() == instances.size());
    }

      
    void LoadSeriesSafe(OrthancPlugins::IOrthancConnection& orthanc,
                        const std::string& seriesId)
    {
      Json::Value series;
      MessagingToolbox::RestApiGet(series, orthanc, "/series/" + seriesId + "/instances-tags?simplify");

      if (series.type() != Json::objectValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      if (series.size() == 0)
      {
        LOG(ERROR) << "This series is empty";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
      }

      Json::Value::Members instances = series.getMemberNames();

      Reserve(instances.size());

      for (Json::Value::ArrayIndex i = 0; i < instances.size(); i++)
      {
        const Json::Value& tags = series[instances[i]];

        if (tags.type() != Json::objectValue ||
            !tags.isMember("ImagePositionPatient") ||
            !tags.isMember("ImageOrientationPatient") ||
            tags["ImagePositionPatient"].type() != Json::stringValue ||
            tags["ImageOrientationPatient"].type() != Json::stringValue)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
        else
        {
          AddSlice(instances[i], 
                   tags["ImagePositionPatient"].asString(),
                   tags["ImageOrientationPatient"].asString());
        }
      }

      assert(GetSliceCount() == instances.size());
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



  OrthancSeriesLoader::OrthancSeriesLoader(OrthancPlugins::IOrthancConnection& orthanc,
                                           const std::string& series) :
    orthanc_(orthanc),
    slices_(new SetOfSlices)
  {
    /**
     * The function "LoadSeriesFast()" might not behave properly if
     * some slice has some outsider value for its normal, which
     * happens sometimes on reprojected series (e.g. coronal and
     * sagittal of Delphine). Don't use it.
     **/

    slices_->LoadSeriesSafe(orthanc, series);
    
    Vector normal;
    slices_->SelectNormal(normal);
    slices_->FilterNormal(normal);
    slices_->Sort(normal);

    if (slices_->GetSliceCount() == 0)  // Sanity check
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    for (size_t i = 0; i < slices_->GetSliceCount(); i++)
    {
      assert(GeometryToolbox::IsParallel(normal, slices_->GetSlice(i).GetGeometry().GetNormal()));
      geometry_.AddSlice(slices_->GetSlice(i).GetGeometry());
    }

    std::string uri = "/instances/" + slices_->GetSlice(0).GetInstanceId() + "/tags";

    OrthancPlugins::FullOrthancDataset dataset(orthanc_, uri);

    Orthanc::DicomMap dicom;
    MessagingToolbox::ConvertDataset(dicom, dataset);
 
    if (!dicom.ParseUnsignedInteger32(width_, Orthanc::DICOM_TAG_COLUMNS) ||
        !dicom.ParseUnsignedInteger32(height_, Orthanc::DICOM_TAG_ROWS))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentTag);
    }

    DicomFrameConverter converter;
    converter.ReadParameters(dicom);
    format_ = converter.GetExpectedPixelFormat();
  }
    

  OrthancPlugins::IDicomDataset* OrthancSeriesLoader::DownloadDicom(size_t index)
  {
    std::string uri = "/instances/" + slices_->GetSlice(index).GetInstanceId() + "/tags";

    std::auto_ptr<OrthancPlugins::IDicomDataset> dataset(new OrthancPlugins::FullOrthancDataset(orthanc_, uri));

    Orthanc::DicomMap dicom;
    MessagingToolbox::ConvertDataset(dicom, *dataset);
 
    uint32_t frames;
    if (dicom.ParseUnsignedInteger32(frames, Orthanc::DICOM_TAG_NUMBER_OF_FRAMES) &&
        frames != 1)
    {
      LOG(ERROR) << "One instance in this series has more than 1 frame";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);          
    }

    return dataset.release();
  }


  void OrthancSeriesLoader::CheckFrame(const Orthanc::ImageAccessor& frame) const
  {
    if (frame.GetFormat() != format_ ||
        frame.GetWidth() != width_ ||
        frame.GetHeight() != height_)
    {
      LOG(ERROR) << "The parameters of this series vary accross its slices";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);          
    }
  }


  Orthanc::ImageAccessor* OrthancSeriesLoader::DownloadFrame(size_t index)
  {
    const Slice& slice = slices_->GetSlice(index);

    std::auto_ptr<Orthanc::ImageAccessor> frame
      (MessagingToolbox::DecodeFrame(orthanc_, slice.GetInstanceId(), 0, format_));

    if (frame.get() != NULL)
    {
      CheckFrame(*frame);
    }

    return frame.release();
  }


  Orthanc::ImageAccessor* OrthancSeriesLoader::DownloadJpegFrame(size_t index,
                                                                 unsigned int quality)
  {
    const Slice& slice = slices_->GetSlice(index);

    std::auto_ptr<Orthanc::ImageAccessor> frame
      (MessagingToolbox::DecodeJpegFrame(orthanc_, slice.GetInstanceId(), 0, quality, format_));

    if (frame.get() != NULL)
    {
      CheckFrame(*frame);
    }

    return frame.release();
  }


  bool OrthancSeriesLoader::IsJpegAvailable()
  {
    return MessagingToolbox::HasWebViewerInstalled(orthanc_);
  }
}
