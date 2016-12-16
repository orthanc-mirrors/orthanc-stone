/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "MessagingToolbox.h"

#include "../../Resources/Orthanc/Core/Images/Image.h"
#include "../../Resources/Orthanc/Core/Images/ImageProcessing.h"
#include "../../Resources/Orthanc/Core/Images/JpegReader.h"
#include "../../Resources/Orthanc/Core/Images/PngReader.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../../Resources/Orthanc/Core/Toolbox.h"
#include "../../Resources/Orthanc/Core/Logging.h"

#include <boost/lexical_cast.hpp>
#include <json/reader.h>

#if defined(__native_client__)
#  include <boost/math/special_functions/round.hpp>
#else
#  include <boost/date_time/posix_time/posix_time.hpp>
#  include <boost/date_time/microsec_time_clock.hpp>
#endif

namespace OrthancStone
{
  namespace MessagingToolbox
  {
#if defined(__native_client__)
    static pp::Core* core_ = NULL;

    void Timestamp::Initialize(pp::Core* core)
    {
      if (core == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      core_ = core;
    }

    Timestamp::Timestamp()
    {
      if (core_ == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      time_ = core_->GetTimeTicks();
    }

    int Timestamp::GetMillisecondsSince(const Timestamp& other)
    {
      double difference = time_ - other.time_;
      return static_cast<int>(boost::math::iround(difference * 1000.0));
    }
#else
    Timestamp::Timestamp()
    {
      time_ = boost::posix_time::microsec_clock::local_time();
    }

    int Timestamp::GetMillisecondsSince(const Timestamp& other)
    {
      boost::posix_time::time_duration difference = time_ - other.time_;
      return static_cast<int>(difference.total_milliseconds());
    }
#endif

    static bool ParseVersion(std::string& version,
                             unsigned int& major,
                             unsigned int& minor,
                             unsigned int& patch,
                             const Json::Value& info)
    {
      if (info.type() != Json::objectValue ||
          !info.isMember("Version") ||
          info["Version"].type() != Json::stringValue)
      {
        return false;
      }

      version = info["Version"].asString();
      if (version == "mainline")
      {
        // Some arbitrary high values Orthanc versions will never reach ;)
        major = 999;
        minor = 999;
        patch = 999;
        return true;
      }

      std::vector<std::string> tokens;
      Orthanc::Toolbox::TokenizeString(tokens, version, '.');
      
      if (tokens.size() != 2 &&
          tokens.size() != 3)
      {
        return false;
      }

      int a, b, c;
      try
      {
        a = boost::lexical_cast<int>(tokens[0]);
        b = boost::lexical_cast<int>(tokens[1]);

        if (tokens.size() == 3)
        {
          c = boost::lexical_cast<int>(tokens[2]);
        }
        else
        {
          c = 0;
        }
      }
      catch (boost::bad_lexical_cast&)
      {
        return false;
      }

      if (a < 0 ||
          b < 0 ||
          c < 0)
      {
        return false;
      }
      else
      {
        major = static_cast<unsigned int>(a);
        minor = static_cast<unsigned int>(b);
        patch = static_cast<unsigned int>(c);
        return true;
      }         
    }

    void ParseJson(Json::Value& target,
                   const std::string& source)
    {
      Json::Reader reader;
      if (!reader.parse(source, target))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }

    void RestApiGet(Json::Value& target,
                    OrthancPlugins::IOrthancConnection& orthanc,
                    const std::string& uri)
    {
      std::string tmp;
      orthanc.RestApiGet(tmp, uri);
      ParseJson(target, tmp);
    }


    bool HasWebViewerInstalled(OrthancPlugins::IOrthancConnection& orthanc)
    {
      try
      {
        Json::Value json;
        RestApiGet(json, orthanc, "/plugins/web-viewer");
        return json.type() == Json::objectValue;
      }
      catch (Orthanc::OrthancException&)
      {
        return false;
      }
    }


    bool CheckOrthancVersion(OrthancPlugins::IOrthancConnection& orthanc)
    {
      Json::Value json;
      std::string version;
      unsigned int major, minor, patch;

      try
      {
        RestApiGet(json, orthanc, "/system");
      }
      catch (Orthanc::OrthancException&)
      {
        LOG(ERROR) << "Cannot connect to your Orthanc server";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }
        
      if (!ParseVersion(version, major, minor, patch, json))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      LOG(WARNING) << "Version of the Orthanc core (must be above 1.1.0): " << version;

      // Stone is only compatible with Orthanc >= 1.1.0, otherwise deadlocks might occur
      if (major < 1 ||
          (major == 1 && minor < 1))
      {
        return false;
      }

      try
      {
        RestApiGet(json, orthanc, "/plugins/web-viewer");       
      }
      catch (Orthanc::OrthancException&)
      {
        // The Web viewer is not installed, this is OK
        LOG(WARNING) << "The Web viewer plugin is not installed, progressive download is disabled";
        return true;
      }

      if (!ParseVersion(version, major, minor, patch, json))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      LOG(WARNING) << "Version of the Web viewer plugin (must be above 2.2): " << version;

      return (major >= 3 ||
              (major == 2 && minor >= 2));
    }


    Orthanc::ImageAccessor* DecodeFrame(OrthancPlugins::IOrthancConnection& orthanc,
                                        const std::string& instance,
                                        unsigned int frame,
                                        Orthanc::PixelFormat targetFormat)
    {
      std::string uri = ("instances/" + instance + "/frames/" + 
                         boost::lexical_cast<std::string>(frame));

      std::string compressed;

      switch (targetFormat)
      {
        case Orthanc::PixelFormat_RGB24:
          orthanc.RestApiGet(compressed, uri + "/preview");
          break;

        case Orthanc::PixelFormat_Grayscale16:
          orthanc.RestApiGet(compressed, uri + "/image-uint16");
          break;

        case Orthanc::PixelFormat_SignedGrayscale16:
          orthanc.RestApiGet(compressed, uri + "/image-int16");
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      
      std::auto_ptr<Orthanc::PngReader> result(new Orthanc::PngReader);
      result->ReadFromMemory(compressed);

      if (targetFormat == Orthanc::PixelFormat_SignedGrayscale16)
      {
        if (result->GetFormat() == Orthanc::PixelFormat_Grayscale16)
        {
          result->SetFormat(Orthanc::PixelFormat_SignedGrayscale16);
        }
        else
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
        }
      }

      return result.release();
    }


    Orthanc::ImageAccessor* DecodeJpegFrame(OrthancPlugins::IOrthancConnection& orthanc,
                                            const std::string& instance,
                                            unsigned int frame,
                                            unsigned int quality,
                                            Orthanc::PixelFormat targetFormat)
    {
      if (quality <= 0 || 
          quality > 100)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      // This requires the official Web viewer plugin to be installed!
      std::string uri = ("web-viewer/instances/jpeg" + 
                         boost::lexical_cast<std::string>(quality) + 
                         "-" + instance + "_" + 
                         boost::lexical_cast<std::string>(frame));

      Json::Value encoded;
      RestApiGet(encoded, orthanc, uri);

      if (encoded.type() != Json::objectValue ||
          !encoded.isMember("Orthanc") ||
          encoded["Orthanc"].type() != Json::objectValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      Json::Value& info = encoded["Orthanc"];
      if (!info.isMember("PixelData") ||
          !info.isMember("Stretched") ||
          !info.isMember("Compression") ||
          info["Compression"].type() != Json::stringValue ||
          info["PixelData"].type() != Json::stringValue ||
          info["Stretched"].type() != Json::booleanValue ||
          info["Compression"].asString() != "Jpeg")
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }          

      bool isSigned = false;
      bool isStretched = info["Stretched"].asBool();

      if (info.isMember("IsSigned"))
      {
        if (info["IsSigned"].type() != Json::booleanValue)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
        }          
        else
        {
          isSigned = info["IsSigned"].asBool();
        }
      }

      std::string jpeg;
      Orthanc::Toolbox::DecodeBase64(jpeg, info["PixelData"].asString());

      std::auto_ptr<Orthanc::JpegReader> reader(new Orthanc::JpegReader);
      reader->ReadFromMemory(jpeg);

      if (reader->GetFormat() == Orthanc::PixelFormat_RGB24)  // This is a color image
      {
        if (targetFormat != Orthanc::PixelFormat_RGB24)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
        }

        if (isSigned || isStretched)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        }
        else
        {
          return reader.release();
        }
      }

