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

#pragma once

#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

namespace OrthancStone
{
  class OrthancSeriesVolumeProgressiveLoader;
  class DicomVolumeImageMPRSlicer;
  class DicomStructureSetLoader;
  class OrthancMultiframeVolumeLoader;

#if ORTHANC_ENABLE_WASM == 1
  class WebAssemblyOracle;
#else
  class ThreadedOracle;
  class LockingEmitter;
#endif

  class LoaderCache
  {
  public:
#if ORTHANC_ENABLE_WASM == 1
    LoaderCache(WebAssemblyOracle& oracle);
#else
    LoaderCache(ThreadedOracle& oracle, LockingEmitter& lockingEmitter);
#endif

    boost::shared_ptr<OrthancSeriesVolumeProgressiveLoader>
      GetSeriesVolumeProgressiveLoader      (std::string seriesUuid);
    boost::shared_ptr<DicomVolumeImageMPRSlicer>
      GetMultiframeDicomVolumeImageMPRSlicer(std::string instanceUuid);
    boost::shared_ptr<DicomStructureSetLoader>
      GetDicomStructureSetLoader            (std::string instanceUuid);

    void ClearCache();

  private:
    
#if ORTHANC_ENABLE_WASM == 1
    WebAssemblyOracle& oracle_;
#else
    ThreadedOracle& oracle_;
    LockingEmitter& lockingEmitter_;
#endif

    std::map<std::string, boost::shared_ptr<OrthancSeriesVolumeProgressiveLoader> >
      seriesVolumeProgressiveLoaders_;
    std::map<std::string, boost::shared_ptr<OrthancMultiframeVolumeLoader> >
      multiframeVolumeLoaders_;
    std::map<std::string, boost::shared_ptr<DicomVolumeImageMPRSlicer> >
      dicomVolumeImageMPRSlicers_;
    std::map<std::string, boost::shared_ptr<DicomStructureSetLoader> >
      dicomStructureSetLoaders_;
  };
}
