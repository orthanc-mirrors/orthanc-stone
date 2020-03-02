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


#pragma once

#include "../Messages/IMessageEmitter.h"
#include "OracleCommandWithPayload.h"

#include <Core/Images/ImageAccessor.h>

#include <map>

namespace OrthancStone
{
  class GetOrthancImageCommand : public OracleCommandWithPayload
  {
  public:
    typedef std::map<std::string, std::string>  HttpHeaders;

    class SuccessMessage : public OriginMessage<GetOrthancImageCommand>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      std::unique_ptr<Orthanc::ImageAccessor>  image_;
      Orthanc::MimeType                      mime_;

    public:
      SuccessMessage(const GetOrthancImageCommand& command,
                     Orthanc::ImageAccessor* image,   // Takes ownership
                     Orthanc::MimeType mime);

      const Orthanc::ImageAccessor& GetImage() const
      {
        return *image_;
      }

      Orthanc::MimeType GetMimeType() const
      {
        return mime_;
      }
    };


  private:
    std::string           uri_;
    HttpHeaders           headers_;
    unsigned int          timeout_;
    bool                  hasExpectedFormat_;
    Orthanc::PixelFormat  expectedFormat_;

  public:
    GetOrthancImageCommand();

    virtual Type GetType() const
    {
      return Type_GetOrthancImage;
    }

    void SetExpectedPixelFormat(Orthanc::PixelFormat format);

    void SetUri(const std::string& uri)
    {
      uri_ = uri;
    }

    void SetInstanceUri(const std::string& instance,
                        Orthanc::PixelFormat pixelFormat);

    void SetHttpHeader(const std::string& key,
                       const std::string& value)
    {
      headers_[key] = value;
    }

    const std::string& GetUri() const
    {
      return uri_;
    }

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

    void ProcessHttpAnswer(IMessageEmitter& emitter,
                           const IObserver& receiver,
                           const std::string& answer,
                           const HttpHeaders& answerHeaders) const;
  };
}
