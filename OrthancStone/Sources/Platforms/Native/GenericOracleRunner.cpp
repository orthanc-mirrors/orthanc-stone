/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include "GenericOracleRunner.h"

#if !defined(ORTHANC_ENABLE_DCMTK)
#  error The macro ORTHANC_ENABLE_DCMTK must be defined
#endif

#include "../../Oracle/GetOrthancImageCommand.h"
#include "../../Oracle/GetOrthancWebViewerJpegCommand.h"
#include "../../Oracle/HttpCommand.h"
#include "../../Oracle/OracleCommandExceptionMessage.h"
#include "../../Oracle/OrthancRestApiCommand.h"
#include "../../Oracle/ParseDicomFromFileCommand.h"
#include "../../Oracle/ParseDicomFromWadoCommand.h"
#include "../../Oracle/ReadFileCommand.h"

#if ORTHANC_ENABLE_DCMTK == 1
#  include "../../Oracle/ParseDicomSuccessMessage.h"
#  include <dcmtk/dcmdata/dcdeftag.h>
#  include <dcmtk/dcmdata/dcfilefo.h>
#endif

#include <Compression/GzipCompressor.h>
#include <HttpClient.h>
#include <Logging.h>
#include <OrthancException.h>
#include <SystemToolbox.h>
#include <Toolbox.h>

#include <boost/filesystem.hpp>


static std::string GetFileSystemCacheKey(const std::string& path)
{
  return "file|" + path;
}


static std::string GetSopInstanceUidCacheKey(const std::string& uid)
{
  return "sop|" + uid;
}


namespace OrthancStone
{
  static void CopyHttpHeaders(Orthanc::HttpClient& client,
                              const Orthanc::HttpClient::HttpHeaders& headers)
  {
    for (Orthanc::HttpClient::HttpHeaders::const_iterator
           it = headers.begin(); it != headers.end(); ++it )
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


  static void RunHttpCommand(std::string& answer,
                             Orthanc::HttpClient::HttpHeaders& answerHeaders,
                             const HttpCommand& command)
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
      client.SetExternalBody(command.GetBody());
    }

    client.ApplyAndThrowException(answer, answerHeaders);
    client.ClearBody();

    DecodeAnswer(answer, answerHeaders);
  }


  static void RunInternal(OracleCallback& callback,
                          const HttpCommand& command)
  {
    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;
    RunHttpCommand(answer, answerHeaders, command);

    std::unique_ptr<HttpCommand::SuccessMessage> message(new HttpCommand::SuccessMessage(command, answerHeaders));
    message->SwapAnswer(answer);
    
    callback.NotifySuccess(message.release());
  }

  
  static void RunOrthancRestApiCommand(std::string& answer,
                                       Orthanc::HttpClient::HttpHeaders& answerHeaders,
                                       const Orthanc::WebServiceParameters& orthanc,
                                       const OrthancRestApiCommand& command)
  {
    Orthanc::HttpClient client(orthanc, command.GetUri());
    client.SetRedirectionFollowed(false);
    client.SetMethod(command.GetMethod());
    client.SetTimeout(command.GetTimeout());

    CopyHttpHeaders(client, command.GetHttpHeaders());

    if (command.GetMethod() == Orthanc::HttpMethod_Post ||
        command.GetMethod() == Orthanc::HttpMethod_Put)
    {
      client.SetExternalBody(command.GetBody());
    }

    client.ApplyAndThrowException(answer, answerHeaders);
    client.ClearBody();
    DecodeAnswer(answer, answerHeaders);
  }

  
  static void RunInternal(OracleCallback& callback,
                          const Orthanc::WebServiceParameters& orthanc,
                          const OrthancRestApiCommand& command)
  {
    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;
    RunOrthancRestApiCommand(answer, answerHeaders, orthanc, command);

    std::unique_ptr<OrthancRestApiCommand::SuccessMessage> message(new OrthancRestApiCommand::SuccessMessage(command, answerHeaders));
    message->SwapAnswer(answer);

    callback.NotifySuccess(message.release());
  }


  static void RunInternal(OracleCallback& callback,
                          const Orthanc::WebServiceParameters& orthanc,
                          const GetOrthancImageCommand& command)
  {
    Orthanc::HttpClient client(orthanc, command.GetUri());
    client.SetRedirectionFollowed(false);
    client.SetTimeout(command.GetTimeout());

    CopyHttpHeaders(client, command.GetHttpHeaders());
    
    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;
    client.ApplyAndThrowException(answer, answerHeaders);

    DecodeAnswer(answer, answerHeaders);

    command.ProcessHttpAnswer(callback, answer, answerHeaders);
  }


  static void RunInternal(OracleCallback& callback,
                          const Orthanc::WebServiceParameters& orthanc,
                          const GetOrthancWebViewerJpegCommand& command)
  {
    Orthanc::HttpClient client(orthanc, command.GetUri());
    client.SetRedirectionFollowed(false);
    client.SetTimeout(command.GetTimeout());

    CopyHttpHeaders(client, command.GetHttpHeaders());

    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;
    client.ApplyAndThrowException(answer, answerHeaders);

    DecodeAnswer(answer, answerHeaders);

    command.ProcessHttpAnswer(callback, answer);
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

    return c.string();
  }


  static void RunInternal(OracleCallback& callback,
                          const std::string& root,
                          const ReadFileCommand& command)
  {
    std::string path = GetPath(root, command.GetPath());
    LOG(TRACE) << "Oracle reading file: " << path;

    std::string content;
    Orthanc::SystemToolbox::ReadFile(content, path, true /* log */);

    std::unique_ptr<ReadFileCommand::SuccessMessage> message(new ReadFileCommand::SuccessMessage(command));
    message->SwapContent(content);
    callback.NotifySuccess(message.release());
  }


