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


#include "OrthancPluginConnection.h"
#include "../Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h"

#include "../../OrthancStone/Sources/Toolbox/AffineTransform2D.h"
#include "../../OrthancStone/Sources/Toolbox/DicomInstanceParameters.h"
#include "../../OrthancStone/Sources/Toolbox/DicomStructureSet.h"

#include <EmbeddedResources.h>

#include <Images/Image.h>
#include <Images/ImageProcessing.h>
#include <Images/NumpyWriter.h>
#include <Logging.h>
#include <SerializationToolbox.h>
#include <Toolbox.h>

#include <boost/math/constants/constants.hpp>


class DicomStructureCache : public boost::noncopyable
{
private:
  boost::mutex   mutex_;
  std::string    instanceId_;
  std::unique_ptr<OrthancStone::DicomStructureSet> rtstruct_;

  DicomStructureCache()  // Singleton design pattern
  {
  }
  
public:
  void Invalidate(const std::string& instanceId)
  {
    boost::mutex::scoped_lock lock(mutex_);

    if (instanceId_ == instanceId)
    {
      rtstruct_.reset(NULL);
    }
  }

  static DicomStructureCache& GetSingleton()
  {
    static DicomStructureCache instance;
    return instance;
  }

  class Accessor : public boost::noncopyable
  {
  private:
    boost::mutex::scoped_lock               lock_;
    std::string                             instanceId_;
    const OrthancStone::DicomStructureSet*  rtstruct_;

  public:
    Accessor(DicomStructureCache& that,
             const std::string& instanceId) :
      lock_(that.mutex_),
      instanceId_(instanceId),
      rtstruct_(NULL)
    {
      if (that.instanceId_ == instanceId &&
          that.rtstruct_.get() != NULL)
      {
        rtstruct_ = that.rtstruct_.get();
      }
      else
      {
        try
        {
          OrthancStone::OrthancPluginConnection connection;
          OrthancStone::FullOrthancDataset dataset(connection, "/instances/" + instanceId + "/tags?ignore-length=3006-0050");
          that.rtstruct_.reset(new OrthancStone::DicomStructureSet(dataset));
          that.instanceId_ = instanceId;
          rtstruct_ = that.rtstruct_.get();
        }
        catch (Orthanc::OrthancException&)
        {
        }
      }
    }

    const std::string& GetInstanceId() const
    {
      return instanceId_;
    }

    bool IsValid() const
    {
      return rtstruct_ != NULL;
    }

    const OrthancStone::DicomStructureSet& GetRtStruct() const
    {
      if (IsValid())
      {
        return *rtstruct_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }
  };
};


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


static bool ParseBoolean(const std::string& key,
                         const std::string& value)
{
  bool result;
  
  if (Orthanc::SerializationToolbox::ParseBoolean(result, value))
  {
    return result;
  }
  else
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                    "Bad value for " + key + ": " + value);
  }
}


static double ParseDouble(const std::string& key,
                         const std::string& value)
{
  double result;
  
  if (Orthanc::SerializationToolbox::ParseDouble(result, value))
  {
    return result;
  }
  else
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                    "Bad value for " + key + ": " + value);
  }
}


