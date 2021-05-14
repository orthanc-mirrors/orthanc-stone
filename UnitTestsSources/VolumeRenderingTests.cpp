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
#include "../OrthancStone/Sources/Scene2D/ColorTextureSceneLayer.h"
#include "../OrthancStone/Sources/Volumes/DicomVolumeImageMPRSlicer.h"
#include "../OrthancStone/Sources/Volumes/DicomVolumeImageReslicer.h"

#include <Images/ImageProcessing.h>
#include <Images/ImageTraits.h>
#include <OrthancException.h>

#include <gtest/gtest.h>


static float GetPixelValue(const Orthanc::ImageAccessor& image,
                           unsigned int x,
                           unsigned int y)
{
  switch (image.GetFormat())
  {
    case Orthanc::PixelFormat_Grayscale8:
      return Orthanc::ImageTraits<Orthanc::PixelFormat_Grayscale8>::GetFloatPixel(image, x, y);
    
    case Orthanc::PixelFormat_Float32:
      return Orthanc::ImageTraits<Orthanc::PixelFormat_Float32>::GetFloatPixel(image, x, y);
    
    case Orthanc::PixelFormat_RGB24:
    {
      Orthanc::PixelTraits<Orthanc::PixelFormat_RGB24>::PixelType pixel;
      Orthanc::ImageTraits<Orthanc::PixelFormat_RGB24>::GetPixel(pixel, image, x, y);
      return pixel.red_;
    }
    
    case Orthanc::PixelFormat_BGRA32:
    {
      Orthanc::PixelTraits<Orthanc::PixelFormat_BGRA32>::PixelType pixel;
      Orthanc::ImageTraits<Orthanc::PixelFormat_BGRA32>::GetPixel(pixel, image, x, y);
      return pixel.red_;
    }
    
    default:
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }
}


static bool IsConstImage(float value,
                         const Orthanc::ImageAccessor& image)
{
  for (unsigned int y = 0; y < image.GetHeight(); y++)
  {
    for (unsigned int x = 0; x < image.GetWidth(); x++)
    {
      if (!OrthancStone::LinearAlgebra::IsNear(value, GetPixelValue(image, x, y)))
      {
        return false;
      }
    }
  }

  return true;
}
  

static bool IsConstRegion(float value,
                          const Orthanc::ImageAccessor& image,
                          unsigned int x,
                          unsigned int y,
                          unsigned int width,
                          unsigned int height)
{
  Orthanc::ImageAccessor region;
  image.GetRegion(region, x, y, width, height);
  return IsConstImage(value, region);
}


static void Assign3x3Pattern(Orthanc::ImageAccessor& image)
{
  if (image.GetFormat() == Orthanc::PixelFormat_Grayscale8 &&
      image.GetWidth() == 3 &&
      image.GetHeight() == 3)
  {
    unsigned int v = 0;
    for (unsigned int y = 0; y < image.GetHeight(); y++)
    {
      uint8_t *p = reinterpret_cast<uint8_t*>(image.GetRow(y));
      for (unsigned int x = 0; x < image.GetWidth(); x++, p++)
      {
        *p = v;
        v += 25;
      }
    }
  }
  else
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }
}
  

TEST(VolumeRendering, Axial)
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
    Assign3x3Pattern(writer.GetAccessor());
  }

  OrthancStone::Vector v = volume->GetGeometry().GetVoxelDimensions(OrthancStone::VolumeProjection_Axial);
  ASSERT_FLOAT_EQ(1, v[0]);
  ASSERT_FLOAT_EQ(1, v[1]);
  ASSERT_FLOAT_EQ(1, v[2]);
  
  OrthancStone::CoordinateSystem3D viewpoint;

  for (unsigned int mode = 0; mode < 2; mode++)
  {
    std::unique_ptr<OrthancStone::IVolumeSlicer> slicer;

    if (mode == 1)
    {
      slicer.reset(new OrthancStone::DicomVolumeImageReslicer(volume));
    }
    else
    {    
      slicer.reset(new OrthancStone::DicomVolumeImageMPRSlicer(volume));
    }
  
    std::unique_ptr<OrthancStone::IVolumeSlicer::IExtractedSlice> slice(slicer->ExtractSlice(viewpoint));
    ASSERT_TRUE(slice->IsValid());

    OrthancStone::CopyStyleConfigurator configurator;
    std::unique_ptr<OrthancStone::ISceneLayer> layer(slice->CreateSceneLayer(&configurator, viewpoint));

    ASSERT_EQ(OrthancStone::ISceneLayer::Type_FloatTexture, layer->GetType());

    OrthancStone::Extent2D box;
    layer->GetBoundingBox(box);
    ASSERT_FLOAT_EQ(-1.0f, box.GetX1());
    ASSERT_FLOAT_EQ(-1.0f, box.GetY1());
    ASSERT_FLOAT_EQ(2.0f, box.GetX2());
    ASSERT_FLOAT_EQ(2.0f, box.GetY2());
    
    {
      const Orthanc::ImageAccessor& texture = dynamic_cast<OrthancStone::TextureBaseSceneLayer&>(*layer).GetTexture();
      ASSERT_EQ(3u, texture.GetWidth());
      ASSERT_EQ(3u, texture.GetHeight());
      ASSERT_FLOAT_EQ(0, GetPixelValue(texture, 0, 0));
      ASSERT_FLOAT_EQ(25, GetPixelValue(texture, 1, 0));
      ASSERT_FLOAT_EQ(50, GetPixelValue(texture, 2, 0));
      ASSERT_FLOAT_EQ(75, GetPixelValue(texture, 0, 1));
      ASSERT_FLOAT_EQ(100, GetPixelValue(texture, 1, 1));
      ASSERT_FLOAT_EQ(125, GetPixelValue(texture, 2, 1));
      ASSERT_FLOAT_EQ(150, GetPixelValue(texture, 0, 2));
      ASSERT_FLOAT_EQ(175, GetPixelValue(texture, 1, 2));
      ASSERT_FLOAT_EQ(200, GetPixelValue(texture, 2, 2));
    }

    OrthancStone::Scene2D scene;  // Scene is initialized with the identity viewpoint
    scene.SetLayer(0, layer.release());

    OrthancStone::CairoCompositor compositor(5, 5);
    compositor.Refresh(scene);

    Orthanc::ImageAccessor rendered;
    compositor.GetCanvas().GetReadOnlyAccessor(rendered);

    ASSERT_EQ(5u, rendered.GetWidth());
    ASSERT_EQ(5u, rendered.GetHeight());
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 2, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 3, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 4, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 1));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 1));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 2, 1));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 3, 1));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 4, 1));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 2));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 2));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 2, 2));
    ASSERT_FLOAT_EQ(25, GetPixelValue(rendered, 3, 2));
    ASSERT_FLOAT_EQ(50, GetPixelValue(rendered, 4, 2));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 3));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 3));
    ASSERT_FLOAT_EQ(75, GetPixelValue(rendered, 2, 3));
    ASSERT_FLOAT_EQ(100, GetPixelValue(rendered, 3, 3));
    ASSERT_FLOAT_EQ(125, GetPixelValue(rendered, 4, 3));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 4));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 4));
    ASSERT_FLOAT_EQ(150, GetPixelValue(rendered, 2, 4));
    ASSERT_FLOAT_EQ(175, GetPixelValue(rendered, 3, 4));
    ASSERT_FLOAT_EQ(200, GetPixelValue(rendered, 4, 4));
  }
}



