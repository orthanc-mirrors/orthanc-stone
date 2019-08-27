/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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

#include "LoaderCache.h"

#include "OrthancSeriesVolumeProgressiveLoader.h"
#include "OrthancMultiframeVolumeLoader.h"
#include "DicomStructureSetLoader.h"

#if ORTHANC_ENABLE_WASM == 1
# include <unistd.h>
# include "../Oracle/WebAssemblyOracle.h"
#else
# include "../Oracle/ThreadedOracle.h"
#endif

#include "../Messages/LockingEmitter.h"
#include "../Volumes/DicomVolumeImage.h"
#include "../Volumes/DicomVolumeImageMPRSlicer.h"

#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

namespace OrthancStone
{
#if ORTHANC_ENABLE_WASM == 1
  LoaderCache::LoaderCache(WebAssemblyOracle& oracle)
    : oracle_(oracle)
  {

  }
#else
  LoaderCache::LoaderCache(ThreadedOracle& oracle, LockingEmitter& lockingEmitter)
    : oracle_(oracle)
    , lockingEmitter_(lockingEmitter)
  {
  }
#endif

  boost::shared_ptr<OrthancStone::OrthancSeriesVolumeProgressiveLoader> 
    LoaderCache::GetSeriesVolumeProgressiveLoader(std::string seriesUuid)
  {
    try
    {
      
      // normalize keys a little
      seriesUuid = Orthanc::Toolbox::StripSpaces(seriesUuid);
      Orthanc::Toolbox::ToLowerCase(seriesUuid);

      // find in cache
      if (seriesVolumeProgressiveLoaders_.find(seriesUuid) == seriesVolumeProgressiveLoaders_.end())
      {
//        LOG(TRACE) << "LoaderCache::GetSeriesVolumeProgressiveLoader : CACHEMISS --> need to load seriesUUid = " << seriesUuid;
#if ORTHANC_ENABLE_WASM == 1
//        LOG(TRACE) << "Performing request for series " << seriesUuid << " sbrk(0) = " << sbrk(0);
#else
//        LOG(TRACE) << "Performing request for series " << seriesUuid;
#endif
        boost::shared_ptr<DicomVolumeImage> volumeImage(new DicomVolumeImage);
        boost::shared_ptr<OrthancSeriesVolumeProgressiveLoader> loader;
//        LOG(TRACE) << "volumeImage = " << volumeImage.get();
        {
#if ORTHANC_ENABLE_WASM == 1
          loader.reset(new OrthancSeriesVolumeProgressiveLoader(volumeImage, oracle_, oracle_));
#else
          LockingEmitter::WriterLock lock(lockingEmitter_);
          loader.reset(new OrthancSeriesVolumeProgressiveLoader(volumeImage, oracle_, lock.GetOracleObservable()));
#endif
//          LOG(TRACE) << "LoaderCache::GetSeriesVolumeProgressiveLoader : loader = " << loader.get();
          loader->LoadSeries(seriesUuid);
//          LOG(TRACE) << "LoaderCache::GetSeriesVolumeProgressiveLoader : loader->LoadSeries successful";
        }
        seriesVolumeProgressiveLoaders_[seriesUuid] = loader;
      }
      else
      {
//        LOG(TRACE) << "LoaderCache::GetSeriesVolumeProgressiveLoader : returning cached loader for seriesUUid = " << seriesUuid;
      }
      return seriesVolumeProgressiveLoaders_[seriesUuid];
    }
    catch (const Orthanc::OrthancException& e)
    {
      if (e.HasDetails())
      {
        LOG(ERROR) << "OrthancException in LoaderCache: " << e.What() << " Details: " << e.GetDetails();
      }
      else
      {
        LOG(ERROR) << "OrthancException in LoaderCache: " << e.What();
      }
      throw;
    }
    catch (const std::exception& e)
    {
      LOG(ERROR) << "std::exception in LoaderCache: " << e.what();
      throw;
    }
    catch (...)
    {
      LOG(ERROR) << "Unknown exception in LoaderCache";
      throw;
    }
  }

  boost::shared_ptr<OrthancMultiframeVolumeLoader> LoaderCache::GetMultiframeVolumeLoader(std::string instanceUuid)
  {
    // if the loader is not available, let's trigger its creation
    if(multiframeVolumeLoaders_.find(instanceUuid) == multiframeVolumeLoaders_.end())
    {
      GetMultiframeDicomVolumeImageMPRSlicer(instanceUuid);
    }
    ORTHANC_ASSERT(multiframeVolumeLoaders_.find(instanceUuid) != multiframeVolumeLoaders_.end());

    return multiframeVolumeLoaders_[instanceUuid];
  }

