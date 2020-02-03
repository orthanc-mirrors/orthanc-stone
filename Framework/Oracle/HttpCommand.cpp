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


#include "HttpCommand.h"

#include <Core/OrthancException.h>

#include <json/reader.h>
#include <json/writer.h>

namespace OrthancStone
{
  HttpCommand::SuccessMessage::SuccessMessage(const HttpCommand& command,
                                              const HttpHeaders& answerHeaders,
                                              std::string& answer) :
    OriginMessage(command),
    headers_(answerHeaders),
    answer_(answer)
  {
  }


  void HttpCommand::SuccessMessage::ParseJsonBody(Json::Value& target) const
  {
    Json::Reader reader;
    if (!reader.parse(answer_, target))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  }


  HttpCommand::HttpCommand() :
    method_(Orthanc::HttpMethod_Get),
    url_("/"),
    timeout_(600)
  {
  }


  void HttpCommand::SetBody(const Json::Value& json)
  {
    Json::FastWriter writer;
    body_ = writer.write(json);
  }


  const std::string& HttpCommand::GetBody() const
  {
    if (method_ == Orthanc::HttpMethod_Post ||
        method_ == Orthanc::HttpMethod_Put)
    {
      return body_;
    }
    else
    {
      LOG(ERROR) << "HttpCommand::GetBody(): method_  not _Post or _Put";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
}
