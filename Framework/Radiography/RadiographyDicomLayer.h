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


#pragma once

#include "RadiographyLayer.h"
#include <Plugins/Samples/Common/FullOrthancDataset.h>

namespace OrthancStone
{
  class RadiographyScene;
  class DicomFrameConverter;

  class RadiographyDicomLayer : public RadiographyLayer
  {
  private:
    std::auto_ptr<Orthanc::ImageAccessor>  source_;  // Content of PixelData
    std::auto_ptr<DicomFrameConverter>     converter_;
    std::auto_ptr<Orthanc::ImageAccessor>  converted_;  // Float32
    std::string                            instanceId_;
    unsigned int                           frame_;

    void ApplyConverter();

  public:
    RadiographyDicomLayer(MessageBroker& broker, const RadiographyScene& scene)
      : RadiographyLayer(broker, scene)
    {
    }

    void SetInstance(const std::string& instanceId, unsigned int frame)
    {
      instanceId_ = instanceId;
      frame_ = frame;
    }

    std::string GetInstanceId() const
    {
      return instanceId_;
    }

    unsigned int GetFrame() const
    {
      return frame_;
    }

    void SetDicomTags(const OrthancPlugins::FullOrthancDataset& dataset);

    void SetSourceImage(Orthanc::ImageAccessor* image);   // Takes ownership

    const Orthanc::ImageAccessor* GetSourceImage() const {return source_.get();}  // currently need this access to serialize scene in plain old data to send to a WASM worker

    const DicomFrameConverter& GetDicomFrameConverter() const {return *converter_;} // currently need this access to serialize scene in plain old data to send to a WASM worker
    void SetDicomFrameConverter(DicomFrameConverter* converter) {converter_.reset(converter);} // Takes ownership

    virtual void Render(Orthanc::ImageAccessor& buffer,
                        const AffineTransform2D& viewTransform,
                        ImageInterpolation interpolation) const;

    virtual bool GetDefaultWindowing(float& center,
                                     float& width) const;

    virtual bool GetRange(float& minValue,
                          float& maxValue) const;
  };
}
