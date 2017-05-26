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

#include "../Platforms/Generic/OracleWebService.h"
#include "../Framework/Toolbox/OrthancSlicesLoader.h"
#include "../Resources/Orthanc/Core/HttpClient.h"
#include "../Resources/Orthanc/Core/Logging.h"
#include "../Resources/Orthanc/Core/MultiThreading/SharedMessageQueue.h"
#include "../Resources/Orthanc/Core/OrthancException.h"

#include "../Framework/Volumes/ImageBuffer3D.h"
#include "../Framework/Volumes/SlicedVolumeBase.h"
#include "../Framework/Toolbox/DownloadStack.h"
#include "../Resources/Orthanc/Core/Images/ImageProcessing.h"

#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 

namespace OrthancStone
{
  class Tata : public OrthancSlicesLoader::ICallback
  {
  public:
    virtual void NotifyGeometryReady(const OrthancSlicesLoader& loader)
    {
      printf(">> %d\n", (int) loader.GetSliceCount());

      for (size_t i = 0; i < loader.GetSliceCount(); i++)
      {
        const_cast<OrthancSlicesLoader&>(loader).ScheduleLoadSliceImage(i);
      }
    }

    virtual void NotifyGeometryError(const OrthancSlicesLoader& loader)
    {
      printf("Error\n");
    }

    virtual void NotifySliceImageReady(const OrthancSlicesLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice,
                                       Orthanc::ImageAccessor* image)
    {
      std::auto_ptr<Orthanc::ImageAccessor> tmp(image);
      printf("Slice OK %dx%d\n", tmp->GetWidth(), tmp->GetHeight());
    }

    virtual void NotifySliceImageError(const OrthancSlicesLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice)
    {
      printf("ERROR 2\n");
    }
  };


  class OrthancVolumeImageLoader : 
    public SlicedVolumeBase,
    private OrthancSlicesLoader::ICallback
  { 
  private:
    OrthancSlicesLoader           loader_;
    std::auto_ptr<ImageBuffer3D>  image_;
    std::auto_ptr<DownloadStack>  downloadStack_;

    
    void ScheduleSliceDownload()
    {
      assert(downloadStack_.get() != NULL);

      unsigned int slice;
      if (downloadStack_->Pop(slice))
      {
        loader_.ScheduleLoadSliceImage(slice);
      }
    }


    static bool IsCompatible(const Slice& a, 
                             const Slice& b)
    {
      if (!GeometryToolbox::IsParallel(a.GetGeometry().GetNormal(),
                                       b.GetGeometry().GetNormal()))
      {
        LOG(ERROR) << "Some slice in the volume image is not parallel to the others";
        return false;
      }

      if (a.GetConverter().GetExpectedPixelFormat() != b.GetConverter().GetExpectedPixelFormat())
      {
        LOG(ERROR) << "The pixel format changes across the slices of the volume image";
        return false;
      }

      if (a.GetWidth() != b.GetWidth() ||
          a.GetHeight() != b.GetHeight())
      {
        LOG(ERROR) << "The width/height of the slices change across the volume image";
        return false;
      }

      if (!GeometryToolbox::IsNear(a.GetPixelSpacingX(), b.GetPixelSpacingX()) ||
          !GeometryToolbox::IsNear(a.GetPixelSpacingY(), b.GetPixelSpacingY()))
      {
        LOG(ERROR) << "The pixel spacing of the slices change across the volume image";
        return false;
      }

      return true;
    }


    static double GetDistance(const Slice& a, 
                              const Slice& b)
    {
      return fabs(a.GetGeometry().ProjectAlongNormal(a.GetGeometry().GetOrigin()) - 
                  a.GetGeometry().ProjectAlongNormal(b.GetGeometry().GetOrigin()));
    }


    virtual void NotifyGeometryReady(const OrthancSlicesLoader& loader)
    {
      if (loader.GetSliceCount() == 0)
      {
        LOG(ERROR) << "Empty volume image";
        SlicedVolumeBase::NotifyGeometryError();
        return;
      }

      for (size_t i = 1; i < loader.GetSliceCount(); i++)
      {
        if (!IsCompatible(loader.GetSlice(0), loader.GetSlice(i)))
        {
          SlicedVolumeBase::NotifyGeometryError();
          return;
        }
      }

      double spacingZ;

      if (loader.GetSliceCount() > 1)
      {
        spacingZ = GetDistance(loader.GetSlice(0), loader.GetSlice(1));
      }
      else
      {
        // This is a volume with one single slice: Choose a dummy
        // z-dimension for voxels
        spacingZ = 1;
      }
      
      for (size_t i = 1; i < loader.GetSliceCount(); i++)
      {
        if (!GeometryToolbox::IsNear(spacingZ, GetDistance(loader.GetSlice(i - 1), loader.GetSlice(i))))
        {
          LOG(ERROR) << "The distance between successive slices is not constant in a volume image";
          SlicedVolumeBase::NotifyGeometryError();
          return;
        }
      }

      unsigned int width = loader.GetSlice(0).GetWidth();
      unsigned int height = loader.GetSlice(0).GetHeight();
      Orthanc::PixelFormat format = loader.GetSlice(0).GetConverter().GetExpectedPixelFormat();
      LOG(INFO) << "Creating a volume image of size " << width << "x" << height 
                << "x" << loader.GetSliceCount() << " in " << Orthanc::EnumerationToString(format);

      image_.reset(new ImageBuffer3D(format, width, height, loader.GetSliceCount()));
      image_->SetAxialGeometry(loader.GetSlice(0).GetGeometry());
      image_->SetVoxelDimensions(loader.GetSlice(0).GetPixelSpacingX(), 
                                 loader.GetSlice(0).GetPixelSpacingY(), spacingZ);
      image_->Clear();

      downloadStack_.reset(new DownloadStack(loader.GetSliceCount()));

      SlicedVolumeBase::NotifyGeometryReady();

      for (unsigned int i = 0; i < 4; i++)  // Limit to 4 simultaneous downloads
      {
        ScheduleSliceDownload();
      }
    }

    virtual void NotifyGeometryError(const OrthancSlicesLoader& loader)
    {
      LOG(ERROR) << "Unable to download a volume image";
    }

    virtual void NotifySliceImageReady(const OrthancSlicesLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice,
                                       Orthanc::ImageAccessor* image)
    {
      std::auto_ptr<Orthanc::ImageAccessor> protection(image);

      {
        ImageBuffer3D::SliceWriter writer(*image_, VolumeProjection_Axial, sliceIndex);
        Orthanc::ImageProcessing::Copy(writer.GetAccessor(), *protection);
      }

      SlicedVolumeBase::NotifySliceChange(sliceIndex, slice);

      ScheduleSliceDownload();
    }

    virtual void NotifySliceImageError(const OrthancSlicesLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice)
    {
      LOG(ERROR) << "Cannot download slice " << sliceIndex << " in a volume image";
      ScheduleSliceDownload();
    }

  public:
    OrthancVolumeImageLoader(IWebService& orthanc) : 
      loader_(*this, orthanc)
    {
    }

    void ScheduleLoadSeries(const std::string& seriesId)
    {
      loader_.ScheduleLoadSeries(seriesId);
    }

    void ScheduleLoadInstance(const std::string& instanceId,
                              unsigned int frame)
    {
      loader_.ScheduleLoadInstance(instanceId, frame);
    }

    virtual size_t GetSliceCount() const
    {
      return loader_.GetSliceCount();
    }

    virtual const Slice& GetSlice(size_t index) const
    {
      return loader_.GetSlice(index);
    }
  };
}


