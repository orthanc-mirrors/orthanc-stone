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


#include "SmartLoader.h"
#include "Layers/OrthancFrameLayerSource.h"

namespace OrthancStone
{
  SmartLoader::SmartLoader(MessageBroker& broker, IWebService& webService) :
    IObservable(broker),
    IObserver(broker),
    imageQuality_(SliceImageQuality_FullPam),
    webService_(webService)
  {}

  void SmartLoader::HandleMessage(IObservable& from, const IMessage& message)
  {
    // forward messages to its own observers
    IObservable::broker_.EmitMessage(from, IObservable::observers_, message);
  }

  ILayerSource* SmartLoader::GetFrame(const std::string& instanceId, unsigned int frame)
  {
    std::auto_ptr<OrthancFrameLayerSource> layerSource (new OrthancFrameLayerSource(IObserver::broker_, webService_));
    layerSource->SetImageQuality(imageQuality_);
    layerSource->RegisterObserver(*this);
    layerSource->LoadFrame(instanceId, frame);

    return layerSource.release();
  }


}
