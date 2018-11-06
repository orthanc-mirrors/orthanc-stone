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
#include "Core/Logging.h"

namespace OrthancStone
{
  enum CachedSliceStatus
  {
    CachedSliceStatus_ScheduledToLoad,
    CachedSliceStatus_GeometryLoaded,
    CachedSliceStatus_ImageLoaded
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
    CachedSlice(MessageBroker& broker) :
    LayerSourceBase(broker)
    {
    }

    virtual ~CachedSlice()
    {
    }

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

      // it has already been loaded -> trigger the "layer ready" message immediately otherwise, do nothing now.  The LayerReady will be triggered
      // once the LayerSource is ready
      if (status_ == CachedSliceStatus_ImageLoaded)
      {
        LOG(WARNING) << "ScheduleLayerCreation for CachedSlice (image is loaded): " << slice_->GetOrthancInstanceId();
        bool isFull = (effectiveQuality_ == SliceImageQuality_FullPng ||
                       effectiveQuality_ == SliceImageQuality_FullPam);
        LayerSourceBase::NotifyLayerReady(FrameRenderer::CreateRenderer(*image_, *slice_, isFull),
                                          slice_->GetGeometry());
      }
      else
      {
        LOG(WARNING) << "ScheduleLayerCreation for CachedSlice (image is not loaded yet): " << slice_->GetOrthancInstanceId();
      }
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


  SmartLoader::SmartLoader(MessageBroker& broker,  
                           OrthancApiClient& orthancApiClient) :
    IObservable(broker),
    IObserver(broker),
    imageQuality_(SliceImageQuality_FullPam),
    orthancApiClient_(orthancApiClient)
  {
  }