TEST(Toto, DISABLED_Tutu)
{
  OrthancStone::Oracle oracle(4);
  oracle.Start();

  Orthanc::WebServiceParameters web;
  //OrthancStone::OrthancAsynchronousWebService orthanc(web, 4);
  OrthancStone::OracleWebService orthanc(oracle, web);
  //orthanc.Start();

  OrthancStone::Tata tata;
  OrthancStone::OrthancSlicesLoader loader(tata, orthanc);
  loader.ScheduleLoadSeries("c1c4cb95-05e3bd11-8da9f5bb-87278f71-0b2b43f5");
  //loader.ScheduleLoadSeries("67f1b334-02c16752-45026e40-a5b60b6b-030ecab5");

  //loader.ScheduleLoadInstance("19816330-cb02e1cf-df3a8fe8-bf510623-ccefe9f5", 0);

  /*printf(">> %d\n", loader.GetSliceCount());
    loader.ScheduleLoadSliceImage(31);*/

  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

  //orthanc.Stop();
  oracle.Stop();
}


TEST(Toto, Tata)
{
  OrthancStone::Oracle oracle(4);
  oracle.Start();

  Orthanc::WebServiceParameters web;
  OrthancStone::OracleWebService orthanc(oracle, web);
  OrthancStone::OrthancVolumeImageLoader volume(orthanc);

  volume.ScheduleLoadInstance("19816330-cb02e1cf-df3a8fe8-bf510623-ccefe9f5", 0);
  //volume.ScheduleLoadSeries("318603c5-03e8cffc-a82b6ee1-3ccd3c1e-18d7e3bb"); // COMUNIX PET
  //volume.ScheduleLoadSeries("5990e39c-51e5f201-fe87a54c-31a55943-e59ef80e"); // Delphine sagital

  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

  oracle.Stop();
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
