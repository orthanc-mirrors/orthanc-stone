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


#include "SmartLoader.h"

#include "../Messages/MessageForwarder.h"
#include "../StoneException.h"
#include "Core/Images/Image.h"
#include "Core/Logging.h"
#include "Layers/DicomSeriesVolumeSlicer.h"
#include "Layers/FrameRenderer.h"
#include "Widgets/SliceViewerWidget.h"

namespace Deprecated
{
  enum CachedSliceStatus
  {
    CachedSliceStatus_ScheduledToLoad,
    CachedSliceStatus_GeometryLoaded,
    CachedSliceStatus_ImageLoaded
  };

  class SmartLoader::CachedSlice : public IVolumeSlicer
  {
  public:
    class RendererFactory : public LayerReadyMessage::IRendererFactory
    {
    private:
      const CachedSlice&  that_;

    public:
      RendererFactory(const CachedSlice& that) :
        that_(that)
      {
      }

      virtual ILayerRenderer* CreateRenderer() const
      {
        bool isFull = (that_.effectiveQuality_ == SliceImageQuality_FullPng ||
                       that_.effectiveQuality_ == SliceImageQuality_FullPam);

        return FrameRenderer::CreateRenderer(*that_.image_, *that_.slice_, isFull);
      }
    };
    
    unsigned int                    sliceIndex_;
    std::unique_ptr<Slice>            slice_;
    boost::shared_ptr<Orthanc::ImageAccessor>   image_;
    SliceImageQuality               effectiveQuality_;
    CachedSliceStatus               status_;

  public:
    CachedSlice(OrthancStone::MessageBroker& broker) :
    IVolumeSlicer(broker)
    {
    }

    virtual ~CachedSlice()
    {
    }

    virtual bool GetExtent(std::vector<OrthancStone::Vector>& points,
                           const OrthancStone::CoordinateSystem3D& viewportSlice)
    {
      // TODO: viewportSlice is not used !!!!
      slice_->GetExtent(points);
      return true;
    }

    virtual void ScheduleLayerCreation(const OrthancStone::CoordinateSystem3D& viewportSlice)
    {
      // TODO: viewportSlice is not used !!!!

      // it has already been loaded -> trigger the "layer ready" message immediately otherwise, do nothing now.  The LayerReady will be triggered
      // once the VolumeSlicer is ready
      if (status_ == CachedSliceStatus_ImageLoaded)
      {
        LOG(WARNING) << "ScheduleLayerCreation for CachedSlice (image is loaded): " << slice_->GetOrthancInstanceId();

        RendererFactory factory(*this);   
        BroadcastMessage(IVolumeSlicer::LayerReadyMessage(*this, factory, slice_->GetGeometry()));
      }
      else
      {
        LOG(WARNING) << "ScheduleLayerCreation for CachedSlice (image is not loaded yet): " << slice_->GetOrthancInstanceId();
      }
    }

    CachedSlice* Clone() const
    {
      CachedSlice* output = new CachedSlice(GetBroker());
      output->sliceIndex_ = sliceIndex_;
      output->slice_.reset(slice_->Clone());
      output->image_ = image_;
      output->effectiveQuality_ = effectiveQuality_;
      output->status_ = status_;

      return output;
    }

  };


  SmartLoader::SmartLoader(OrthancStone::MessageBroker& broker,  
                           OrthancApiClient& orthancApiClient) :
    IObservable(broker),
    IObserver(broker),
    imageQuality_(SliceImageQuality_FullPam),
    orthancApiClient_(orthancApiClient)
  {
  }

  void SmartLoader::SetFrameInWidget(SliceViewerWidget& sliceViewer, 
                                     size_t layerIndex, 
                                     const std::string& instanceId, 
                                     unsigned int frame)
  {
    // TODO: check if this frame has already been loaded or is already being loaded.
    // - if already loaded: create a "clone" that will emit the GeometryReady/ImageReady messages "immediately"
    //   (it can not be immediate because Observers needs to register first and this is done after this method returns)
    // - if currently loading, we need to return an object that will observe the existing VolumeSlicer and forward
    //   the messages to its observables
    // in both cases, we must be carefull about objects lifecycle !!!

    std::unique_ptr<IVolumeSlicer> layerSource;
    std::string sliceKeyId = instanceId + ":" + boost::lexical_cast<std::string>(frame);
    SmartLoader::CachedSlice* cachedSlice = NULL;

    if (cachedSlices_.find(sliceKeyId) != cachedSlices_.end()) // && cachedSlices_[sliceKeyId]->status_ == CachedSliceStatus_Loaded)
    {
      layerSource.reset(cachedSlices_[sliceKeyId]->Clone());
      cachedSlice = dynamic_cast<SmartLoader::CachedSlice*>(layerSource.get());
    }
    else
    {
      layerSource.reset(new DicomSeriesVolumeSlicer(IObserver::GetBroker(), orthancApiClient_));
      dynamic_cast<DicomSeriesVolumeSlicer*>(layerSource.get())->SetImageQuality(imageQuality_);
      layerSource->RegisterObserverCallback(new OrthancStone::Callable<SmartLoader, IVolumeSlicer::GeometryReadyMessage>(*this, &SmartLoader::OnLayerGeometryReady));
      layerSource->RegisterObserverCallback(new OrthancStone::Callable<SmartLoader, DicomSeriesVolumeSlicer::FrameReadyMessage>(*this, &SmartLoader::OnFrameReady));
      layerSource->RegisterObserverCallback(new OrthancStone::Callable<SmartLoader, IVolumeSlicer::LayerReadyMessage>(*this, &SmartLoader::OnLayerReady));
      dynamic_cast<DicomSeriesVolumeSlicer*>(layerSource.get())->LoadFrame(instanceId, frame);
    }

    // make sure that the widget registers the events before we trigger them
    if (sliceViewer.GetLayerCount() == layerIndex)
    {
      sliceViewer.AddLayer(layerSource.release());
    }
    else if (sliceViewer.GetLayerCount() > layerIndex)
    {
      sliceViewer.ReplaceLayer(layerIndex, layerSource.release());
    }
    else
    {
      throw OrthancStone::StoneException(OrthancStone::ErrorCode_CanOnlyAddOneLayerAtATime);
    }

    if (cachedSlice != NULL)
    {
      BroadcastMessage(IVolumeSlicer::GeometryReadyMessage(*cachedSlice));
    }

  }

