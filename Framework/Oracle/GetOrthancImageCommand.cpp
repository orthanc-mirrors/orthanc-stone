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


#include "GetOrthancImageCommand.h"

#include <Core/Images/JpegReader.h>
#include <Core/Images/PamReader.h>
#include <Core/Images/PngReader.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

namespace OrthancStone
{
  GetOrthancImageCommand::SuccessMessage::SuccessMessage(const GetOrthancImageCommand& command,
                                                         Orthanc::ImageAccessor* image,   // Takes ownership
                                                         Orthanc::MimeType mime) :
    OriginMessage(command),
    image_(image),
    mime_(mime)
  {
    if (image == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
  }


  GetOrthancImageCommand::GetOrthancImageCommand() :
    uri_("/"),
    timeout_(600),
    hasExpectedFormat_(false)
  {
  }


  void GetOrthancImageCommand::SetExpectedPixelFormat(Orthanc::PixelFormat format)
  {
    hasExpectedFormat_ = true;
    expectedFormat_ = format;
  }


  void GetOrthancImageCommand::SetInstanceUri(const std::string& instance,
                                              Orthanc::PixelFormat pixelFormat)
  {
    uri_ = "/instances/" + instance;
          
    switch (pixelFormat)
    {
      case Orthanc::PixelFormat_RGB24:
        uri_ += "/preview";
        break;
      
      case Orthanc::PixelFormat_Grayscale16:
        uri_ += "/image-uint16";
        break;
      
      case Orthanc::PixelFormat_SignedGrayscale16:
        uri_ += "/image-int16";
        break;
      
      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }

  void GetOrthancImageCommand::ProcessHttpAnswer(IMessageEmitter& emitter,
                                                 const IObserver& receiver,
                                                 const std::string& answer,
                                                 const HttpHeaders& answerHeaders) const
  {
    Orthanc::MimeType contentType = Orthanc::MimeType_Binary;

    for (HttpHeaders::const_iterator it = answerHeaders.begin(); 
         it != answerHeaders.end(); ++it)
    {
      std::string s;
      Orthanc::Toolbox::ToLowerCase(s, it->first);

      if (s == "content-type")
      {
        contentType = Orthanc::StringToMimeType(it->second);
        break;
      }
    }

    std::unique_ptr<Orthanc::ImageAccessor> image;

    switch (contentType)
    {
      case Orthanc::MimeType_Png:
      {
        image.reset(new Orthanc::PngReader);
        dynamic_cast<Orthanc::PngReader&>(*image).ReadFromMemory(answer);
        break;
      }

      case Orthanc::MimeType_Pam:
      {
        image.reset(new Orthanc::PamReader);
        dynamic_cast<Orthanc::PamReader&>(*image).ReadFromMemory(answer);
        break;
      }

      case Orthanc::MimeType_Jpeg:
      {
        image.reset(new Orthanc::JpegReader);
        dynamic_cast<Orthanc::JpegReader&>(*image).ReadFromMemory(answer);
        break;
      }

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol,
                                        "Unsupported HTTP Content-Type for an image: " + 
                                        std::string(Orthanc::EnumerationToString(contentType)));
    }

    if (hasExpectedFormat_)
    {
      if (expectedFormat_ == Orthanc::PixelFormat_SignedGrayscale16 &&
          image->GetFormat() == Orthanc::PixelFormat_Grayscale16)
      {
        image->SetFormat(Orthanc::PixelFormat_SignedGrayscale16);
      }

      if (expectedFormat_ != image->GetFormat())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }
    }

    SuccessMessage message(*this, image.release(), contentType);
    emitter.EmitMessage(receiver, message);
  }
}