TEST(VolumeRendering, TextureCorners)
{
  // The origin of a 2D texture is the coordinate of the BORDER of the
  // top-left pixel, *not* the center of the top-left pixel (as in
  // DICOM 3D convention)
  
  Orthanc::Image pixel(Orthanc::PixelFormat_RGB24, 1, 1, false);
  Orthanc::ImageProcessing::Set(pixel, 255, 0, 0, 255);
  
  {    
    std::unique_ptr<OrthancStone::ColorTextureSceneLayer> layer(new OrthancStone::ColorTextureSceneLayer(pixel));
    layer->SetOrigin(0, 0);

    OrthancStone::Scene2D scene;
    scene.SetLayer(0, layer.release());

    OrthancStone::CairoCompositor compositor(2, 2);
    compositor.Refresh(scene);
    
    Orthanc::ImageAccessor rendered;
    compositor.GetCanvas().GetReadOnlyAccessor(rendered);
    
    ASSERT_EQ(2u, rendered.GetWidth());
    ASSERT_EQ(2u, rendered.GetHeight());
  
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 1));
    ASSERT_FLOAT_EQ(255, GetPixelValue(rendered, 1, 1));
  }
  
  {    
    std::unique_ptr<OrthancStone::ColorTextureSceneLayer> layer(new OrthancStone::ColorTextureSceneLayer(pixel));
    layer->SetOrigin(-0.01, 0);

    OrthancStone::Scene2D scene;
    scene.SetLayer(0, layer.release());

    OrthancStone::CairoCompositor compositor(2, 2);
    compositor.Refresh(scene);
    
    Orthanc::ImageAccessor rendered;
    compositor.GetCanvas().GetReadOnlyAccessor(rendered);
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 0));
    ASSERT_FLOAT_EQ(255, GetPixelValue(rendered, 0, 1));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 1));
  }
  
  {    
    std::unique_ptr<OrthancStone::ColorTextureSceneLayer> layer(new OrthancStone::ColorTextureSceneLayer(pixel));
    layer->SetOrigin(-0.01, -0.01);

    OrthancStone::Scene2D scene;
    scene.SetLayer(0, layer.release());

    OrthancStone::CairoCompositor compositor(2, 2);
    compositor.Refresh(scene);
    
    Orthanc::ImageAccessor rendered;
    compositor.GetCanvas().GetReadOnlyAccessor(rendered);
    ASSERT_FLOAT_EQ(255, GetPixelValue(rendered, 0, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 1));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 1));
  }
  
  {    
    std::unique_ptr<OrthancStone::ColorTextureSceneLayer> layer(new OrthancStone::ColorTextureSceneLayer(pixel));
    layer->SetOrigin(0, -0.01);

    OrthancStone::Scene2D scene;
    scene.SetLayer(0, layer.release());

    OrthancStone::CairoCompositor compositor(2, 2);
    compositor.Refresh(scene);
    
    Orthanc::ImageAccessor rendered;
    compositor.GetCanvas().GetReadOnlyAccessor(rendered);
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 0));
    ASSERT_FLOAT_EQ(255, GetPixelValue(rendered, 1, 0));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 0, 1));
    ASSERT_FLOAT_EQ(0, GetPixelValue(rendered, 1, 1));
  }
}