#if ORTHANC_ENABLE_DCMTK == 1
  static Orthanc::ParsedDicomFile* ParseDicom(uint64_t& fileSize,  /* OUT */
                                              const std::string& path,
                                              bool isPixelData)
  {
    if (!Orthanc::SystemToolbox::IsRegularFile(path))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentFile);
    }
    
    LOG(TRACE) << "Parsing DICOM file, " << (isPixelData ? "with" : "without")
               << " pixel data: " << path;
    
    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();
    
    fileSize = Orthanc::SystemToolbox::GetFileSize(path);
    
    // Check for 32bit systems
    if (fileSize != static_cast<uint64_t>(static_cast<size_t>(fileSize)))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotEnoughMemory);
    }
    
    DcmFileFormat dicom;
    bool ok;
    
    if (isPixelData)
    {
      ok = dicom.loadFile(path.c_str()).good();
    }
    else
    {
#if DCMTK_VERSION_NUMBER >= 362
      /**
       * NB : We could stop at (0x3007, 0x0000) instead of
       * DCM_PixelData as the Stone framework does not use further
       * tags (cf. the Orthanc::DICOM_TAG_* constants), but we still
       * use "PixelData" as this does not change the runtime much, and
       * as it is more explicit.
       **/
      static const DcmTagKey STOP = DCM_PixelData;
      //static const DcmTagKey STOP(0x3007, 0x0000);

      ok = dicom.loadFileUntilTag(path.c_str(), EXS_Unknown, EGL_noChange,
                                  DCM_MaxReadLength, ERM_autoDetect, STOP).good();
#else
      // The primitive "loadFileUntilTag" was introduced in DCMTK 3.6.2
      ok = dicom.loadFile(path.c_str()).good();
#endif
    }

    if (ok)
    {
      std::unique_ptr<Orthanc::ParsedDicomFile> result(new Orthanc::ParsedDicomFile(dicom));

      boost::posix_time::ptime end = boost::posix_time::microsec_clock::local_time();
      LOG(TRACE) << path << ": parsed in " << (end-start).total_milliseconds() << " ms";

      return result.release();
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                      "Cannot parse file: " + path);
    }
  }
#endif

  
#if ORTHANC_ENABLE_DCMTK == 1
  static void RunInternal(OracleCallback& callback,
                          boost::shared_ptr<ParsedDicomCache> cache,
                          const std::string& root,
                          const ParseDicomFromFileCommand& command)
  {
    const std::string path = GetPath(root, command.GetPath());
    const std::string cacheKey = GetFileSystemCacheKey(path);

    if (cache)
    {
      ParsedDicomCache::Accessor accessor(*cache, cacheKey);

      if (accessor.IsValid())
      {
        assert(accessor.HasPixelData());

        // Reuse the DICOM file from the cache
        callback.NotifySuccess(new ParseDicomSuccessMessage(command, command.GetSource(), accessor.GetDicom(),
                                                            accessor.GetFileSize(), accessor.HasPixelData()));
        return;
      }
    }

    uint64_t fileSize;
    boost::shared_ptr<Orthanc::ParsedDicomFile> parsed(ParseDicom(fileSize, path, command.IsPixelDataIncluded()));

    if (fileSize != static_cast<size_t>(fileSize))
    {
      // Cannot load such a large file on 32-bit architecture
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotEnoughMemory);
    }
    
    callback.NotifySuccess(new ParseDicomSuccessMessage(command, command.GetSource(), parsed,
                                                        static_cast<size_t>(fileSize), command.IsPixelDataIncluded()));

    if (cache &&
        command.IsPixelDataIncluded())
    {
      cache->Store(cacheKey, parsed, static_cast<size_t>(fileSize), command.IsPixelDataIncluded());
    }
  }
