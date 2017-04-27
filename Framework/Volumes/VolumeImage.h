/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "ISliceableVolume.h"
#include "ImageBuffer3D.h"
#include "../Toolbox/ISeriesLoader.h"
#include "../Toolbox/MessagingToolbox.h"
#include "../Toolbox/ObserversRegistry.h"
#include "../Layers/ILayerRendererFactory.h"

#include <boost/thread.hpp>

namespace OrthancStone
{
  class VolumeImage : public ISliceableVolume
  {
  public:
    class IDownloadPolicy : public boost::noncopyable
    {
    public:
      virtual ~IDownloadPolicy()
      {
      }
      
      virtual void Initialize(ImageBuffer3D& buffer,
                              ISeriesLoader& loader) = 0;

      virtual void Finalize() = 0;

      // Must return "true" if the thread has completed its task. Pay
      // attention that this method can be invoked concurrently by
      // several download threads.
      virtual bool DownloadStep(bool& complete) = 0;

      virtual bool IsFullQualityAxial(size_t slice) = 0;
    };


  private:
    std::auto_ptr<ISeriesLoader>                  loader_;
    std::auto_ptr<ImageBuffer3D>                  buffer_;
    std::vector<boost::thread*>                   threads_;
    bool                                          started_;
    bool                                          continue_;
    ObserversRegistry<ISliceableVolume>           observers_;
    bool                                          loadingComplete_;
    MessagingToolbox::Timestamp                   lastUpdate_;
    std::auto_ptr<OrthancPlugins::IDicomDataset>  referenceDataset_;
    std::auto_ptr<IDownloadPolicy>                policy_;
    
    std::auto_ptr<ParallelSlices>        axialGeometry_;
    std::auto_ptr<ParallelSlices>        coronalGeometry_;
    std::auto_ptr<ParallelSlices>        sagittalGeometry_;

    void StoreUpdateTime();

    void NotifyChange(bool force);

    static void LoadThread(VolumeImage* that);

    bool DetectProjection(VolumeProjection& projection,
                          bool& reverse,
                          const SliceGeometry& viewportSlice);

    const ParallelSlices& GetGeometryInternal(VolumeProjection projection);

  public:
    VolumeImage(ISeriesLoader* loader);   // Takes ownership

    virtual ~VolumeImage();

    void SetDownloadPolicy(IDownloadPolicy* policy);   // Takes ownership

    void SetThreadCount(size_t count);

    size_t GetThreadCount() const
    {
      return threads_.size();
    }

    virtual void Register(IChangeObserver& observer);

    virtual void Unregister(IChangeObserver& observer);

    virtual void Start();

    virtual void Stop();

    ParallelSlices* GetGeometry(VolumeProjection projection,
                                bool reverse);

    Vector GetVoxelDimensions(VolumeProjection projection)
    {
      return buffer_->GetVoxelDimensions(projection);
    }

    bool IsLoadingComplete() const
    {
      return loadingComplete_;
    }

    class LayerFactory : public ILayerRendererFactory
    {
    private:
      VolumeImage&  that_;

    public:
      LayerFactory(VolumeImage& that) :
        that_(that)
      {
      }

      virtual bool HasSourceVolume() const
      {
        return true;
      }

      virtual ISliceableVolume& GetSourceVolume() const
      {
        return that_;
      }

      virtual bool GetExtent(double& x1,
                             double& y1,
                             double& x2,
                             double& y2,
                             const SliceGeometry& viewportSlice);

      virtual ILayerRenderer* CreateLayerRenderer(const SliceGeometry& viewportSlice);    
    };
  };
}