  void SmartLoader::PreloadSlice(const std::string instanceId, 
                                 unsigned int frame)
  {
    // TODO: reactivate -> need to be able to ScheduleLayerLoading in IVolumeSlicer without calling ScheduleLayerCreation
    return;
    // TODO: check if it is already in the cache



    // create the slice in the cache with "empty" data
    boost::shared_ptr<CachedSlice> cachedSlice(new CachedSlice(IObserver::GetBroker()));
    cachedSlice->slice_.reset(new Slice(instanceId, frame));
    cachedSlice->status_ = CachedSliceStatus_ScheduledToLoad;
    std::string sliceKeyId = instanceId + ":" + boost::lexical_cast<std::string>(frame);

    LOG(WARNING) << "Will preload: " << sliceKeyId;

    cachedSlices_[sliceKeyId] = boost::shared_ptr<CachedSlice>(cachedSlice);

    std::unique_ptr<IVolumeSlicer> layerSource(new DicomSeriesVolumeSlicer(IObserver::GetBroker(), orthancApiClient_));

    dynamic_cast<DicomSeriesVolumeSlicer*>(layerSource.get())->SetImageQuality(imageQuality_);
    layerSource->RegisterObserverCallback(new OrthancStone::Callable<SmartLoader, IVolumeSlicer::GeometryReadyMessage>(*this, &SmartLoader::OnLayerGeometryReady));
    layerSource->RegisterObserverCallback(new OrthancStone::Callable<SmartLoader, DicomSeriesVolumeSlicer::FrameReadyMessage>(*this, &SmartLoader::OnFrameReady));
    layerSource->RegisterObserverCallback(new OrthancStone::Callable<SmartLoader, IVolumeSlicer::LayerReadyMessage>(*this, &SmartLoader::OnLayerReady));
    dynamic_cast<DicomSeriesVolumeSlicer*>(layerSource.get())->LoadFrame(instanceId, frame);

    // keep a ref to the VolumeSlicer until the slice is fully loaded and saved to cache
    preloadingInstances_[sliceKeyId] = boost::shared_ptr<IVolumeSlicer>(layerSource.release());
  }


//  void PreloadStudy(const std::string studyId)
//  {
//    /* TODO */
//  }

//  void PreloadSeries(const std::string seriesId)
//  {
//    /* TODO */
//  }


  void SmartLoader::OnLayerGeometryReady(const IVolumeSlicer::GeometryReadyMessage& message)
  {
    const DicomSeriesVolumeSlicer& source =
      dynamic_cast<const DicomSeriesVolumeSlicer&>(message.GetOrigin());

    // save/replace the slice in cache
    const Slice& slice = source.GetSlice(0); // TODO handle GetSliceCount()
    std::string sliceKeyId = (slice.GetOrthancInstanceId() + ":" + 
                              boost::lexical_cast<std::string>(slice.GetFrame()));

    LOG(WARNING) << "Geometry ready: " << sliceKeyId;

    boost::shared_ptr<CachedSlice> cachedSlice(new CachedSlice(IObserver::GetBroker()));
    cachedSlice->slice_.reset(slice.Clone());
    cachedSlice->effectiveQuality_ = source.GetImageQuality();
    cachedSlice->status_ = CachedSliceStatus_GeometryLoaded;

    cachedSlices_[sliceKeyId] = boost::shared_ptr<CachedSlice>(cachedSlice);

    // re-emit original Layer message to observers
    BroadcastMessage(message);
  }


  void SmartLoader::OnFrameReady(const DicomSeriesVolumeSlicer::FrameReadyMessage& message)
  {
    // save/replace the slice in cache
    const Slice& slice = message.GetSlice();
    std::string sliceKeyId = (slice.GetOrthancInstanceId() + ":" + 
                              boost::lexical_cast<std::string>(slice.GetFrame()));

    LOG(WARNING) << "Image ready: " << sliceKeyId;

    boost::shared_ptr<CachedSlice> cachedSlice(new CachedSlice(IObserver::GetBroker()));
    cachedSlice->image_.reset(Orthanc::Image::Clone(message.GetFrame()));
    cachedSlice->effectiveQuality_ = message.GetImageQuality();
    cachedSlice->slice_.reset(message.GetSlice().Clone());
    cachedSlice->status_ = CachedSliceStatus_ImageLoaded;

    cachedSlices_[sliceKeyId] = cachedSlice;

    // re-emit original Layer message to observers
    BroadcastMessage(message);
  }


  void SmartLoader::OnLayerReady(const IVolumeSlicer::LayerReadyMessage& message)
  {
    const DicomSeriesVolumeSlicer& source =
      dynamic_cast<const DicomSeriesVolumeSlicer&>(message.GetOrigin());
    
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
    BroadcastMessage(message);
  }
}