      if (reader->GetFormat() != Orthanc::PixelFormat_Grayscale8)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }

      if (!isStretched)
      {
        if (targetFormat != reader->GetFormat())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
        }

        return reader.release();
      }

      int32_t stretchLow = 0;
      int32_t stretchHigh = 0;

      if (!info.isMember("StretchLow") ||
          !info.isMember("StretchHigh") ||
          info["StretchLow"].type() != Json::intValue ||
          info["StretchHigh"].type() != Json::intValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      stretchLow = info["StretchLow"].asInt();
      stretchHigh = info["StretchHigh"].asInt();

      if (stretchLow < -32768 ||
          stretchHigh > 65535 ||
          (stretchLow < 0 && stretchHigh > 32767))
      {
        // This range cannot be represented with a uint16_t or an int16_t
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);          
      }

      // Decode a grayscale JPEG 8bpp image coming from the Web viewer
      std::auto_ptr<Orthanc::ImageAccessor> image
        (new Orthanc::Image(targetFormat, reader->GetWidth(), reader->GetHeight(), false));

      float scaling = static_cast<float>(stretchHigh - stretchLow) / 255.0f;
      float offset = static_cast<float>(stretchLow) / scaling;
      
      Orthanc::ImageProcessing::Convert(*image, *reader);
      Orthanc::ImageProcessing::ShiftScale(*image, offset, scaling);

#if 0
      /*info.removeMember("PixelData");
        std::cout << info.toStyledString();*/
      
      int64_t a, b;
      Orthanc::ImageProcessing::GetMinMaxValue(a, b, *image);
      std::cout << stretchLow << "->" << stretchHigh << " = " << a << "->" << b << std::endl;
#endif

      return image.release();
    }
  }
}
