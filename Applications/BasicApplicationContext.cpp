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


#include "BasicApplicationContext.h"

namespace OrthancStone
{
  void BasicApplicationContext::UpdateThread(BasicApplicationContext* that)
  {
    while (!that->stopped_)
    {
      {
        ViewportLocker locker(*that);
        locker.GetViewport().UpdateContent();
      }
      
      boost::this_thread::sleep(boost::posix_time::milliseconds(that->updateDelay_));
    }
  }
  

  BasicApplicationContext::BasicApplicationContext(Orthanc::WebServiceParameters& orthanc) :
    oracle_(viewportMutex_, 4),  // Use 4 threads to download
    //oracle_(viewportMutex_, 1),  // Disable threading to be reproducible
    webService_(oracle_, orthanc),
    stopped_(true),
    updateDelay_(100)   // By default, 100ms between each refresh of the content
  {
    srand(time(NULL)); 
  }


  BasicApplicationContext::~BasicApplicationContext()
  {
    for (Interactors::iterator it = interactors_.begin(); it != interactors_.end(); ++it)
    {
      assert(*it != NULL);
      delete *it;
    }

    for (SlicedVolumes::iterator it = slicedVolumes_.begin(); it != slicedVolumes_.end(); ++it)
    {
      assert(*it != NULL);
      delete *it;
    }

    for (VolumeLoaders::iterator it = volumeLoaders_.begin(); it != volumeLoaders_.end(); ++it)
    {
      assert(*it != NULL);
      delete *it;
    }
  }


  IWidget& BasicApplicationContext::SetCentralWidget(IWidget* widget)   // Takes ownership
  {
    viewport_.SetCentralWidget(widget);
    return *widget;
  }


  ISlicedVolume& BasicApplicationContext::AddSlicedVolume(ISlicedVolume* volume)
  {
    if (volume == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      slicedVolumes_.push_back(volume);
      return *volume;
    }
  }


  IVolumeLoader& BasicApplicationContext::AddVolumeLoader(IVolumeLoader* loader)
  {
    if (loader == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      volumeLoaders_.push_back(loader);
      return *loader;
    }
  }


  IWorldSceneInteractor& BasicApplicationContext::AddInteractor(IWorldSceneInteractor* interactor)
  {
    if (interactor == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    interactors_.push_back(interactor);

    return *interactor;
  }


  void BasicApplicationContext::Start()
  {
    oracle_.Start();

    if (viewport_.HasUpdateContent())
    {
      stopped_ = false;
      updateThread_ = boost::thread(UpdateThread, this);
    }
  }


  void BasicApplicationContext::Stop()
  {
    stopped_ = true;
    
    if (updateThread_.joinable())
    {
      updateThread_.join();
    }
    
    oracle_.Stop();
  }
}
