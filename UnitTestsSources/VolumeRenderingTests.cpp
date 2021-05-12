/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2021 Osimis S.A., Belgium
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


#include "../OrthancStone/Sources/Scene2D/CairoCompositor.h"
#include "../OrthancStone/Sources/Scene2D/CopyStyleConfigurator.h"
#include "../OrthancStone/Sources/Volumes/DicomVolumeImageMPRSlicer.h"
#include "../OrthancStone/Sources/Volumes/DicomVolumeImageReslicer.h"

#include <Images/ImageProcessing.h>
#include <Images/ImageTraits.h>

#include <gtest/gtest.h>

TEST(VolumeRendering, Basic)
{
  Orthanc::DicomMap dicom;
  dicom.SetValue(Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, "study", false);
  dicom.SetValue(Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, "series", false);
  dicom.SetValue(Orthanc::DICOM_TAG_SOP_INSTANCE_UID, "sop", false);
  
  OrthancStone::CoordinateSystem3D axial(OrthancStone::LinearAlgebra::CreateVector(-0.5, -0.5, 0),
                                         OrthancStone::LinearAlgebra::CreateVector(1, 0, 0),
                                         OrthancStone::LinearAlgebra::CreateVector(0, 1, 0));
  
  OrthancStone::VolumeImageGeometry geometry;
  geometry.SetSizeInVoxels(3, 3, 1);
  geometry.SetAxialGeometry(axial);
  
  boost::shared_ptr<OrthancStone::DicomVolumeImage> volume(new OrthancStone::DicomVolumeImage);
  volume->Initialize(geometry, Orthanc::PixelFormat_Grayscale8, false);
  volume->SetDicomParameters(OrthancStone::DicomInstanceParameters(dicom));

  {
    OrthancStone::ImageBuffer3D::SliceWriter writer(volume->GetPixelData(), OrthancStone::VolumeProjection_Axial, 0);
    unsigned int v = 0;
    for (unsigned int y = 0; y < writer.GetAccessor().GetHeight(); y++)
    {
      uint8_t *p = reinterpret_cast<uint8_t*>(writer.GetAccessor().GetRow(y));
      for (unsigned int x = 0; x < writer.GetAccessor().GetWidth(); x++, p++)
      {
        *p = v;
        v += 25;
      }
    }
  }

  OrthancStone::CoordinateSystem3D viewpoint;

  //OrthancStone::DicomVolumeImageReslicer slicer(volume); slicer.SetInterpolation(OrthancStone::ImageInterpolation_Nearest);
  OrthancStone::DicomVolumeImageMPRSlicer slicer(volume);
  std::unique_ptr<OrthancStone::IVolumeSlicer::IExtractedSlice> slice(slicer.ExtractSlice(viewpoint));
  ASSERT_TRUE(slice->IsValid());

  OrthancStone::CopyStyleConfigurator configurator;
  std::unique_ptr<OrthancStone::ISceneLayer> layer(slice->CreateSceneLayer(&configurator, viewpoint));

  {
    const Orthanc::ImageAccessor& a = dynamic_cast<OrthancStone::TextureBaseSceneLayer&>(*layer).GetTexture();
    Orthanc::Image i(Orthanc::PixelFormat_Grayscale8, a.GetWidth(), a.GetHeight(), false);
    Orthanc::ImageProcessing::Convert(i, a);

    ASSERT_EQ(3u, i.GetWidth());
    ASSERT_EQ(3u, i.GetHeight());

    ASSERT_FLOAT_EQ(0, Orthanc::ImageTraits<Orthanc::PixelFormat_Grayscale8>::GetFloatPixel(i, 0, 0));
    ASSERT_FLOAT_EQ(25, Orthanc::ImageTraits<Orthanc::PixelFormat_Grayscale8>::GetFloatPixel(i, 1, 0));
    ASSERT_FLOAT_EQ(50, Orthanc::ImageTraits<Orthanc::PixelFormat_Grayscale8>::GetFloatPixel(i, 2, 0));
    ASSERT_FLOAT_EQ(75, Orthanc::ImageTraits<Orthanc::PixelFormat_Grayscale8>::GetFloatPixel(i, 0, 1));
    ASSERT_FLOAT_EQ(100, Orthanc::ImageTraits<Orthanc::PixelFormat_Grayscale8>::GetFloatPixel(i, 1, 1));
    ASSERT_FLOAT_EQ(125, Orthanc::ImageTraits<Orthanc::PixelFormat_Grayscale8>::GetFloatPixel(i, 2, 1));
    ASSERT_FLOAT_EQ(150, Orthanc::ImageTraits<Orthanc::PixelFormat_Grayscale8>::GetFloatPixel(i, 0, 2));
    ASSERT_FLOAT_EQ(175, Orthanc::ImageTraits<Orthanc::PixelFormat_Grayscale8>::GetFloatPixel(i, 1, 2));
    ASSERT_FLOAT_EQ(200, Orthanc::ImageTraits<Orthanc::PixelFormat_Grayscale8>::GetFloatPixel(i, 2, 2));
  }

  OrthancStone::Scene2D scene;  // Scene is initialized with the identity viewpoint
  scene.SetLayer(0, layer.release());

  OrthancStone::CairoCompositor compositor(5, 5);
  compositor.Refresh(scene);

  Orthanc::ImageAccessor i;
  compositor.GetCanvas().GetReadOnlyAccessor(i);

  Orthanc::Image j(Orthanc::PixelFormat_RGB24, i.GetWidth(), i.GetHeight(), false);
  Orthanc::ImageProcessing::Convert(j, i);

  ASSERT_EQ(5u, j.GetWidth());
  ASSERT_EQ(5u, j.GetHeight());
  Orthanc::PixelTraits<Orthanc::PixelFormat_RGB24>::PixelType pixel;
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 0, 0);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 1, 0);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 2, 0);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 3, 0);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 4, 0);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 0, 1);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 1, 1);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 2, 1);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 3, 1);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 4, 1);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 0, 2);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 1, 2);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 2, 2);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 3, 2);  ASSERT_EQ(25, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 4, 2);  ASSERT_EQ(50, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 0, 3);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 1, 3);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 2, 3);  ASSERT_EQ(75, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 3, 3);  ASSERT_EQ(100, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 4, 3);  ASSERT_EQ(125, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 0, 4);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 1, 4);  ASSERT_EQ(0, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 2, 4);  ASSERT_EQ(150, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 3, 4);  ASSERT_EQ(175, pixel.red_);
  Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, j, 4, 4);  ASSERT_EQ(200, pixel.red_);
}

