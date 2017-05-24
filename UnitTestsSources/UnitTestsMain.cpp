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
#include "../Framework/Toolbox/OrthancAsynchronousWebService.h"
#include "../Framework/Toolbox/OrthancSlicesLoader.h"
#include "../Resources/Orthanc/Core/HttpClient.h"
#include "../Resources/Orthanc/Core/Logging.h"
#include "../Resources/Orthanc/Core/MultiThreading/SharedMessageQueue.h"
#include "../Resources/Orthanc/Core/OrthancException.h"

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
}


TEST(Toto, Tutu)
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



int main(int argc, char **argv)
{
  Orthanc::Logging::Initialize();
  Orthanc::Logging::EnableInfoLevel(true);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  Orthanc::Logging::Finalize();

  return result;
}