static void RenderNumpyFrame(OrthancPluginRestOutput* output,
                             const char* url,
                             const OrthancPluginHttpRequest* request)
{
  double angleRadians = 0;
  double scaling = 1;
  double offsetX = 0;
  double offsetY = 0;
  bool flipX = false;
  bool flipY = false;
  bool compress = false;

  for (uint32_t i = 0; i < request->getCount; i++)
  {
    std::string key(request->getKeys[i]);
    std::string value(request->getValues[i]);

    if (key == "angle")
    {
      double angle = ParseDouble(key, value);
      angleRadians = angle / 180.0 * boost::math::constants::pi<double>();
    }
    else if (key == "scaling")
    {
      scaling = ParseDouble(key, value);
    }
    else if (key == "offset-x")
    {
      offsetX = ParseDouble(key, value);
    }
    else if (key == "offset-y")
    {
      offsetY = ParseDouble(key, value);
    }
    else if (key == "flip-x")
    {
      flipX = ParseBoolean(key, value);
    }
    else if (key == "flip-y")
    {
      flipY = ParseBoolean(key, value);
    }
    else if (key == "compress")
    {
      compress = ParseBoolean(key, value);
    }
    else
    {
      LOG(WARNING) << "Unsupported option: " << key;
    }
  }

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

  OrthancStone::AffineTransform2D t;
  t = OrthancStone::AffineTransform2D::Combine(
    OrthancStone::AffineTransform2D::CreateOffset(static_cast<double>(image.GetWidth()) / 2.0 + offsetX,
                                                  static_cast<double>(image.GetHeight()) / 2.0 + offsetY),
    OrthancStone::AffineTransform2D::CreateScaling(scaling, scaling),
    OrthancStone::AffineTransform2D::CreateRotation(angleRadians),
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
  writer.SetCompressed(compress);
  Orthanc::IImageWriter::WriteToMemory(writer, answer, *modified);
  
  OrthancPluginAnswerBuffer(OrthancPlugins::GetGlobalContext(), output,
                            answer.c_str(), answer.size(), "application/octet-stream");
}


static bool IsRtStruct(const std::string& instanceId)
{
  static const char* SOP_CLASS_UID = "0008,0016";
  static const char* RT_STRUCT_IOD = "1.2.840.10008.5.1.4.1.1.481.3";
      
  std::string s;
  if (OrthancPlugins::RestApiGetString(s, "/instances/" + instanceId + "/content/" + SOP_CLASS_UID, false) &&
      !s.empty())
  {
    if (s[s.size() - 1] == '\0')  // Deal with DICOM padding
    {
      s.resize(s.size() - 1);
    }
    
    return s == RT_STRUCT_IOD;
  }
  else
  {
    return false;
  }
}


static void ListRtStruct(OrthancPluginRestOutput* output,
                         const char* url,
                         const OrthancPluginHttpRequest* request)
{
  // This is a quick version of "/tools/find" on "SOPClassUID" (the
  // latter would load all the DICOM files from disk)

  static const char* INSTANCES = "Instances";
  
  Json::Value series;
  OrthancPlugins::RestApiGet(series, "/series?expand", false);

  if (series.type() != Json::arrayValue)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }

  Json::Value answer = Json::arrayValue;

  for (Json::Value::ArrayIndex i = 0; i < series.size(); i++)
  {
    if (series[i].type() != Json::objectValue ||
        !series[i].isMember(INSTANCES) ||
        series[i][INSTANCES].type() != Json::arrayValue)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    
    const Json::Value& instances = series[i][INSTANCES];

    for (Json::Value::ArrayIndex j = 0; j < instances.size(); j++)
    {
      if (instances[j].type() != Json::stringValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
    
    if (instances.size() > 0 &&
        IsRtStruct(instances[0].asString()))
    {
      for (Json::Value::ArrayIndex j = 0; j < instances.size(); j++)
      {
        answer.append(instances[j].asString());
      }
    }
  }

  std::string s = answer.toStyledString();
  OrthancPluginAnswerBuffer(OrthancPlugins::GetGlobalContext(), output, s.c_str(), s.size(), "application/json");
}


static void GetRtStruct(OrthancPluginRestOutput* output,
                        const char* url,
                        const OrthancPluginHttpRequest* request)
{
  static const char* STRUCTURES = "Structures";
  static const char* INSTANCES = "Instances";
  
  DicomStructureCache::Accessor accessor(DicomStructureCache::GetSingleton(), request->groups[0]);

  if (!accessor.IsValid())
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
  }

  Json::Value answer;
  answer[STRUCTURES] = Json::arrayValue;

  for (size_t i = 0; i < accessor.GetRtStruct().GetStructuresCount(); i++)
  {
    Json::Value color = Json::arrayValue;
    color.append(accessor.GetRtStruct().GetStructureColor(i).GetRed());
    color.append(accessor.GetRtStruct().GetStructureColor(i).GetGreen());
    color.append(accessor.GetRtStruct().GetStructureColor(i).GetBlue());
    
    Json::Value structure;
    structure["Name"] = accessor.GetRtStruct().GetStructureName(i);
    structure["Interpretation"] = accessor.GetRtStruct().GetStructureInterpretation(i);
    structure["Color"] = color;
    
    answer[STRUCTURES].append(structure);
  }

  std::set<std::string> sopInstanceUids;
  accessor.GetRtStruct().GetReferencedInstances(sopInstanceUids);

  answer[INSTANCES] = Json::arrayValue;
  for (std::set<std::string>::const_iterator it = sopInstanceUids.begin(); it != sopInstanceUids.end(); ++it)
  {
    OrthancPlugins::OrthancString s;
    s.Assign(OrthancPluginLookupInstance(OrthancPlugins::GetGlobalContext(), it->c_str()));

    std::string t;
    s.ToString(t);

    answer[INSTANCES].append(t);
  }  

  std::string s = answer.toStyledString();
  OrthancPluginAnswerBuffer(OrthancPlugins::GetGlobalContext(), output, s.c_str(), s.size(), "application/json");
}


static void RenderRtStruct(OrthancPluginRestOutput* output,
                           const char* url,
                           const OrthancPluginHttpRequest* request)
{
  static const char* MAIN_DICOM_TAGS = "MainDicomTags";
  
  DicomStructureCache::Accessor accessor(DicomStructureCache::GetSingleton(), request->groups[0]);

  if (!accessor.IsValid())
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
  }

  std::string structureName;
  std::string instanceId;

  for (uint32_t i = 0; i < request->getCount; i++)
  {
    std::string key(request->getKeys[i]);
    std::string value(request->getValues[i]);

    if (key == "structure")
    {
      structureName = value;
    }
    else if (key == "instance")
    {
      instanceId = value;
    }
    else
    {
      LOG(WARNING) << "Unsupported option: " << key;
    }
  }

  if (structureName.empty())
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol,
                                    "Missing option \"structure\" to provide the structure name");
  }

  if (instanceId.empty())
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol,
                                    "Missing option \"instance\" to provide the Orthanc identifier of the instance of interest");
  }

  size_t structureIndex;
  bool found = false;
  for (size_t i = 0; i < accessor.GetRtStruct().GetStructuresCount(); i++)
  {
    if (accessor.GetRtStruct().GetStructureName(i) == structureName)
    {
      structureIndex = i;
      found = true;
      break;
    }
  }

  if (!found)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem,
                                    "Unknown structure name: " + structureName);
  }

  Json::Value instance;
  if (!OrthancPlugins::RestApiGet(instance, "/instances/" + instanceId, false))
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem,
                                    "Unknown instance with Orthanc ID: " + instanceId);
  }

  if (instance.type() != Json::objectValue ||
      !instance.isMember(MAIN_DICOM_TAGS) ||
      instance[MAIN_DICOM_TAGS].type() != Json::objectValue)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }

  std::string sopInstanceUid = Orthanc::SerializationToolbox::ReadString(instance[MAIN_DICOM_TAGS], "SOPInstanceUID");
  

  std::list< std::vector<OrthancStone::Vector> > p;
  accessor.GetRtStruct().GetStructurePoints(p, structureIndex, sopInstanceUid);

  std::string s = "hello";
  OrthancPluginAnswerBuffer(OrthancPlugins::GetGlobalContext(), output, s.c_str(), s.size(), "application/json");
}


OrthancPluginErrorCode OnChangeCallback(OrthancPluginChangeType changeType,
                                        OrthancPluginResourceType resourceType,
                                        const char* resourceId)
{
  switch (changeType)
  {
    case OrthancPluginChangeType_Deleted:
      if (resourceType == OrthancPluginResourceType_Instance)
      {
        DicomStructureCache::GetSingleton().Invalidate(resourceId);
      }
      
      break;

    default:
      break;
  }

  return OrthancPluginErrorCode_Success;
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
      OrthancPlugins::RegisterRestCallback<ListRtStruct>("/stone/rt-struct", true);
      OrthancPlugins::RegisterRestCallback<GetRtStruct>("/stone/rt-struct/([^/]+)/info", true);
      OrthancPlugins::RegisterRestCallback<RenderRtStruct>("/stone/rt-struct/([^/]+)/numpy", true);
      OrthancPluginRegisterOnChangeCallback(context, OnChangeCallback);
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
