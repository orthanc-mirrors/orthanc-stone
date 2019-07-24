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

#include "../Messages/LockingEmitter.h"
#include "../Volumes/DicomVolumeImage.h"
#include "../Volumes/DicomVolumeImageMPRSlicer.h"

#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

namespace OrthancStone
{
#if ORTHANC_ENABLE_WASM == 1
  LoaderCache::LoaderCache(IOracle& oracle)
    : oracle_(oracle)
  {

  }
#else
  LoaderCache::LoaderCache(IOracle& oracle, LockingEmitter& lockingEmitter)
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
        boost::shared_ptr<DicomVolumeImage> volumeImage(new DicomVolumeImage);
        boost::shared_ptr<OrthancSeriesVolumeProgressiveLoader> loader;

        {
#if ORTHANC_ENABLE_WASM == 1
          loader.reset(new OrthancSeriesVolumeProgressiveLoader(volumeImage, GetParent()->GetOracleRef(), GetParent()->GetOracleRef()));
#else
          LockingEmitter::WriterLock lock(lockingEmitter_);
          loader.reset(new OrthancSeriesVolumeProgressiveLoader(volumeImage, oracle_, lock.GetOracleObservable()));
#endif
          loader->LoadSeries(seriesUuid);
        }
        seriesVolumeProgressiveLoaders_[seriesUuid] = loader;
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
          loader.reset(new OrthancMultiframeVolumeLoader(volumeImage, GetParent()->GetOracleRef(), GetParent()->GetOracleRef()));
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
          loader.reset(new DicomStructureSetLoader(volumeImage, GetParent()->GetOracleRef(), GetParent()->GetOracleRef()));
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
    seriesVolumeProgressiveLoaders_.clear();
    multiframeVolumeLoaders_.clear();
    dicomVolumeImageMPRSlicers_.clear();
    dicomStructureSetLoaders_.clear();
  }

}