  void SmartLoader::SetFrameInWidget(LayerWidget& layerWidget, 
                                     size_t layerIndex, 
                                     const std::string& instanceId, 
                                     unsigned int frame)
  {
    // TODO: check if this frame has already been loaded or is already being loaded.
    // - if already loaded: create a "clone" that will emit the GeometryReady/ImageReady messages "immediately"
    //   (it can not be immediate because Observers needs to register first and this is done after this method returns)
    // - if currently loading, we need to return an object that will observe the existing LayerSource and forward
    //   the messages to its observables
    // in both cases, we must be carefull about objects lifecycle !!!

    std::auto_ptr<ILayerSource> layerSource;
    std::string sliceKeyId = instanceId + ":" + boost::lexical_cast<std::string>(frame);
    SmartLoader::CachedSlice* cachedSlice = NULL;

    if (cachedSlices_.find(sliceKeyId) != cachedSlices_.end()) // && cachedSlices_[sliceKeyId]->status_ == CachedSliceStatus_Loaded)
    {
      layerSource.reset(cachedSlices_[sliceKeyId]->Clone());
      cachedSlice = dynamic_cast<SmartLoader::CachedSlice*>(layerSource.get());
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

    if (cachedSlice != NULL)
    {
      cachedSlice->NotifyGeometryReady();
    }

  }

  void SmartLoader::PreloadSlice(const std::string instanceId, 
                                 unsigned int frame)
  {
    // TODO: reactivate -> need to be able to ScheduleLayerLoading in ILayerSource without calling ScheduleLayerCreation
    return;
    // TODO: check if it is already in the cache



    // create the slice in the cache with "empty" data
    boost::shared_ptr<CachedSlice> cachedSlice(new CachedSlice(IObserver::broker_));
    cachedSlice->slice_.reset(new Slice(instanceId, frame));
    cachedSlice->status_ = CachedSliceStatus_ScheduledToLoad;
    std::string sliceKeyId = instanceId + ":" + boost::lexical_cast<std::string>(frame);

    LOG(WARNING) << "Will preload: " << sliceKeyId;

    cachedSlices_[sliceKeyId] = boost::shared_ptr<CachedSlice>(cachedSlice);

    std::auto_ptr<ILayerSource> layerSource(new OrthancFrameLayerSource(IObserver::broker_, orthancApiClient_));

    dynamic_cast<OrthancFrameLayerSource*>(layerSource.get())->SetImageQuality(imageQuality_);
    layerSource->RegisterObserverCallback(new Callable<SmartLoader, ILayerSource::GeometryReadyMessage>(*this, &SmartLoader::OnLayerGeometryReady));
    layerSource->RegisterObserverCallback(new Callable<SmartLoader, ILayerSource::ImageReadyMessage>(*this, &SmartLoader::OnImageReady));
    layerSource->RegisterObserverCallback(new Callable<SmartLoader, ILayerSource::LayerReadyMessage>(*this, &SmartLoader::OnLayerReady));
    dynamic_cast<OrthancFrameLayerSource*>(layerSource.get())->LoadFrame(instanceId, frame);

    // keep a ref to the LayerSource until the slice is fully loaded and saved to cache
    preloadingInstances_[sliceKeyId] = boost::shared_ptr<ILayerSource>(layerSource.release());
  }


//  void PreloadStudy(const std::string studyId)
//  {
//    /* TODO */
//  }

//  void PreloadSeries(const std::string seriesId)
//  {
//    /* TODO */
//  }


  void SmartLoader::OnLayerGeometryReady(const ILayerSource::GeometryReadyMessage& message)
  {
    OrthancFrameLayerSource& source = dynamic_cast<OrthancFrameLayerSource&>(message.GetOrigin());

    // save/replace the slice in cache
    const Slice& slice = source.GetSlice(0); // TODO handle GetSliceCount()
    std::string sliceKeyId = (slice.GetOrthancInstanceId() + ":" + 
                              boost::lexical_cast<std::string>(slice.GetFrame()));

    LOG(WARNING) << "Geometry ready: " << sliceKeyId;

    boost::shared_ptr<CachedSlice> cachedSlice(new CachedSlice(IObserver::broker_));
    cachedSlice->slice_.reset(slice.Clone());
    cachedSlice->effectiveQuality_ = source.GetImageQuality();
    cachedSlice->status_ = CachedSliceStatus_GeometryLoaded;

    cachedSlices_[sliceKeyId] = boost::shared_ptr<CachedSlice>(cachedSlice);

    // re-emit original Layer message to observers
    EmitMessage(message);
  }


  void SmartLoader::OnImageReady(const ILayerSource::ImageReadyMessage& message)
  {
    OrthancFrameLayerSource& source = dynamic_cast<OrthancFrameLayerSource&>(message.GetOrigin());

    // save/replace the slice in cache
    const Slice& slice = source.GetSlice(0); // TODO handle GetSliceCount() ?
    std::string sliceKeyId = (slice.GetOrthancInstanceId() + ":" + 
                              boost::lexical_cast<std::string>(slice.GetFrame()));

    LOG(WARNING) << "Image ready: " << sliceKeyId;

    boost::shared_ptr<CachedSlice> cachedSlice(new CachedSlice(IObserver::broker_));
    cachedSlice->image_.reset(Orthanc::Image::Clone(message.GetImage()));
    cachedSlice->effectiveQuality_ = message.GetImageQuality();
    cachedSlice->slice_.reset(message.GetSlice().Clone());
    cachedSlice->status_ = CachedSliceStatus_ImageLoaded;

    cachedSlices_[sliceKeyId] = cachedSlice;

    // re-emit original Layer message to observers
    EmitMessage(message);
  }


  void SmartLoader::OnLayerReady(const ILayerSource::LayerReadyMessage& message)
  {
    OrthancFrameLayerSource& source = dynamic_cast<OrthancFrameLayerSource&>(message.GetOrigin());
    const Slice& slice = source.GetSlice(0); // TODO handle GetSliceCount() ?
    std::string sliceKeyId = (slice.GetOrthancInstanceId() + ":" + 
                              boost::lexical_cast<std::string>(slice.GetFrame()));

    LOG(WARNING) << "Layer ready: " << sliceKeyId;

    // remove the slice from the preloading slices now that it has been fully loaded and it is referenced in the cache
    if (preloadingInstances_.find(sliceKeyId) != preloadingInstances_.end())
    {
      preloadingInstances_.erase(sliceKeyId);
    }

    // re-emit original Layer message to observers
    EmitMessage(message);
  }
}
