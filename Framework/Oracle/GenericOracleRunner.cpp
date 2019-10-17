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

#include "CustomOracleCommand.h"
#include "GetOrthancImageCommand.h"
#include "GetOrthancWebViewerJpegCommand.h"
#include "HttpCommand.h"
#include "OracleCommandExceptionMessage.h"
#include "OrthancRestApiCommand.h"

#include <Core/Compression/GzipCompressor.h>
#include <Core/HttpClient.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

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


  static void Execute(IMessageEmitter& emitter,
                      boost::weak_ptr<IObserver>& receiver,
                      const HttpCommand& command)
  {
    Orthanc::HttpClient client;
    client.SetUrl(command.GetUrl());
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

    HttpCommand::SuccessMessage message(command, answerHeaders, answer);
    emitter.EmitMessage(receiver, message);
  }


  static void Execute(IMessageEmitter& emitter,
                      const Orthanc::WebServiceParameters& orthanc,
                      boost::weak_ptr<IObserver>& receiver,
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

    OrthancRestApiCommand::SuccessMessage message(command, answerHeaders, answer);
    emitter.EmitMessage(receiver, message);
  }


  static void Execute(IMessageEmitter& emitter,
                      const Orthanc::WebServiceParameters& orthanc,
                      boost::weak_ptr<IObserver>& receiver,
                      const GetOrthancImageCommand& command)
  {
    Orthanc::HttpClient client(orthanc, command.GetUri());
    client.SetTimeout(command.GetTimeout());

    CopyHttpHeaders(client, command.GetHttpHeaders());
    
    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;
    client.ApplyAndThrowException(answer, answerHeaders);

    DecodeAnswer(answer, answerHeaders);

    command.ProcessHttpAnswer(emitter, receiver, answer, answerHeaders);
  }


  static void Execute(IMessageEmitter& emitter,
                      const Orthanc::WebServiceParameters& orthanc,
                      boost::weak_ptr<IObserver>& receiver,
                      const GetOrthancWebViewerJpegCommand& command)
  {
    Orthanc::HttpClient client(orthanc, command.GetUri());
    client.SetTimeout(command.GetTimeout());

    CopyHttpHeaders(client, command.GetHttpHeaders());

    std::string answer;
    Orthanc::HttpClient::HttpHeaders answerHeaders;
    client.ApplyAndThrowException(answer, answerHeaders);

    DecodeAnswer(answer, answerHeaders);

    command.ProcessHttpAnswer(emitter, receiver, answer);
  }


  void GenericOracleRunner::Run(boost::weak_ptr<IObserver>& receiver,
                                IOracleCommand& command)
  {
    try
    {
      switch (command.GetType())
      {
        case IOracleCommand::Type_Sleep:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType,
                                          "Sleep command cannot be executed by the runner");
          break;

        case IOracleCommand::Type_Http:
          Execute(emitter_, receiver, 
                  dynamic_cast<const HttpCommand&>(command));
          break;

        case IOracleCommand::Type_OrthancRestApi:
          Execute(emitter_, orthanc_, receiver, dynamic_cast<const OrthancRestApiCommand&>(command));
          break;

        case IOracleCommand::Type_GetOrthancImage:
          Execute(emitter_, orthanc_, receiver, dynamic_cast<const GetOrthancImageCommand&>(command));
          break;

        case IOracleCommand::Type_GetOrthancWebViewerJpeg:
          Execute(emitter_, orthanc_, receiver, dynamic_cast<const GetOrthancWebViewerJpegCommand&>(command));
          break;

        case IOracleCommand::Type_Custom:
          dynamic_cast<CustomOracleCommand&>(command).Execute(emitter_, receiver, *this);
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "Exception within the oracle: " << e.What();
      emitter_.EmitMessage(receiver, OracleCommandExceptionMessage(command, e));
    }
    catch (...)
    {
      LOG(ERROR) << "Threaded exception within the oracle";
      emitter_.EmitMessage(receiver, OracleCommandExceptionMessage
                           (command, Orthanc::ErrorCode_InternalError));
    }
  }
}
