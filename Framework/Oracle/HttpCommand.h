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


#pragma once

#include "../Messages/IMessage.h"
#include "OracleCommandWithPayload.h"

#include <Core/Enumerations.h>

#include <map>
#include <json/value.h>

namespace OrthancStone
{
  class HttpCommand : public OracleCommandWithPayload
  {
  public:
    typedef std::map<std::string, std::string>  HttpHeaders;

    class SuccessMessage : public OriginMessage<HttpCommand>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      HttpHeaders   headers_;
      std::string   answer_;

    public:
      SuccessMessage(const HttpCommand& command,
                     const HttpHeaders& answerHeaders,
                     std::string& answer  /* will be swapped to avoid a memcpy() */);

      const std::string& GetAnswer() const
      {
        return answer_;
      }

      void ParseJsonBody(Json::Value& target) const;

      const HttpHeaders&  GetAnswerHeaders() const
      {
        return headers_;
      }
    };


  private:
    Orthanc::HttpMethod  method_;
    std::string          url_;
    std::string          body_;
    HttpHeaders          headers_;
    unsigned int         timeout_;

  public:
    HttpCommand();

    virtual Type GetType() const
    {
      return Type_Http;
    }

    void SetMethod(Orthanc::HttpMethod method)
    {
      method_ = method;
    }

    void SetUrl(const std::string& url)
    {
      url_ = url;
    }

    void SetBody(const std::string& body)
    {
      body_ = body;
    }

    void SetBody(const Json::Value& json);

    void SwapBody(std::string& body)
    {
      body_.swap(body);
    }

    void SetHttpHeaders(const HttpHeaders& headers)
    {
      headers_ = headers;
    }

    void SetHttpHeader(const std::string& key,
                       const std::string& value)
    {
      headers_[key] = value;
    }

    Orthanc::HttpMethod GetMethod() const
    {
      return method_;
    }

    const std::string& GetUrl() const
    {
      return url_;
    }

    const std::string& GetBody() const;

    const HttpHeaders& GetHttpHeaders() const
    {
      return headers_;
    }

    void SetTimeout(unsigned int seconds)
    {
      timeout_ = seconds;
    }

    unsigned int GetTimeout() const
    {
      return timeout_;
    }
  };
}