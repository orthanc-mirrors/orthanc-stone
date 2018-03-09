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
  using namespace OrthancStone::GeometryToolbox;
  
  // https://en.wikipedia.org/wiki/Bilinear_interpolation#Application_in_image_processing
  ASSERT_FLOAT_EQ(146.1f, ComputeBilinearInterpolationUnitSquare(0.5f, 0.2f, 91, 210, 162, 95));

  ASSERT_FLOAT_EQ(91,  ComputeBilinearInterpolationUnitSquare(0, 0, 91, 210, 162, 95));
  ASSERT_FLOAT_EQ(210, ComputeBilinearInterpolationUnitSquare(1, 0, 91, 210, 162, 95));
  ASSERT_FLOAT_EQ(162, ComputeBilinearInterpolationUnitSquare(0, 1, 91, 210, 162, 95));
  ASSERT_FLOAT_EQ(95,  ComputeBilinearInterpolationUnitSquare(1, 1, 91, 210, 162, 95));

  ASSERT_FLOAT_EQ(123.35f, ComputeTrilinearInterpolationUnitSquare
                  (0.5f, 0.2f, 0.7f,
                   91, 210, 162, 95,
                   51, 190, 80, 92));

  ASSERT_FLOAT_EQ(ComputeBilinearInterpolationUnitSquare(0.5f, 0.2f, 91, 210, 162, 95),
                  ComputeTrilinearInterpolationUnitSquare(0.5f, 0.2f, 0,
                                                          91, 210, 162, 95,
                                                          51, 190, 80, 92));

  ASSERT_FLOAT_EQ(ComputeBilinearInterpolationUnitSquare(0.5f, 0.2f, 51, 190, 80, 92),
                  ComputeTrilinearInterpolationUnitSquare(0.5f, 0.2f, 1,
                                                          91, 210, 162, 95,
                                                          51, 190, 80, 92));
}


static bool CompareMatrix(const OrthancStone::Matrix& a,
                          const OrthancStone::Matrix& b,
                          double threshold = 0.00000001)
{
  if (a.size1() != b.size1() ||
      a.size2() != b.size2())
  {
    return false;
  }

  for (size_t i = 0; i < a.size1(); i++)
  {
    for (size_t j = 0; j < a.size2(); j++)
    {
      if (fabs(a(i, j) - b(i, j)) > threshold)
      {
        LOG(ERROR) << "Too large difference in component ("
                   << i << "," << j << "): " << a(i,j) << " != " << b(i,j);
        return false;
      }
    }
  }

  return true;
}


static bool CompareVector(const OrthancStone::Vector& a,
                          const OrthancStone::Vector& b,
                          double threshold = 0.00000001)
{
  if (a.size() != b.size())
  {
    return false;
  }

  for (size_t i = 0; i < a.size(); i++)
  {
    if (fabs(a(i) - b(i)) > threshold)
    {
      LOG(ERROR) << "Too large difference in component "
                 << i << ": " << a(i) << " != " << b(i);
      return false;
    }
  }

  return true;
}



TEST(FiniteProjectiveCamera, Decomposition1)
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

  ASSERT_TRUE(CompareMatrix(camera.GetMatrix(), camera2.GetMatrix()));
  ASSERT_TRUE(CompareMatrix(camera.GetIntrinsicParameters(), camera2.GetIntrinsicParameters()));
  ASSERT_TRUE(CompareMatrix(camera.GetRotation(), camera2.GetRotation()));
  ASSERT_TRUE(CompareVector(camera.GetCenter(), camera2.GetCenter()));
}


