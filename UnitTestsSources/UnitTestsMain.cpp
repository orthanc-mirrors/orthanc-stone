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


#include "../Framework/dev.h"
#include "gtest/gtest.h"

#include "../Framework/Layers/FrameRenderer.h"
#include "../Framework/Layers/LayerSourceBase.h"
#include "../Framework/Toolbox/DownloadStack.h"
#include "../Framework/Toolbox/OrthancSlicesLoader.h"
#include "../Framework/Volumes/ImageBuffer3D.h"
#include "../Framework/Volumes/SlicedVolumeBase.h"
#include "../Platforms/Generic/OracleWebService.h"

#include <Core/HttpClient.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Logging.h>
#include <Core/MultiThreading/SharedMessageQueue.h>
#include <Core/OrthancException.h>

#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 
#include <boost/math/special_functions/round.hpp>


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
        const_cast<OrthancSlicesLoader&>(loader).ScheduleLoadSliceImage(i, SliceImageQuality_Full);
      }
    }

    virtual void NotifyGeometryError(const OrthancSlicesLoader& loader)
    {
      printf("Error\n");
    }

    virtual void NotifySliceImageReady(const OrthancSlicesLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice,
                                       std::auto_ptr<Orthanc::ImageAccessor>& image,
                                       SliceImageQuality quality)
    {
      std::auto_ptr<Orthanc::ImageAccessor> tmp(image);
      printf("Slice OK %dx%d\n", tmp->GetWidth(), tmp->GetHeight());
    }

    virtual void NotifySliceImageError(const OrthancSlicesLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice,
                                       SliceImageQuality quality)
    {
      printf("ERROR 2\n");
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
  OrthancStone::OrthancVolumeImage volume(orthanc, true);

  //volume.ScheduleLoadInstance("19816330-cb02e1cf-df3a8fe8-bf510623-ccefe9f5", 0);
  //volume.ScheduleLoadSeries("318603c5-03e8cffc-a82b6ee1-3ccd3c1e-18d7e3bb"); // COMUNIX PET
  //volume.ScheduleLoadSeries("7124dba7-09803f33-98b73826-33f14632-ea842d29"); // COMUNIX CT
  //volume.ScheduleLoadSeries("5990e39c-51e5f201-fe87a54c-31a55943-e59ef80e"); // Delphine sagital
  volume.ScheduleLoadSeries("6f1b492a-e181e200-44e51840-ef8db55e-af529ab6"); // Delphine ax 2.5

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