#endif
  

#if ORTHANC_ENABLE_DCMTK == 1
  static void RunInternal(OracleCallback& callback,
                          boost::shared_ptr<ParsedDicomCache> cache,
                          const Orthanc::WebServiceParameters& orthanc,
                          const ParseDicomFromWadoCommand& command)
  {
    const std::string cacheKey = GetSopInstanceUidCacheKey(command.GetSopInstanceUid());

    if (cache)
    {
      ParsedDicomCache::Accessor accessor(*cache, cacheKey);

      if (accessor.IsValid())
      {
        assert(accessor.HasPixelData());

        // Reuse the DICOM file from the cache
        callback.NotifySuccess(new ParseDicomSuccessMessage(command, command.GetSource(), accessor.GetDicom(),
                                                            accessor.GetFileSize(), accessor.HasPixelData()));
        return;
      }
    }

    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;

    switch (command.GetRestCommand().GetType())
    {
      case IOracleCommand::Type_Http:
        RunHttpCommand(answer, answerHeaders, dynamic_cast<const HttpCommand&>(command.GetRestCommand()));
        break;
        
      case IOracleCommand::Type_OrthancRestApi:
        RunOrthancRestApiCommand(answer, answerHeaders, orthanc,
                                 dynamic_cast<const OrthancRestApiCommand&>(command.GetRestCommand()));
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    size_t fileSize;
    boost::shared_ptr<Orthanc::ParsedDicomFile> parsed(ParseDicomSuccessMessage::ParseWadoAnswer(fileSize, answer, answerHeaders));

    callback.NotifySuccess(new ParseDicomSuccessMessage(command, command.GetSource(), parsed, fileSize,
                                                        true /* pixel data always is included in WADO-RS */));

    if (cache)
    {
      // Store it into the cache for future use
      cache->Store(cacheKey, parsed, static_cast<size_t>(fileSize), true);
    }
  }
#endif


  void GenericOracleRunner::Run(OracleCallback& callback)
  {
    Orthanc::ErrorCode error = Orthanc::ErrorCode_Success;
    
    try
    {
      switch (callback.GetCommand().GetType())
      {
        case IOracleCommand::Type_Sleep:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType,
                                          "Sleep command cannot be executed by the runner");

        case IOracleCommand::Type_Http:
          RunInternal(callback, dynamic_cast<const HttpCommand&>(callback.GetCommand()));
          break;

        case IOracleCommand::Type_OrthancRestApi:
          RunInternal(callback, configuration_.GetRemoteOrthancParameters(),
                      dynamic_cast<const OrthancRestApiCommand&>(callback.GetCommand()));
          break;

        case IOracleCommand::Type_GetOrthancImage:
          RunInternal(callback, configuration_.GetRemoteOrthancParameters(),
                      dynamic_cast<const GetOrthancImageCommand&>(callback.GetCommand()));
          break;

        case IOracleCommand::Type_GetOrthancWebViewerJpeg:
          RunInternal(callback, configuration_.GetRemoteOrthancParameters(),
                      dynamic_cast<const GetOrthancWebViewerJpegCommand&>(callback.GetCommand()));
          break;

        case IOracleCommand::Type_ReadFile:
          RunInternal(callback, configuration_.GetRootDirectory(),
                      dynamic_cast<const ReadFileCommand&>(callback.GetCommand()));
          break;

        case IOracleCommand::Type_ParseDicomFromFile:
        case IOracleCommand::Type_ParseDicomFromWado:
#if ORTHANC_ENABLE_DCMTK == 1
          switch (callback.GetCommand().GetType())
          {
            case IOracleCommand::Type_ParseDicomFromFile:
              RunInternal(callback, dicomCache_, configuration_.GetRootDirectory(),
                          dynamic_cast<const ParseDicomFromFileCommand&>(callback.GetCommand()));
              break;

            case IOracleCommand::Type_ParseDicomFromWado:
              RunInternal(callback, dicomCache_, configuration_.GetRemoteOrthancParameters(),
                          dynamic_cast<const ParseDicomFromWadoCommand&>(callback.GetCommand()));
              break;

            default:
              throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
          }            
          break;
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
      error = e.GetErrorCode();
    }
    catch (...)
    {
      LOG(ERROR) << "Threaded exception within the oracle";
      error = Orthanc::ErrorCode_InternalError;
    }

    if (error != Orthanc::ErrorCode_Success)
    {
      callback.NotifyError(Orthanc::OrthancException(error));
    }
  }
}