TEST(FiniteProjectiveCamera, Decomposition2)
{
  const double p[] = { 1188.111986, 580.205341, -808.445330, 128000.000000, -366.466264, 1446.510501, 418.499736, 128000.000000, -0.487118, 0.291726, -0.823172, 500.000000 };
  const double k[] = { -1528.494743, 0.000000, 256.000000, 0.000000, 1528.494743, 256.000000, 0.000000, 0.000000, 1.000000 };
  const double r[] = { -0.858893, -0.330733, 0.391047, -0.158171, 0.897503, 0.411668, -0.487118, 0.291726, -0.823172 };
  const double c[] = { 243.558936, -145.863085, 411.585964 };

  OrthancStone::FiniteProjectiveCamera camera(p);
  ASSERT_TRUE(OrthancStone::LinearAlgebra::IsRotationMatrix(camera.GetRotation()));

  OrthancStone::FiniteProjectiveCamera camera2(k, r, c);
  ASSERT_TRUE(CompareMatrix(camera.GetMatrix(), camera2.GetMatrix(), 1));
  ASSERT_TRUE(CompareMatrix(camera.GetIntrinsicParameters(), camera2.GetIntrinsicParameters(), 0.001));
  ASSERT_TRUE(CompareMatrix(camera.GetRotation(), camera2.GetRotation(), 0.000001));
  ASSERT_TRUE(CompareVector(camera.GetCenter(), camera2.GetCenter(), 0.0001));
}


TEST(FiniteProjectiveCamera, Decomposition3)
{
  const double p[] = { 10, 0, 0, 0,
                       0, 20, 0, 0,
                       0, 0, 30, 0 };

  OrthancStone::FiniteProjectiveCamera camera(p);
  ASSERT_TRUE(OrthancStone::LinearAlgebra::IsRotationMatrix(camera.GetRotation()));
  ASSERT_DOUBLE_EQ(10, camera.GetIntrinsicParameters() (0, 0));
  ASSERT_DOUBLE_EQ(20, camera.GetIntrinsicParameters() (1, 1));
  ASSERT_DOUBLE_EQ(30, camera.GetIntrinsicParameters() (2, 2));
  ASSERT_DOUBLE_EQ(1, camera.GetRotation() (0, 0));
  ASSERT_DOUBLE_EQ(1, camera.GetRotation() (1, 1));
  ASSERT_DOUBLE_EQ(1, camera.GetRotation() (2, 2));
  ASSERT_DOUBLE_EQ(0, camera.GetCenter() (0));
  ASSERT_DOUBLE_EQ(0, camera.GetCenter() (1));
  ASSERT_DOUBLE_EQ(0, camera.GetCenter() (2));
}


TEST(FiniteProjectiveCamera, Decomposition4)
{
  const double p[] = { 1, 0, 0, 10,
                       0, 1, 0, 20,
                       0, 0, 1, 30 };

  OrthancStone::FiniteProjectiveCamera camera(p);
  ASSERT_TRUE(OrthancStone::LinearAlgebra::IsRotationMatrix(camera.GetRotation()));
  ASSERT_DOUBLE_EQ(1, camera.GetIntrinsicParameters() (0, 0));
  ASSERT_DOUBLE_EQ(1, camera.GetIntrinsicParameters() (1, 1));
  ASSERT_DOUBLE_EQ(1, camera.GetIntrinsicParameters() (2, 2));
  ASSERT_DOUBLE_EQ(1, camera.GetRotation() (0, 0));
  ASSERT_DOUBLE_EQ(1, camera.GetRotation() (1, 1));
  ASSERT_DOUBLE_EQ(1, camera.GetRotation() (2, 2));
  ASSERT_DOUBLE_EQ(-10, camera.GetCenter() (0));
  ASSERT_DOUBLE_EQ(-20, camera.GetCenter() (1));
  ASSERT_DOUBLE_EQ(-30, camera.GetCenter() (2));
}


