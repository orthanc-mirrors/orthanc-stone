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
#include <map>

#include "Layers/DicomSeriesVolumeSlicer.h"
#include "../Messages/IObservable.h"
#include "Toolbox/OrthancApiClient.h"

namespace Deprecated
{
  class SliceViewerWidget;

  class SmartLoader : public OrthancStone::IObservable, public OrthancStone::IObserver
  {
    class CachedSlice;

  protected:
    typedef std::map<std::string, boost::shared_ptr<SmartLoader::CachedSlice> > CachedSlices;
    CachedSlices cachedSlices_;

    typedef std::map<std::string, boost::shared_ptr<IVolumeSlicer> > PreloadingInstances;
    PreloadingInstances preloadingInstances_;

    OrthancStone::SliceImageQuality     imageQuality_;
    OrthancApiClient&     orthancApiClient_;

  public:
    SmartLoader(OrthancStone::MessageBroker& broker, OrthancApiClient& orthancApiClient);  // TODO: add maxPreloadStorageSizeInBytes

//    void PreloadStudy(const std::string studyId);
//    void PreloadSeries(const std::string seriesId);
    void PreloadSlice(const std::string instanceId, unsigned int frame);

    void SetImageQuality(OrthancStone::SliceImageQuality imageQuality) { imageQuality_ = imageQuality; }

    void SetFrameInWidget(SliceViewerWidget& sliceViewer, size_t layerIndex, const std::string& instanceId, unsigned int frame);

    void GetFirstInstanceIdForSeries(std::string& output, const std::string& seriesId);

  private:
    void OnLayerGeometryReady(const IVolumeSlicer::GeometryReadyMessage& message);
    void OnFrameReady(const DicomSeriesVolumeSlicer::FrameReadyMessage& message);
    void OnLayerReady(const IVolumeSlicer::LayerReadyMessage& message);

  };

}
