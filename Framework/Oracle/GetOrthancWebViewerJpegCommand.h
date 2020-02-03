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
  class GetOrthancWebViewerJpegCommand : public OracleCommandWithPayload
  {
  public:
    typedef std::map<std::string, std::string>  HttpHeaders;

    class SuccessMessage : public OriginMessage<GetOrthancWebViewerJpegCommand>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      std::auto_ptr<Orthanc::ImageAccessor>  image_;

    public:
      SuccessMessage(const GetOrthancWebViewerJpegCommand& command,
                     Orthanc::ImageAccessor* image);   // Takes ownership

      const Orthanc::ImageAccessor& GetImage() const
      {
        return *image_;
      }
    };

  private:
    std::string           instanceId_;
    unsigned int          frame_;
    unsigned int          quality_;
    HttpHeaders           headers_;
    unsigned int          timeout_;
    Orthanc::PixelFormat  expectedFormat_;

  public:
    GetOrthancWebViewerJpegCommand();

    virtual Type GetType() const
    {
      return Type_GetOrthancWebViewerJpeg;
    }

    void SetExpectedPixelFormat(Orthanc::PixelFormat format)
    {
      expectedFormat_ = format;
    }

    void SetInstance(const std::string& instanceId)
    {
      instanceId_ = instanceId;
    }

    void SetFrame(unsigned int frame)
    {
      frame_ = frame;
    }

    void SetQuality(unsigned int quality);

    void SetHttpHeader(const std::string& key,
                       const std::string& value)
    {
      headers_[key] = value;
    }

    Orthanc::PixelFormat GetExpectedPixelFormat() const
    {
      return expectedFormat_;
    }

    const std::string& GetInstanceId() const
    {
      return instanceId_;
    }

    unsigned int GetFrame() const
    {
      return frame_;
    }

    unsigned int GetQuality() const
    {
      return quality_;
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

    std::string GetUri() const;

    void ProcessHttpAnswer(IMessageEmitter& emitter,
                           const IObserver& receiver,
                           const std::string& answer) const;
  };
}
