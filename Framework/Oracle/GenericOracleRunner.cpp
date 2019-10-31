/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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


#include "GenericOracleRunner.h"

#if !defined(ORTHANC_ENABLE_DCMTK)
#  error The macro ORTHANC_ENABLE_DCMTK must be defined
#endif

#include "CustomOracleCommand.h"
#include "GetOrthancImageCommand.h"
#include "GetOrthancWebViewerJpegCommand.h"
#include "HttpCommand.h"
#include "OracleCommandExceptionMessage.h"
#include "OrthancRestApiCommand.h"
#include "ReadFileCommand.h"

#if ORTHANC_ENABLE_DCMTK == 1
#  include "ParseDicomFileCommand.h"
#  include <dcmtk/dcmdata/dcdeftag.h>
#endif

#include <Core/Compression/GzipCompressor.h>
#include <Core/HttpClient.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>
#include <Core/SystemToolbox.h>

#include <boost/filesystem.hpp>


namespace OrthancStone
{
  static void CopyHttpHeaders(Orthanc::HttpClient& client,
                              const Orthanc::HttpClient::HttpHeaders& headers)
  {
    for (Orthanc::HttpClient::HttpHeaders::const_iterator
           it = headers.begin(); it != headers.end(); it++ )
    {
      client.AddHeader(it->first, it->second);
    }
  }


  static void DecodeAnswer(std::string& answer,
                           const Orthanc::HttpClient::HttpHeaders& headers)
  {
    Orthanc::HttpCompression contentEncoding = Orthanc::HttpCompression_None;

    for (Orthanc::HttpClient::HttpHeaders::const_iterator it = headers.begin(); 
         it != headers.end(); ++it)
    {
      std::string s;
      Orthanc::Toolbox::ToLowerCase(s, it->first);

      if (s == "content-encoding")
      {
        if (it->second == "gzip")
        {
          contentEncoding = Orthanc::HttpCompression_Gzip;
        }
        else 
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol,
                                          "Unsupported HTTP Content-Encoding: " + it->second);
        }

        break;
      }
    }

    if (contentEncoding == Orthanc::HttpCompression_Gzip)
    {
      std::string compressed;
      answer.swap(compressed);
          
      Orthanc::GzipCompressor compressor;
      compressor.Uncompress(answer, compressed.c_str(), compressed.size());

      LOG(INFO) << "Uncompressing gzip Encoding: from " << compressed.size()
                << " to " << answer.size() << " bytes";
    }
  }


  static IMessage* Execute(const HttpCommand& command)
  {
    Orthanc::HttpClient client;
    client.SetUrl(command.GetUrl());
    client.SetMethod(command.GetMethod());
    client.SetTimeout(command.GetTimeout());

    CopyHttpHeaders(client, command.GetHttpHeaders());

    if (command.HasCredentials())
    {
      client.SetCredentials(command.GetUsername().c_str(), command.GetPassword().c_str());
    }

    if (command.GetMethod() == Orthanc::HttpMethod_Post ||
        command.GetMethod() == Orthanc::HttpMethod_Put)
    {
      client.SetBody(command.GetBody());
    }

    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;
    client.ApplyAndThrowException(answer, answerHeaders);

    DecodeAnswer(answer, answerHeaders);

    return new HttpCommand::SuccessMessage(command, answerHeaders, answer);
  }


  static IMessage* Execute(const Orthanc::WebServiceParameters& orthanc,
                           const OrthancRestApiCommand& command)
  {
    Orthanc::HttpClient client(orthanc, command.GetUri());
    client.SetMethod(command.GetMethod());
    client.SetTimeout(command.GetTimeout());

    CopyHttpHeaders(client, command.GetHttpHeaders());

    if (command.GetMethod() == Orthanc::HttpMethod_Post ||
        command.GetMethod() == Orthanc::HttpMethod_Put)
    {
      client.SetBody(command.GetBody());
    }

    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;
    client.ApplyAndThrowException(answer, answerHeaders);

    DecodeAnswer(answer, answerHeaders);

    return new OrthancRestApiCommand::SuccessMessage(command, answerHeaders, answer);
  }


  static IMessage* Execute(const Orthanc::WebServiceParameters& orthanc,
                           const GetOrthancImageCommand& command)
  {
    Orthanc::HttpClient client(orthanc, command.GetUri());
    client.SetTimeout(command.GetTimeout());

    CopyHttpHeaders(client, command.GetHttpHeaders());
    
    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;
    client.ApplyAndThrowException(answer, answerHeaders);

    DecodeAnswer(answer, answerHeaders);

    return command.ProcessHttpAnswer(answer, answerHeaders);
  }


  static IMessage* Execute(const Orthanc::WebServiceParameters& orthanc,
                           const GetOrthancWebViewerJpegCommand& command)
  {
    Orthanc::HttpClient client(orthanc, command.GetUri());
    client.SetTimeout(command.GetTimeout());

    CopyHttpHeaders(client, command.GetHttpHeaders());

    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;
    client.ApplyAndThrowException(answer, answerHeaders);

    DecodeAnswer(answer, answerHeaders);

    return command.ProcessHttpAnswer(answer);
  }


  static std::string GetPath(const std::string& root,
                             const std::string& file)
  {
    boost::filesystem::path a(root);
    boost::filesystem::path b(file);

    boost::filesystem::path c;
    if (b.is_absolute())
    {
      c = b;
    }
    else
    {
      c = a / b;
    }

    LOG(TRACE) << "Oracle reading file: " << c.string();
    return c.string();
  }


  static IMessage* Execute(const std::string& root,
                           const ReadFileCommand& command)
  {
    std::string path = GetPath(root, command.GetPath());

    std::string content;
    Orthanc::SystemToolbox::ReadFile(content, path, true /* log */);

    return new ReadFileCommand::SuccessMessage(command, content);
  }


