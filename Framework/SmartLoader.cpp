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
#include "Layers/OrthancFrameLayer.h"

namespace OrthancStone
{
  SmartLoader::SmartLoader(MessageBroker& broker, IWebService& webService) :
    IObservable(broker),
    IObserver(broker),
    imageQuality_(SliceImageQuality_FullPam),
    webService_(webService),
    orthancApiClient_(broker, webService)
  {
    DeclareHandledMessage(MessageType_LayerSource_GeometryReady);
    DeclareHandledMessage(MessageType_LayerSource_LayerReady);
    DeclareIgnoredMessage(MessageType_LayerSource_GeometryError);
    DeclareIgnoredMessage(MessageType_LayerSource_ContentChanged);
    DeclareIgnoredMessage(MessageType_LayerSource_SliceChanged);

//    DeclareHandledMessage(MessageType_OrthancApi_InternalGetJsonResponseReady);
//    DeclareIgnoredMessage(MessageType_OrthancApi_InternalGetJsonResponseError);
  }

  void SmartLoader::HandleMessage(IObservable& from, const IMessage& message)
  {
    switch (message.GetType()) {
    case MessageType_LayerSource_GeometryReady:
    {
      const OrthancFrameLayerSource* layerSource=dynamic_cast<const OrthancFrameLayerSource*>(&from);
      // TODO keep track of objects that have been loaded already
    }; break;
    case MessageType_LayerSource_LayerReady:
    {
      const OrthancFrameLayerSource* layerSource=dynamic_cast<const OrthancFrameLayerSource*>(&from);
      // TODO keep track of objects that have been loaded already
    }; break;
//    case MessageType_OrthancApi_GetStudyIds_Ready:
//    {

//      const OrthancApiClient::GetJsonResponseReadyMessage& msg = dynamic_cast<OrthancApiClient::GetJsonResponseReadyMessage&>(message);

//    }; break;
    default:
      VLOG("unhandled message type" << message.GetType());
    }

    // forward messages to its own observers
    IObservable::broker_.EmitMessage(from, IObservable::observers_, message);
  }

  ILayerSource* SmartLoader::GetFrame(const std::string& instanceId, unsigned int frame)
  {
    // TODO: check if this frame has already been loaded or is already being loaded.
    // - if already loaded: create a "clone" that will emit the GeometryReady/ImageReady messages "immediately"
    //   (it can not be immediate because Observers needs to register first and this is done after this method returns)
    // - if currently loading, we need to return an object that will observe the existing LayerSource and forward
    //   the messages to its observables
    // in both cases, we must be carefull about objects lifecycle !!!
    std::auto_ptr<OrthancFrameLayerSource> layerSource (new OrthancFrameLayerSource(IObserver::broker_, webService_));
    layerSource->SetImageQuality(imageQuality_);
    layerSource->RegisterObserver(*this);
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