  boost::shared_ptr<DicomVolumeImageMPRSlicer> LoaderCache::GetMultiframeDicomVolumeImageMPRSlicer(std::string instanceUuid)
  {
    try
    {
      // normalize keys a little
      instanceUuid = Orthanc::Toolbox::StripSpaces(instanceUuid);
      Orthanc::Toolbox::ToLowerCase(instanceUuid);

      // find in cache
      if (dicomVolumeImageMPRSlicers_.find(instanceUuid) == dicomVolumeImageMPRSlicers_.end())
      {
        boost::shared_ptr<DicomVolumeImage> volumeImage(new DicomVolumeImage);
        boost::shared_ptr<OrthancMultiframeVolumeLoader> loader;

        {
#if ORTHANC_ENABLE_WASM == 1
          loader.reset(new OrthancMultiframeVolumeLoader(volumeImage, oracle_, oracle_));
#else
          LockingEmitter::WriterLock lock(lockingEmitter_);
          loader.reset(new OrthancMultiframeVolumeLoader(volumeImage, oracle_, lock.GetOracleObservable()));
#endif
          loader->LoadInstance(instanceUuid);
        }
        multiframeVolumeLoaders_[instanceUuid] = loader;
        boost::shared_ptr<DicomVolumeImageMPRSlicer> mprSlicer(new DicomVolumeImageMPRSlicer(volumeImage));
        dicomVolumeImageMPRSlicers_[instanceUuid] = mprSlicer;
      }
      return dicomVolumeImageMPRSlicers_[instanceUuid];
    }
    catch (const Orthanc::OrthancException& e)
    {
      if (e.HasDetails())
      {
        LOG(ERROR) << "OrthancException in LoaderCache: " << e.What() << " Details: " << e.GetDetails();
      }
      else
      {
        LOG(ERROR) << "OrthancException in LoaderCache: " << e.What();
      }
      throw;
    }
    catch (const std::exception& e)
    {
      LOG(ERROR) << "std::exception in LoaderCache: " << e.what();
      throw;
    }
    catch (...)
    {
      LOG(ERROR) << "Unknown exception in LoaderCache";
      throw;
    }
  }
  
  boost::shared_ptr<DicomStructureSetLoader> LoaderCache::GetDicomStructureSetLoader(std::string instanceUuid)
  {
    try
    {
      // normalize keys a little
      instanceUuid = Orthanc::Toolbox::StripSpaces(instanceUuid);
      Orthanc::Toolbox::ToLowerCase(instanceUuid);

      // find in cache
      if (dicomStructureSetLoaders_.find(instanceUuid) == dicomStructureSetLoaders_.end())
      {
        boost::shared_ptr<DicomStructureSetLoader> loader;

        {
#if ORTHANC_ENABLE_WASM == 1
          loader.reset(new DicomStructureSetLoader(oracle_, oracle_));
#else
          LockingEmitter::WriterLock lock(lockingEmitter_);
          loader.reset(new DicomStructureSetLoader(oracle_, lock.GetOracleObservable()));
#endif
          loader->LoadInstance(instanceUuid);
        }
        dicomStructureSetLoaders_[instanceUuid] = loader;
      }
      return dicomStructureSetLoaders_[instanceUuid];
    }
    catch (const Orthanc::OrthancException& e)
    {
      if (e.HasDetails())
      {
        LOG(ERROR) << "OrthancException in LoaderCache: " << e.What() << " Details: " << e.GetDetails();
      }
      else
      {
        LOG(ERROR) << "OrthancException in LoaderCache: " << e.What();
      }
      throw;
    }
    catch (const std::exception& e)
    {
      LOG(ERROR) << "std::exception in LoaderCache: " << e.what();
      throw;
    }
    catch (...)
    {
      LOG(ERROR) << "Unknown exception in LoaderCache";
      throw;
    }
  }


  void LoaderCache::ClearCache()
  {
#if ORTHANC_ENABLE_WASM != 1
    LockingEmitter::WriterLock lock(lockingEmitter_);
#endif
    
//#ifndef NDEBUG
    // ISO way of checking for debug builds
    DebugDisplayObjRefCounts();
//#endif
    seriesVolumeProgressiveLoaders_.clear();
    multiframeVolumeLoaders_.clear();
    dicomVolumeImageMPRSlicers_.clear();
    dicomStructureSetLoaders_.clear();
  }


  template<typename T> void DebugDisplayObjRefCountsInMap(
    const std::string& name, const std::map<std::string, boost::shared_ptr<T> >& myMap)
  {
    LOG(TRACE) << "Map \"" << name << "\" ref counts:";
    size_t i = 0;
    for (typename std::map<std::string, boost::shared_ptr<T> >::const_iterator 
           it = myMap.begin(); it != myMap.end(); ++it)
    {
      LOG(TRACE) << "  element #" << i << ": ref count = " << it->second.use_count();
      i++;
    }
  }

  void LoaderCache::DebugDisplayObjRefCounts()
  {
    DebugDisplayObjRefCountsInMap("seriesVolumeProgressiveLoaders_", seriesVolumeProgressiveLoaders_);
    DebugDisplayObjRefCountsInMap("multiframeVolumeLoaders_", multiframeVolumeLoaders_);
    DebugDisplayObjRefCountsInMap("dicomVolumeImageMPRSlicers_", dicomVolumeImageMPRSlicers_);
    DebugDisplayObjRefCountsInMap("dicomStructureSetLoaders_", dicomStructureSetLoaders_);
  }
}
