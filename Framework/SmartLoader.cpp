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
#include "Core/Images/Image.h"
#include "Framework/Widgets/LayerWidget.h"
#include "Framework/StoneException.h"
#include "Framework/Layers/FrameRenderer.h"

namespace OrthancStone
{
    enum CachedSliceStatus
    {
      CachedSliceStatus_Loading,
      CachedSliceStatus_Loaded
    };

  class SmartLoader::CachedSlice : public LayerSourceBase
  {
  public:
    unsigned int                    sliceIndex_;
    std::auto_ptr<Slice>            slice_;
    boost::shared_ptr<Orthanc::ImageAccessor>   image_;
    SliceImageQuality               effectiveQuality_;
    CachedSliceStatus               status_;

  public:

    CachedSlice(MessageBroker& broker)
      : LayerSourceBase(broker)
    {}

    virtual bool GetExtent(std::vector<Vector>& points,
                           const CoordinateSystem3D& viewportSlice)
    {
      // TODO: viewportSlice is not used !!!!
      slice_->GetExtent(points);
      return true;
    }

    virtual void ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice)
    {
      // TODO: viewportSlice is not used !!!!

      // it has already been loaded -> trigger the "layer ready" message immediately
      bool isFull = (effectiveQuality_ == SliceImageQuality_FullPng || effectiveQuality_ == SliceImageQuality_FullPam);
      std::auto_ptr<Orthanc::ImageAccessor> accessor(new Orthanc::ImageAccessor());
      image_->GetReadOnlyAccessor(*accessor);
      LayerSourceBase::NotifyLayerReady(FrameRenderer::CreateRenderer(accessor.release(), *slice_, isFull),
                                        slice_->GetGeometry(), false);
    }

    CachedSlice* Clone() const
    {
      CachedSlice* output = new CachedSlice(broker_);
      output->sliceIndex_ = sliceIndex_;
      output->slice_.reset(slice_->Clone());
      output->image_ = image_;
      output->effectiveQuality_ = effectiveQuality_;
      output->status_ = status_;

      return output;
    }

  };


  SmartLoader::SmartLoader(MessageBroker& broker, OrthancApiClient& orthancApiClient) :
    IObservable(broker),
    IObserver(broker),
    imageQuality_(SliceImageQuality_FullPam),
    orthancApiClient_(orthancApiClient)
  {
  }

  void SmartLoader::SetFrameInWidget(LayerWidget& layerWidget, size_t layerIndex, const std::string& instanceId, unsigned int frame)
  {
    // TODO: check if this frame has already been loaded or is already being loaded.
    // - if already loaded: create a "clone" that will emit the GeometryReady/ImageReady messages "immediately"
    //   (it can not be immediate because Observers needs to register first and this is done after this method returns)
    // - if currently loading, we need to return an object that will observe the existing LayerSource and forward
    //   the messages to its observables
    // in both cases, we must be carefull about objects lifecycle !!!

    std::auto_ptr<ILayerSource> layerSource;

    if (cachedSlices_.find(instanceId) != cachedSlices_.end() && cachedSlices_[instanceId]->status_ == CachedSliceStatus_Loaded)
    {
      layerSource.reset(cachedSlices_[instanceId]->Clone());
    }
    else
    {
      layerSource.reset(new OrthancFrameLayerSource(IObserver::broker_, orthancApiClient_));
      dynamic_cast<OrthancFrameLayerSource*>(layerSource.get())->SetImageQuality(imageQuality_);
      layerSource->RegisterObserverCallback(new Callable<SmartLoader, ILayerSource::GeometryReadyMessage>(*this, &SmartLoader::OnLayerGeometryReady));
      layerSource->RegisterObserverCallback(new Callable<SmartLoader, ILayerSource::ImageReadyMessage>(*this, &SmartLoader::OnImageReady));
      layerSource->RegisterObserverCallback(new Callable<SmartLoader, ILayerSource::LayerReadyMessage>(*this, &SmartLoader::OnLayerReady));
      dynamic_cast<OrthancFrameLayerSource*>(layerSource.get())->LoadFrame(instanceId, frame);
    }

    // make sure that the widget registers the events before we trigger them
    if (layerWidget.GetLayerCount() == layerIndex)
    {
      layerWidget.AddLayer(layerSource.release());
    }
    else if (layerWidget.GetLayerCount() > layerIndex)
    {
      layerWidget.ReplaceLayer(layerIndex, layerSource.release());
    }
    else
    {
      throw StoneException(ErrorCode_CanOnlyAddOneLayerAtATime);
    }

    SmartLoader::CachedSlice* cachedSlice = dynamic_cast<SmartLoader::CachedSlice*>(layerSource.get());
    if (cachedSlice != NULL)
    {
      cachedSlice->NotifyGeometryReady();
    }

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


  void SmartLoader::OnLayerGeometryReady(const ILayerSource::GeometryReadyMessage& message)
  {
    OrthancFrameLayerSource& source = dynamic_cast<OrthancFrameLayerSource&>(message.origin_);
    // save the slice
    const Slice& slice = source.GetSlice(0); // TODO handle GetSliceCount()
    std::string instanceId = slice.GetOrthancInstanceId();

    CachedSlice* cachedSlice = new CachedSlice(IObserver::broker_);
    cachedSlice->slice_.reset(slice.Clone());
    cachedSlice->effectiveQuality_ = source.GetImageQuality();
    cachedSlice->status_ = CachedSliceStatus_Loading;

    cachedSlices_[instanceId] = boost::shared_ptr<CachedSlice>(cachedSlice);

    // re-emit original Layer message to observers
    EmitMessage(message);
  }

  void SmartLoader::OnImageReady(const ILayerSource::ImageReadyMessage& message)
  {
    OrthancFrameLayerSource& source = dynamic_cast<OrthancFrameLayerSource&>(message.origin_);

    // save the slice
    const Slice& slice = source.GetSlice(0); // TODO handle GetSliceCount() ?
    std::string instanceId = slice.GetOrthancInstanceId();

    boost::shared_ptr<CachedSlice> cachedSlice(new CachedSlice(IObserver::broker_));
    cachedSlice->image_ = message.image_;
    cachedSlice->effectiveQuality_ = message.imageQuality_;
    cachedSlice->slice_.reset(message.slice_.Clone());
    cachedSlice->status_ = CachedSliceStatus_Loaded;

    cachedSlices_[instanceId] = cachedSlice;

    // re-emit original Layer message to observers
    EmitMessage(message);
  }

  void SmartLoader::OnLayerReady(const ILayerSource::LayerReadyMessage& message)
  {
    // re-emit original Layer message to observers
    EmitMessage(message);
  }

}