TEST(FiniteProjectiveCamera, Decomposition5)
{
  const double p[] = { 0, 0, 10, 0,
                       0, 20, 0, 0,
                       30, 0, 0, 0 };

  OrthancStone::FiniteProjectiveCamera camera(p);
  ASSERT_TRUE(OrthancStone::LinearAlgebra::IsRotationMatrix(camera.GetRotation()));
  ASSERT_DOUBLE_EQ(-10, camera.GetIntrinsicParameters() (0, 0));
  ASSERT_DOUBLE_EQ(20, camera.GetIntrinsicParameters() (1, 1));
  ASSERT_DOUBLE_EQ(30, camera.GetIntrinsicParameters() (2, 2));
  ASSERT_DOUBLE_EQ(-1, camera.GetRotation() (0, 2));
  ASSERT_DOUBLE_EQ(1, camera.GetRotation() (1, 1));
  ASSERT_DOUBLE_EQ(1, camera.GetRotation() (2, 0));
  ASSERT_DOUBLE_EQ(0, camera.GetCenter() (0));
  ASSERT_DOUBLE_EQ(0, camera.GetCenter() (1));
  ASSERT_DOUBLE_EQ(0, camera.GetCenter() (2));

  OrthancStone::FiniteProjectiveCamera camera2(camera.GetIntrinsicParameters(),
                                               camera.GetRotation(),
                                               camera.GetCenter());
  ASSERT_TRUE(CompareMatrix(camera.GetMatrix(), camera2.GetMatrix()));
  ASSERT_TRUE(CompareMatrix(camera.GetIntrinsicParameters(), camera2.GetIntrinsicParameters()));
  ASSERT_TRUE(CompareMatrix(camera.GetRotation(), camera2.GetRotation()));
  ASSERT_TRUE(CompareVector(camera.GetCenter(), camera2.GetCenter()));
}


static double GetCosAngle(const OrthancStone::Vector& a,
                          const OrthancStone::Vector& b)
{
  // Returns the cosine of the angle between two vectors
  // https://en.wikipedia.org/wiki/Dot_product#Geometric_definition
  return boost::numeric::ublas::inner_prod(a, b) / 
    (boost::numeric::ublas::norm_2(a) * boost::numeric::ublas::norm_2(b));
}


TEST(FiniteProjectiveCamera, Ray)
{
  const double pp[] = { -1499.650894, 2954.618773, -259.737419, 637891.819097,
                        -2951.517707, -1501.019129, -285.785281, 637891.819097,
                        0.008528, 0.003067, -0.999959, 2491.764918 };

  const OrthancStone::FiniteProjectiveCamera camera(pp);

  ASSERT_NEAR(-21.2492, camera.GetCenter() (0), 0.0001);
  ASSERT_NEAR(-7.64234, camera.GetCenter() (1), 0.00001);
  ASSERT_NEAR(2491.66, camera.GetCenter() (2), 0.01);

  // Image plane that led to these parameters, with principal point at
  // (256,256). The image has dimensions 512x512.
  OrthancStone::Vector o =
    OrthancStone::LinearAlgebra::CreateVector(7.009620, 2.521030, -821.942000);
  OrthancStone::Vector ax =
    OrthancStone::LinearAlgebra::CreateVector(-0.453219, 0.891399, -0.001131);
  OrthancStone::Vector ay =
    OrthancStone::LinearAlgebra::CreateVector(-0.891359, -0.453210, -0.008992);

  OrthancStone::CoordinateSystem3D imagePlane(o, ax, ay);

  // Back-projection of the principal point
  {
    OrthancStone::Vector ray = camera.GetRayDirection(256, 256);

    // The principal axis vector is orthogonal to the image plane
    // (i.e. parallel to the plane normal), in the opposite direction
    // ("-1" corresponds to "cos(pi)").
    ASSERT_NEAR(-1, GetCosAngle(ray, imagePlane.GetNormal()), 0.0000001);

    // Forward projection of principal axis, resulting in the principal point
    double x, y;
    camera.ApplyFinite(x, y, camera.GetCenter() - ray);

    ASSERT_NEAR(256, x, 0.00001);
    ASSERT_NEAR(256, y, 0.00001);
  }

  // Back-projection of the 4 corners of the image
  std::vector<double> cx, cy;
  cx.push_back(0);
  cy.push_back(0);
  cx.push_back(512);
  cy.push_back(0);
  cx.push_back(512);
  cy.push_back(512);
  cx.push_back(0);
  cy.push_back(512);

  bool first = true;
  double angle;

  for (size_t i = 0; i < cx.size(); i++)
  {
    OrthancStone::Vector ray = camera.GetRayDirection(cx[i], cy[i]);

    // Check that the angle wrt. principal axis is the same for all
    // the 4 corners
    double a = GetCosAngle(ray, imagePlane.GetNormal());
    if (first)
    {
      first = false;
      angle = a;
    }
    else
    {
      ASSERT_NEAR(angle, a, 0.000001);
    }

    // Forward projection of the ray, going back to the original point
    double x, y;
    camera.ApplyFinite(x, y, camera.GetCenter() - ray);
    
    ASSERT_NEAR(cx[i], x, 0.00001);
    ASSERT_NEAR(cy[i], y, 0.00001);

    // Alternative construction, by computing the intersection of the
    // ray with the image plane
    OrthancStone::Vector p;
    ASSERT_TRUE(imagePlane.IntersectLine(p, camera.GetCenter(), -ray));
    imagePlane.ProjectPoint(x, y, p);
    ASSERT_NEAR(cx[i], x + 256, 0.01);
    ASSERT_NEAR(cy[i], y + 256, 0.01);
  }
}


