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

#include "../StoneException.h"
#include "OrthancSeriesVolumeProgressiveLoader.h"
#include "OrthancMultiframeVolumeLoader.h"
#include "DicomStructureSetLoader.h"

#ifdef BGO_ENABLE_DICOMSTRUCTURESETLOADER2
#include "DicomStructureSetLoader2.h"
#endif 
 //BGO_ENABLE_DICOMSTRUCTURESETLOADER2


#if ORTHANC_ENABLE_WASM == 1
# include <unistd.h>
# include "../Oracle/WebAssemblyOracle.h"
#else
# include "../Oracle/ThreadedOracle.h"
#endif

#include "../Messages/LockingEmitter.h"

#ifdef BGO_ENABLE_DICOMSTRUCTURESETLOADER2
#include "../Toolbox/DicomStructureSet2.h"
#endif 
//BGO_ENABLE_DICOMSTRUCTURESETLOADER2

#include "../Volumes/DicomVolumeImage.h"
#include "../Volumes/DicomVolumeImageMPRSlicer.h"

#ifdef BGO_ENABLE_DICOMSTRUCTURESETLOADER2
#include "../Volumes/DicomStructureSetSlicer2.h"
#endif 
//BGO_ENABLE_DICOMSTRUCTURESETLOADER2

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
          loader.reset(new OrthancMultiframeVolumeLoader(volumeImage, 
            oracle_, 
            lock.GetOracleObservable()));
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
  
#ifdef BGO_ENABLE_DICOMSTRUCTURESETLOADER2

  boost::shared_ptr<DicomStructureSetSlicer2> LoaderCache::GetDicomStructureSetSlicer2(std::string instanceUuid)
  {
    // if the loader is not available, let's trigger its creation
    if (dicomStructureSetSlicers2_.find(instanceUuid) == dicomStructureSetSlicers2_.end())
    {
      GetDicomStructureSetLoader2(instanceUuid);
    }
    ORTHANC_ASSERT(dicomStructureSetSlicers2_.find(instanceUuid) != dicomStructureSetSlicers2_.end());

    return dicomStructureSetSlicers2_[instanceUuid];
  }
#endif
//BGO_ENABLE_DICOMSTRUCTURESETLOADER2


  /**
  This method allows to convert a list of string into a string by 
  sorting the strings then joining them
  */
  static std::string SortAndJoin(const std::vector<std::string>& stringList)
  {
    if (stringList.size() == 0)
    {
      return "";
    } 
    else
    {
      std::vector<std::string> sortedStringList = stringList;
      std::sort(sortedStringList.begin(), sortedStringList.end());
      std::stringstream s;
      s << sortedStringList[0];
      for (size_t i = 1; i < sortedStringList.size(); ++i)
      {
        s << "-" << sortedStringList[i];
      }
      return s.str();
    }
  }
  
  boost::shared_ptr<DicomStructureSetLoader> 
    LoaderCache::GetDicomStructureSetLoader(
      std::string inInstanceUuid, 
      const std::vector<std::string>& initiallyVisibleStructures)
  {
    try
    {
      // normalize keys a little
      inInstanceUuid = Orthanc::Toolbox::StripSpaces(inInstanceUuid);
      Orthanc::Toolbox::ToLowerCase(inInstanceUuid);

      std::string initiallyVisibleStructuresKey = 
        SortAndJoin(initiallyVisibleStructures);

      std::string entryKey = inInstanceUuid + "_" + initiallyVisibleStructuresKey;

      // find in cache
      if (dicomStructureSetLoaders_.find(entryKey) == dicomStructureSetLoaders_.end())
      {
        boost::shared_ptr<DicomStructureSetLoader> loader;

        {
#if ORTHANC_ENABLE_WASM == 1
          loader.reset(new DicomStructureSetLoader(oracle_, oracle_));
#else
          LockingEmitter::WriterLock lock(lockingEmitter_);
          loader.reset(new DicomStructureSetLoader(oracle_, lock.GetOracleObservable()));
#endif
          loader->LoadInstance(inInstanceUuid, initiallyVisibleStructures);
        }
        dicomStructureSetLoaders_[entryKey] = loader;
      }
      return dicomStructureSetLoaders_[entryKey];
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

#ifdef BGO_ENABLE_DICOMSTRUCTURESETLOADER2

  boost::shared_ptr<DicomStructureSetLoader2> LoaderCache::GetDicomStructureSetLoader2(std::string instanceUuid)
  {
    try
    {
      // normalize keys a little
      instanceUuid = Orthanc::Toolbox::StripSpaces(instanceUuid);
      Orthanc::Toolbox::ToLowerCase(instanceUuid);

      // find in cache
      if (dicomStructureSetLoaders2_.find(instanceUuid) == dicomStructureSetLoaders2_.end())
      {
        boost::shared_ptr<DicomStructureSetLoader2> loader;
        boost::shared_ptr<DicomStructureSet2> structureSet(new DicomStructureSet2());
        boost::shared_ptr<DicomStructureSetSlicer2> rtSlicer(new DicomStructureSetSlicer2(structureSet));
        dicomStructureSetSlicers2_[instanceUuid] = rtSlicer;
        dicomStructureSets2_[instanceUuid] = structureSet; // to prevent it from being deleted
        {
#if ORTHANC_ENABLE_WASM == 1
          loader.reset(new DicomStructureSetLoader2(*(structureSet.get()), oracle_, oracle_));
#else
          LockingEmitter::WriterLock lock(lockingEmitter_);
          // TODO: clarify lifetimes... this is DANGEROUS!
          loader.reset(new DicomStructureSetLoader2(*(structureSet.get()), oracle_, lock.GetOracleObservable()));
#endif
          loader->LoadInstance(instanceUuid);
        }
        dicomStructureSetLoaders2_[instanceUuid] = loader;
      }
      return dicomStructureSetLoaders2_[instanceUuid];
    }
    catch (const Orthanc::OrthancException& e)
    {
      if (e.HasDetails())
      {
        LOG(ERROR) << "OrthancException in GetDicomStructureSetLoader2: " << e.What() << " Details: " << e.GetDetails();
      }
      else
      {
        LOG(ERROR) << "OrthancException in GetDicomStructureSetLoader2: " << e.What();
      }
      throw;
    }
    catch (const std::exception& e)
    {
      LOG(ERROR) << "std::exception in GetDicomStructureSetLoader2: " << e.what();
      throw;
    }
    catch (...)
    {
      LOG(ERROR) << "Unknown exception in GetDicomStructureSetLoader2";
      throw;
    }
  }

#endif
// BGO_ENABLE_DICOMSTRUCTURESETLOADER2


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

#ifdef BGO_ENABLE_DICOMSTRUCTURESETLOADER2
    // order is important!
    dicomStructureSetLoaders2_.clear();
    dicomStructureSetSlicers2_.clear();
    dicomStructureSets2_.clear();
#endif
// BGO_ENABLE_DICOMSTRUCTURESETLOADER2
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
#ifdef BGO_ENABLE_DICOMSTRUCTURESETLOADER2
    DebugDisplayObjRefCountsInMap("dicomStructureSetLoaders2_", dicomStructureSetLoaders2_);
    DebugDisplayObjRefCountsInMap("dicomStructureSetSlicers2_", dicomStructureSetSlicers2_);
#endif
//BGO_ENABLE_DICOMSTRUCTURESETLOADER2
  }
}
