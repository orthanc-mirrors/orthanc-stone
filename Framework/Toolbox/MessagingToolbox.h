/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

#include "../../Resources/Orthanc/Plugins/Samples/Common/IOrthancConnection.h"

#include "../Enumerations.h"
#include "../../Resources/Orthanc/Core/Images/ImageAccessor.h"

#include <json/value.h>

#if defined(__native_client__)
#  include <ppapi/cpp/core.h>
#else
#  include <boost/date_time/posix_time/ptime.hpp>
#endif

namespace OrthancStone
{
  namespace MessagingToolbox
  {
    class Timestamp
    {
    private:
#if defined(__native_client__)
      PP_TimeTicks   time_;
#else
      boost::posix_time::ptime   time_;
#endif

    public:
      Timestamp();

#if defined(__native_client__)
      static void Initialize(pp::Core* core);
#endif

      int GetMillisecondsSince(const Timestamp& other);
    };


    void RestApiGet(Json::Value& target,
                    OrthancPlugins::IOrthancConnection& orthanc,
                    const std::string& uri);

    void RestApiPost(Json::Value& target,
                     OrthancPlugins::IOrthancConnection& orthanc,
                     const std::string& uri,
                     const std::string& body);

    bool HasWebViewerInstalled(OrthancPlugins::IOrthancConnection& orthanc);

    bool CheckOrthancVersion(OrthancPlugins::IOrthancConnection& orthanc);

    // This downloads the image from Orthanc and keeps its pixel
    // format unchanged (will be either Grayscale8, Grayscale16,
    // SignedGrayscale16, or RGB24)
    Orthanc::ImageAccessor* DecodeFrame(OrthancPlugins::IOrthancConnection& orthanc,
                                        const std::string& instance,
                                        unsigned int frame,
                                        Orthanc::PixelFormat targetFormat);

    Orthanc::ImageAccessor* DecodeJpegFrame(OrthancPlugins::IOrthancConnection& orthanc,
                                            const std::string& instance,
                                            unsigned int frame,
                                            unsigned int quality,
                                            Orthanc::PixelFormat targetFormat);
  }
}
