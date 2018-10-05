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
#include "Messages/MessageForwarder.h"

namespace OrthancStone
{
  SmartLoader::SmartLoader(MessageBroker& broker, OrthancApiClient& orthancApiClient) :
    IObservable(broker),
    IObserver(broker),
    imageQuality_(SliceImageQuality_FullPam),
    orthancApiClient_(orthancApiClient)
  {
  }

  ILayerSource* SmartLoader::GetFrame(const std::string& instanceId, unsigned int frame)
  {
    // TODO: check if this frame has already been loaded or is already being loaded.
    // - if already loaded: create a "clone" that will emit the GeometryReady/ImageReady messages "immediately"
    //   (it can not be immediate because Observers needs to register first and this is done after this method returns)
    // - if currently loading, we need to return an object that will observe the existing LayerSource and forward
    //   the messages to its observables
    // in both cases, we must be carefull about objects lifecycle !!!
    std::auto_ptr<OrthancFrameLayerSource> layerSource (new OrthancFrameLayerSource(IObserver::broker_, orthancApiClient_));
    layerSource->SetImageQuality(imageQuality_);
    layerSource->RegisterObserverCallback(new MessageForwarder<ILayerSource::GeometryReadyMessage>(IObserver::broker_, *this));
    layerSource->RegisterObserverCallback(new MessageForwarder<ILayerSource::LayerReadyMessage>(IObserver::broker_, *this));
    layerSource->LoadFrame(instanceId, frame);

    return layerSource.release();
  }

  void SmartLoader::LoadStudyList()
  {
//    orthancApiClient_.ScheduleGetJsonRequest("/studies");
  }

  void PreloadStudy(const std::string studyId)
  {
    /* TODO */
  }

  void PreloadSeries(const std::string seriesId)
  {
    /* TODO */
  }


}