TEST(Matrix, Inverse1)
{
  OrthancStone::Matrix a, b;

  a.resize(0, 0);
  OrthancStone::LinearAlgebra::InvertMatrix(b, a);
  ASSERT_EQ(0u, b.size1());
  ASSERT_EQ(0u, b.size2());

  a.resize(2, 3);
  ASSERT_THROW(OrthancStone::LinearAlgebra::InvertMatrix(b, a), Orthanc::OrthancException);

  a.resize(1, 1);
  a(0, 0) = 45.0;

  ASSERT_DOUBLE_EQ(45, OrthancStone::LinearAlgebra::ComputeDeterminant(a));
  OrthancStone::LinearAlgebra::InvertMatrix(b, a);
  ASSERT_EQ(1u, b.size1());
  ASSERT_EQ(1u, b.size2());
  ASSERT_DOUBLE_EQ(1.0 / 45.0, b(0, 0));

  a(0, 0) = 0;
  ASSERT_DOUBLE_EQ(0, OrthancStone::LinearAlgebra::ComputeDeterminant(a));
  ASSERT_THROW(OrthancStone::LinearAlgebra::InvertMatrix(b, a), Orthanc::OrthancException);
}


TEST(Matrix, Inverse2)
{
  OrthancStone::Matrix a, b;
  a.resize(2, 2);
  a(0, 0) = 4;
  a(0, 1) = 3;
  a(1, 0) = 3;
  a(1, 1) = 2;

  ASSERT_DOUBLE_EQ(-1, OrthancStone::LinearAlgebra::ComputeDeterminant(a));
  OrthancStone::LinearAlgebra::InvertMatrix(b, a);
  ASSERT_EQ(2u, b.size1());
  ASSERT_EQ(2u, b.size2());

  ASSERT_DOUBLE_EQ(-2, b(0, 0));
  ASSERT_DOUBLE_EQ(3, b(0, 1));
  ASSERT_DOUBLE_EQ(3, b(1, 0));
  ASSERT_DOUBLE_EQ(-4, b(1, 1));

  a(0, 0) = 1;
  a(0, 1) = 2;
  a(1, 0) = 3;
  a(1, 1) = 4;

  ASSERT_DOUBLE_EQ(-2, OrthancStone::LinearAlgebra::ComputeDeterminant(a));
  OrthancStone::LinearAlgebra::InvertMatrix(b, a);

  ASSERT_DOUBLE_EQ(-2, b(0, 0));
  ASSERT_DOUBLE_EQ(1, b(0, 1));
  ASSERT_DOUBLE_EQ(1.5, b(1, 0));
  ASSERT_DOUBLE_EQ(-0.5, b(1, 1));
}


