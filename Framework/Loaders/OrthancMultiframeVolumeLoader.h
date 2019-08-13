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

#include "LoaderStateMachine.h"
#include "../Volumes/DicomVolumeImage.h"

#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  class OrthancMultiframeVolumeLoader :
    public LoaderStateMachine,
    public IObservable,
    public IGeometryProvider
  {
  private:
    class LoadRTDoseGeometry;
    class LoadGeometry;
    class LoadTransferSyntax;    
    class LoadUncompressedPixelData;

    boost::shared_ptr<DicomVolumeImage>  volume_;
    std::string                          instanceId_;
    std::string                          transferSyntaxUid_;
    bool                                 pixelDataLoaded_;

    const std::string& GetInstanceId() const;

    void ScheduleFrameDownloads();

    void SetTransferSyntax(const std::string& transferSyntax);

    void SetGeometry(const Orthanc::DicomMap& dicom);

    template <typename T>
    void CopyPixelData(const std::string& pixelData);

    void SetUncompressedPixelData(const std::string& pixelData);

    virtual bool HasGeometry() const ORTHANC_OVERRIDE;
    virtual const VolumeImageGeometry& GetImageGeometry() const ORTHANC_OVERRIDE;

  public:
    OrthancMultiframeVolumeLoader(boost::shared_ptr<DicomVolumeImage> volume,
                                  IOracle& oracle,
                                  IObservable& oracleObservable);
    
    virtual ~OrthancMultiframeVolumeLoader();

    bool IsPixelDataLoaded() const
    {
      return pixelDataLoaded_;
    }

    void LoadInstance(const std::string& instanceId);
  };
}
