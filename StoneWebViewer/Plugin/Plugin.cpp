/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include <OrthancPluginCppWrapper.h>
#include <EmbeddedResources.h>

#include <SystemToolbox.h>
#include <Toolbox.h>

OrthancPluginErrorCode OnChangeCallback(OrthancPluginChangeType changeType,
                                        OrthancPluginResourceType resourceType,
                                        const char* resourceId)
{
  try
  {
    if (changeType == OrthancPluginChangeType_OrthancStarted)
    {
      Json::Value info;
      if (!OrthancPlugins::RestApiGet(info, "/plugins/dicom-web", false))
      {
        throw Orthanc::OrthancException(
          Orthanc::ErrorCode_InternalError,
          "The Stone Web viewer requires the DICOMweb plugin to be installed");
      }

      if (info.type() != Json::objectValue ||
          !info.isMember("ID") ||
          !info.isMember("Version") ||
          info["ID"].type() != Json::stringValue ||
          info["Version"].type() != Json::stringValue ||
          info["ID"].asString() != "dicom-web")
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "The DICOMweb plugin is not properly installed");
      }

      std::string version = info["Version"].asString();
      if (version != "mainline")
      {
        std::vector<std::string> tokens;
        Orthanc::Toolbox::TokenizeString(tokens, version, '.');
        if (tokens.size() != 2)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                          "Bad version of the DICOMweb plugin: " + version);
        }

        int major, minor;
        
        try
        {
          major = boost::lexical_cast<int>(tokens[0]);
          minor = boost::lexical_cast<int>(tokens[1]);
        }
        catch (boost::bad_lexical_cast&)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                          "Bad version of the DICOMweb plugin: " + version);
        }

        if (major <= 0 ||
            (major == 1 && minor <= 1))
        {
          throw Orthanc::OrthancException(
            Orthanc::ErrorCode_InternalError,
            "The Stone Web viewer requires DICOMweb plugin with version >= 1.2, found: " + version);
        }

        if (major <= 0 ||
            (major == 1 && minor == 2))
        {
          /**
           * DICOMweb 1.3 is better than 1.2 for 2 reasons: (1)
           * MONOCHROME1 images are not properly rendered in DICOMweb
           * 1.2, and (2) DICOMweb 1.2 cannot transcode images (this
           * causes issues on JPEG2k images).
           **/
          LOG(WARNING) << "The Stone Web viewer has some incompatibilities "
                       << "with DICOMweb plugin 1.2, consider upgrading the DICOMweb plugin";
        }
      }
    }
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Exception: " << e.What();
    return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
  }

  return OrthancPluginErrorCode_Success;
}
    

template <enum Orthanc::EmbeddedResources::DirectoryResourceId folder>
void ServeEmbeddedFolder(OrthancPluginRestOutput* output,
                         const char* url,
                         const OrthancPluginHttpRequest* request)
{
  OrthancPluginContext* context = OrthancPlugins::GetGlobalContext();

  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context, output, "GET");
  }
  else
  {
    std::string path = "/" + std::string(request->groups[0]);
    const char* mime = Orthanc::EnumerationToString(Orthanc::SystemToolbox::AutodetectMimeType(path));

    std::string s;
    Orthanc::EmbeddedResources::GetDirectoryResource(s, folder, path.c_str());

    const char* resource = s.size() ? s.c_str() : NULL;
    OrthancPluginAnswerBuffer(context, output, resource, s.size(), mime);
  }
}


template <enum Orthanc::EmbeddedResources::FileResourceId file>
void ServeEmbeddedFile(OrthancPluginRestOutput* output,
                       const char* url,
                       const OrthancPluginHttpRequest* request)
{
  OrthancPluginContext* context = OrthancPlugins::GetGlobalContext();

  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context, output, "GET");
  }
  else
  {
    const char* mime = Orthanc::EnumerationToString(Orthanc::SystemToolbox::AutodetectMimeType(url));

    std::string s;
    Orthanc::EmbeddedResources::GetFileResource(s, file);

    const char* resource = s.size() ? s.c_str() : NULL;
    OrthancPluginAnswerBuffer(context, output, resource, s.size(), mime);
  }
}


extern "C"
{
  ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext* context)
  {
    OrthancPlugins::SetGlobalContext(context);
    Orthanc::Logging::InitializePluginContext(context);

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
      std::string explorer;
      Orthanc::EmbeddedResources::GetFileResource(
        explorer, Orthanc::EmbeddedResources::ORTHANC_EXPLORER);
      OrthancPluginExtendOrthancExplorer(OrthancPlugins::GetGlobalContext(), explorer.c_str());
      
      OrthancPlugins::RegisterRestCallback
        <ServeEmbeddedFile<Orthanc::EmbeddedResources::STONE_WEB_VIEWER_WASM> >
        ("/stone-webviewer/StoneWebViewer.wasm", true);
      
      OrthancPlugins::RegisterRestCallback
        <ServeEmbeddedFile<Orthanc::EmbeddedResources::STONE_WEB_VIEWER_JS> >
        ("/stone-webviewer/StoneWebViewer.js", true);
      
      OrthancPlugins::RegisterRestCallback
        <ServeEmbeddedFile<Orthanc::EmbeddedResources::STONE_WRAPPER> >
        ("/stone-webviewer/stone.js", true);
      
      OrthancPlugins::RegisterRestCallback
        <ServeEmbeddedFolder<Orthanc::EmbeddedResources::IMAGES> >
        ("/stone-webviewer/img/(.*)", true);

      OrthancPlugins::RegisterRestCallback
        <ServeEmbeddedFolder<Orthanc::EmbeddedResources::WEB_APPLICATION> >
        ("/stone-webviewer/(.*)", true);

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