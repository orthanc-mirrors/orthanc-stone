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
#include <map>

#include "Layers/ILayerSource.h"
#include "Messages/IObservable.h"
#include "../Platforms/Generic/OracleWebService.h"
#include "Toolbox/OrthancApiClient.h"

namespace OrthancStone
{
  class SmartLoader : public IObservable, IObserver
  {
    SliceImageQuality     imageQuality_;
    IWebService&          webService_;
    OrthancApiClient      orthancApiClient_;

    int studyListRequest_;

  public:
    SmartLoader(MessageBroker& broker, IWebService& webService);  // TODO: add maxPreloadStorageSizeInBytes

    virtual void HandleMessage(IObservable& from, const IMessage& message);

    void PreloadStudy(const std::string studyId);
    void PreloadSeries(const std::string seriesId);
    void LoadStudyList();

    void SetImageQuality(SliceImageQuality imageQuality) { imageQuality_ = imageQuality; }

    ILayerSource* GetFrame(const std::string& instanceId, unsigned int frame);

    void GetFirstInstanceIdForSeries(std::string& output, const std::string& seriesId);

  };

}
