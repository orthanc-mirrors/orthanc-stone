/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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
#include "../Framework/Toolbox/FiniteProjectiveCamera.h"
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


#if 0
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
#endif


TEST(GeometryToolbox, Interpolation)
{
  // https://en.wikipedia.org/wiki/Bilinear_interpolation#Application_in_image_processing
  ASSERT_FLOAT_EQ(146.1f, OrthancStone::GeometryToolbox::ComputeBilinearInterpolation
                  (20.5f, 14.2f, 91, 210, 162, 95));

  ASSERT_FLOAT_EQ(123.35f, OrthancStone::GeometryToolbox::ComputeTrilinearInterpolation
                  (20.5f, 15.2f, 5.7f,
                   91, 210, 162, 95,
                   51, 190, 80, 92));
}


TEST(FiniteProjectiveCamera, Decomposition)
{
  // Example 6.2 of "Multiple View Geometry in Computer Vision - 2nd
  // edition" (page 163)
  const double p[12] = {
    3.53553e+2,  3.39645e+2,  2.77744e+2, -1.44946e+6,
    -1.03528e+2, 2.33212e+1,  4.59607e+2, -6.32525e+5,
    7.07107e-1,  -3.53553e-1, 6.12372e-1, -9.18559e+2
  };

  OrthancStone::FiniteProjectiveCamera camera(p);
  ASSERT_EQ(3u, camera.GetMatrix().size1());
  ASSERT_EQ(4u, camera.GetMatrix().size2());
  ASSERT_EQ(3u, camera.GetIntrinsicParameters().size1());
  ASSERT_EQ(3u, camera.GetIntrinsicParameters().size2());
  ASSERT_EQ(3u, camera.GetRotation().size1());
  ASSERT_EQ(3u, camera.GetRotation().size2());
  ASSERT_EQ(3u, camera.GetCenter().size());

  ASSERT_NEAR(1000.0, camera.GetCenter()[0], 0.01);
  ASSERT_NEAR(2000.0, camera.GetCenter()[1], 0.01);
  ASSERT_NEAR(1500.0, camera.GetCenter()[2], 0.01);

  ASSERT_NEAR(468.2, camera.GetIntrinsicParameters() (0, 0), 0.1);
  ASSERT_NEAR(91.2,  camera.GetIntrinsicParameters() (0, 1), 0.1);
  ASSERT_NEAR(300.0, camera.GetIntrinsicParameters() (0, 2), 0.1);
  ASSERT_NEAR(427.2, camera.GetIntrinsicParameters() (1, 1), 0.1);
  ASSERT_NEAR(200.0, camera.GetIntrinsicParameters() (1, 2), 0.1);
  ASSERT_NEAR(1.0,   camera.GetIntrinsicParameters() (2, 2), 0.1);

  ASSERT_NEAR(0, camera.GetIntrinsicParameters() (1, 0), 0.0000001);
  ASSERT_NEAR(0, camera.GetIntrinsicParameters() (2, 0), 0.0000001);
  ASSERT_NEAR(0, camera.GetIntrinsicParameters() (2, 1), 0.0000001);

  ASSERT_NEAR(0.41380,  camera.GetRotation() (0, 0), 0.00001);
  ASSERT_NEAR(0.90915,  camera.GetRotation() (0, 1), 0.00001);
  ASSERT_NEAR(0.04708,  camera.GetRotation() (0, 2), 0.00001);
  ASSERT_NEAR(-0.57338, camera.GetRotation() (1, 0), 0.00001);
  ASSERT_NEAR(0.22011,  camera.GetRotation() (1, 1), 0.00001);
  ASSERT_NEAR(0.78917,  camera.GetRotation() (1, 2), 0.00001);
  ASSERT_NEAR(0.70711,  camera.GetRotation() (2, 0), 0.00001);
  ASSERT_NEAR(-0.35355, camera.GetRotation() (2, 1), 0.00001);
  ASSERT_NEAR(0.61237,  camera.GetRotation() (2, 2), 0.00001);

  ASSERT_TRUE(OrthancStone::LinearAlgebra::IsRotationMatrix(camera.GetRotation()));

  OrthancStone::FiniteProjectiveCamera camera2(camera.GetIntrinsicParameters(),
                                               camera.GetRotation(),
                                               camera.GetCenter());
  ASSERT_EQ(3u, camera2.GetMatrix().size1());
  ASSERT_EQ(4u, camera2.GetMatrix().size2());
  ASSERT_EQ(3u, camera2.GetIntrinsicParameters().size1());
  ASSERT_EQ(3u, camera2.GetIntrinsicParameters().size2());
  ASSERT_EQ(3u, camera2.GetRotation().size1());
  ASSERT_EQ(3u, camera2.GetRotation().size2());
  ASSERT_EQ(3u, camera2.GetCenter().size());

  for (size_t i = 0; i < 3; i++)
  {
    for (size_t j = 0; j < 4; j++)
    {
      ASSERT_NEAR(camera.GetMatrix() (i, j),
                  camera2.GetMatrix() (i, j), 0.00000001);
    }
  }
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
