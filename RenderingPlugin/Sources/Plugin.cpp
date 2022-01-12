/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "../Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h"

#include "../../OrthancStone/Sources/Toolbox/AffineTransform2D.h"
#include "../../OrthancStone/Sources/Toolbox/DicomInstanceParameters.h"

#include <EmbeddedResources.h>

#include <Images/Image.h>
#include <Images/ImageProcessing.h>
#include <Images/NumpyWriter.h>
#include <Logging.h>

#include <boost/math/constants/constants.hpp>


static Orthanc::PixelFormat Convert(OrthancPluginPixelFormat format)
{
  switch (format)
  {
    case OrthancPluginPixelFormat_RGB24:
      return Orthanc::PixelFormat_RGB24;

    case OrthancPluginPixelFormat_Grayscale8:
      return Orthanc::PixelFormat_Grayscale8;

    case OrthancPluginPixelFormat_Grayscale16:
      return Orthanc::PixelFormat_Grayscale16;

    case OrthancPluginPixelFormat_SignedGrayscale16:
      return Orthanc::PixelFormat_SignedGrayscale16;

    default:
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }
}


static void RenderNumpyFrame(OrthancPluginRestOutput* output,
                             const char* url,
                             const OrthancPluginHttpRequest* request)
{
  // TODO: Parameters in GET

  OrthancPlugins::MemoryBuffer tags;
  if (!tags.RestApiGet("/instances/" + std::string(request->groups[0]) + "/tags", false))
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
  }

  Json::Value json;
  tags.ToJson(json);

  Orthanc::DicomMap m;
  m.FromDicomAsJson(json);

  OrthancStone::DicomInstanceParameters parameters(m);
  
  OrthancPlugins::MemoryBuffer dicom;
  dicom.GetDicomInstance(request->groups[0]);

  unsigned int frame = boost::lexical_cast<unsigned int>(request->groups[1]);
  
  OrthancPlugins::OrthancImage image;
  image.DecodeDicomImage(dicom.GetData(), dicom.GetSize(), frame);
  
  Orthanc::ImageAccessor source;
  source.AssignReadOnly(Convert(image.GetPixelFormat()), image.GetWidth(), image.GetHeight(),
                        image.GetPitch(), image.GetBuffer());

  double angle = 0;
  double scaling = 1;
  double offsetX = 0;
  double offsetY = 0;
  bool flipX = false;
  bool flipY = false;

  OrthancStone::AffineTransform2D t;
  t = OrthancStone::AffineTransform2D::Combine(
    OrthancStone::AffineTransform2D::CreateOffset(static_cast<double>(image.GetWidth()) / 2.0 + offsetX,
                                                  static_cast<double>(image.GetHeight()) / 2.0 + offsetY),
    OrthancStone::AffineTransform2D::CreateScaling(scaling, scaling),
    OrthancStone::AffineTransform2D::CreateRotation(angle / 180.0 * boost::math::constants::pi<double>()),
    OrthancStone::AffineTransform2D::CreateOffset(-static_cast<double>(image.GetWidth()) / 2.0,
                                                  -static_cast<double>(image.GetHeight()) / 2.0),
    OrthancStone::AffineTransform2D::CreateFlip(flipX, flipY, image.GetWidth(), image.GetHeight()));
  
  std::unique_ptr<Orthanc::ImageAccessor> modified;
  
  if (source.GetFormat() == Orthanc::PixelFormat_RGB24)
  {
    LOG(WARNING) << "Bilinear interpolation for color images is not implemented yet";

    modified.reset(new Orthanc::Image(source.GetFormat(), source.GetWidth(), source.GetHeight(), false));
    t.Apply(*modified, source, OrthancStone::ImageInterpolation_Nearest, true);
  }
  else
  {
    std::unique_ptr<Orthanc::ImageAccessor> converted(parameters.ConvertToFloat(source));

    assert(converted.get() != NULL);
    modified.reset(new Orthanc::Image(converted->GetFormat(), converted->GetWidth(), converted->GetHeight(), false));
    t.Apply(*modified, *converted, OrthancStone::ImageInterpolation_Bilinear, true);
  }

  assert(modified.get() != NULL);
  
  std::string answer;
  Orthanc::NumpyWriter writer;
  Orthanc::IImageWriter::WriteToMemory(writer, answer, *modified);

  OrthancPluginAnswerBuffer(OrthancPlugins::GetGlobalContext(), output,
                            answer.c_str(), answer.size(), "text/plain");
}


extern "C"
{
  ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext* context)
  {
    OrthancPlugins::SetGlobalContext(context);

#if ORTHANC_FRAMEWORK_VERSION_IS_ABOVE(1, 7, 2)
    Orthanc::Logging::InitializePluginContext(context);
#else
    Orthanc::Logging::Initialize(context);
#endif

    /* Check the version of the Orthanc core */
    if (OrthancPluginCheckVersion(context) == 0)
    {
      char info[1024];
      sprintf(info, "Your version of Orthanc (%s) must be above %d.%d.%d to run this plugin",
              context->orthancVersion,
              ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER);
      OrthancPluginLogError(context, info);
      return -1;
    }

    try
    {
      OrthancPlugins::RegisterRestCallback<RenderNumpyFrame>("/stone/instances/([^/]+)/frames/([0-9]+)/numpy", true);
    }
    catch (...)
    {
      OrthancPlugins::LogError("Exception while initializing the Stone Web viewer plugin");
      return -1;
    }

    return 0;
  }


  ORTHANC_PLUGINS_API void OrthancPluginFinalize()
  {
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetName()
  {
    return PLUGIN_NAME;
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return PLUGIN_VERSION;
  }
}