TEST(Matrix, Inverse3)
{
  OrthancStone::Matrix a, b;
  a.resize(3, 3);
  a(0, 0) = 7;
  a(0, 1) = 2;
  a(0, 2) = 1;
  a(1, 0) = 0;
  a(1, 1) = 3;
  a(1, 2) = -1;
  a(2, 0) = -3;
  a(2, 1) = 4;
  a(2, 2) = -2;

  ASSERT_DOUBLE_EQ(1, OrthancStone::LinearAlgebra::ComputeDeterminant(a));
  OrthancStone::LinearAlgebra::InvertMatrix(b, a);
  ASSERT_EQ(3u, b.size1());
  ASSERT_EQ(3u, b.size2());

  ASSERT_DOUBLE_EQ(-2, b(0, 0));
  ASSERT_DOUBLE_EQ(8, b(0, 1));
  ASSERT_DOUBLE_EQ(-5, b(0, 2));
  ASSERT_DOUBLE_EQ(3, b(1, 0));
  ASSERT_DOUBLE_EQ(-11, b(1, 1));
  ASSERT_DOUBLE_EQ(7, b(1, 2));
  ASSERT_DOUBLE_EQ(9, b(2, 0));
  ASSERT_DOUBLE_EQ(-34, b(2, 1));
  ASSERT_DOUBLE_EQ(21, b(2, 2));


  a(0, 0) = 1;
  a(0, 1) = 2;
  a(0, 2) = 2;
  a(1, 0) = 1;
  a(1, 1) = 0;
  a(1, 2) = 1;
  a(2, 0) = 1;
  a(2, 1) = 2;
  a(2, 2) = 1;

  ASSERT_DOUBLE_EQ(2, OrthancStone::LinearAlgebra::ComputeDeterminant(a));
  OrthancStone::LinearAlgebra::InvertMatrix(b, a);
  ASSERT_EQ(3u, b.size1());
  ASSERT_EQ(3u, b.size2());

  ASSERT_DOUBLE_EQ(-1, b(0, 0));
  ASSERT_DOUBLE_EQ(1, b(0, 1));
  ASSERT_DOUBLE_EQ(1, b(0, 2));
  ASSERT_DOUBLE_EQ(0, b(1, 0));
  ASSERT_DOUBLE_EQ(-0.5, b(1, 1));
  ASSERT_DOUBLE_EQ(0.5, b(1, 2));
  ASSERT_DOUBLE_EQ(1, b(2, 0));
  ASSERT_DOUBLE_EQ(0, b(2, 1));
  ASSERT_DOUBLE_EQ(-1, b(2, 2));
}


TEST(Matrix, Inverse4)
{
  OrthancStone::Matrix a, b;
  a.resize(4, 4);
  a(0, 0) = 2;
  a(0, 1) = 1;
  a(0, 2) = 2;
  a(0, 3) = -3;
  a(1, 0) = -2;
  a(1, 1) = 2;
  a(1, 2) = -1;
  a(1, 3) = -1;
  a(2, 0) = 2;
  a(2, 1) = 2;
  a(2, 2) = -3;
  a(2, 3) = -1;
  a(3, 0) = 3;
  a(3, 1) = -2;
  a(3, 2) = -3;
  a(3, 3) = -1;

  OrthancStone::LinearAlgebra::InvertMatrix(b, a);
  ASSERT_EQ(4u, b.size1());
  ASSERT_EQ(4u, b.size2());

  b *= 134.0;  // This is the determinant

  ASSERT_DOUBLE_EQ(8, b(0, 0));
  ASSERT_DOUBLE_EQ(-44, b(0, 1));
  ASSERT_DOUBLE_EQ(30, b(0, 2));
  ASSERT_DOUBLE_EQ(-10, b(0, 3));
  ASSERT_DOUBLE_EQ(2, b(1, 0));
  ASSERT_DOUBLE_EQ(-11, b(1, 1));
  ASSERT_DOUBLE_EQ(41, b(1, 2));
  ASSERT_DOUBLE_EQ(-36, b(1, 3));
  ASSERT_DOUBLE_EQ(16, b(2, 0));
  ASSERT_DOUBLE_EQ(-21, b(2, 1));
  ASSERT_DOUBLE_EQ(-7, b(2, 2));
  ASSERT_DOUBLE_EQ(-20, b(2, 3));
  ASSERT_DOUBLE_EQ(-28, b(3, 0));
  ASSERT_DOUBLE_EQ(-47, b(3, 1));
  ASSERT_DOUBLE_EQ(29, b(3, 2));
  ASSERT_DOUBLE_EQ(-32, b(3, 3));
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