#if ORTHANC_ENABLE_DCMTK == 1
  static IMessage* Execute(const std::string& root,
                           const ParseDicomFileCommand& command)
  {
    std::string path = GetPath(root, command.GetPath());

    DcmFileFormat f;
    bool ok;

    if (command.IsPixelDataIncluded())
    {
      ok = f.loadFile(path.c_str()).good();
    }
    else
    {
#if DCMTK_VERSION_NUMBER >= 362
      // NB : We could stop at (0x3007, 0x0000) instead of
      // DCM_PixelData, cf. the Orthanc::DICOM_TAG_* constants

      static const DcmTagKey STOP = DCM_PixelData;
      //static const DcmTagKey STOP(0x3007, 0x0000);

      ok = f.loadFileUntilTag(path.c_str(), EXS_Unknown, EGL_noChange,
                              DCM_MaxReadLength, ERM_autoDetect, STOP).good();
#else
      // The primitive "loadFileUntilTag" was introduced in DCMTK 3.6.2
      ok = f.loadFile(path.c_str()).good();
#endif
    }

    if (ok)
    {
      return new ParseDicomFileCommand::SuccessMessage(command, f);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                      "Cannot parse file: " + path);
    }
  }
#endif


  IMessage* GenericOracleRunner::Run(IOracleCommand& command)
  {
    try
    {
      switch (command.GetType())
      {
        case IOracleCommand::Type_Sleep:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType,
                                          "Sleep command cannot be executed by the runner");

        case IOracleCommand::Type_Http:
          return Execute(dynamic_cast<const HttpCommand&>(command));

        case IOracleCommand::Type_OrthancRestApi:
          return Execute(orthanc_, dynamic_cast<const OrthancRestApiCommand&>(command));

        case IOracleCommand::Type_GetOrthancImage:
          return Execute(orthanc_, dynamic_cast<const GetOrthancImageCommand&>(command));

        case IOracleCommand::Type_GetOrthancWebViewerJpeg:
          return Execute(orthanc_, dynamic_cast<const GetOrthancWebViewerJpegCommand&>(command));

        case IOracleCommand::Type_Custom:
          return dynamic_cast<CustomOracleCommand&>(command).Execute(*this);

        case IOracleCommand::Type_ReadFile:
          return Execute(rootDirectory_, dynamic_cast<const ReadFileCommand&>(command));

        case IOracleCommand::Type_ParseDicomFile:
#if ORTHANC_ENABLE_DCMTK == 1
          return Execute(rootDirectory_, dynamic_cast<const ParseDicomFileCommand&>(command));
#else
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented,
                                          "DCMTK must be enabled to parse DICOM files");
#endif

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "Exception within the oracle: " << e.What();
      return new OracleCommandExceptionMessage(command, e);
    }
    catch (...)
    {
      LOG(ERROR) << "Threaded exception within the oracle";
      return new OracleCommandExceptionMessage(command, Orthanc::ErrorCode_InternalError);
    }
  }
}
