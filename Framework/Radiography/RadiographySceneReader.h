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

#include "RadiographyScene.h"
#include "RadiographyAlphaLayer.h"
#include "RadiographyDicomLayer.h"
#include "RadiographyMaskLayer.h"
#include "RadiographyTextLayer.h"
#include "../Deprecated/Toolbox/OrthancApiClient.h"

#include <json/value.h>
#include <Core/Images/FontRegistry.h>

namespace OrthancStone
{
  // HACK: I had to introduce this builder class in order to be able to recreate a RadiographyScene
  // from a serialized scene that is passed to web-workers.
  // It needs some architecturing...
  class RadiographySceneBuilder : public boost::noncopyable
  {
  protected:
    RadiographyScene&                               scene_;
    const Orthanc::FontRegistry*                    fontRegistry_;
    std::auto_ptr<Orthanc::ImageAccessor>           dicomImage_;
    std::auto_ptr<Deprecated::DicomFrameConverter>  dicomFrameConverter_;
    RadiographyPhotometricDisplayMode               preferredPhotometricDisplayMode_;

  public:
    RadiographySceneBuilder(RadiographyScene& scene) :
      scene_(scene),
      fontRegistry_(NULL)
    {
    }

    void Read(const Json::Value& input);
    void Read(const Json::Value& input,
              Orthanc::ImageAccessor* dicomImage, // takes ownership
              Deprecated::DicomFrameConverter* dicomFrameConverter, // takes ownership
              RadiographyPhotometricDisplayMode preferredPhotometricDisplayMode
              );

    void SetFontRegistry(const Orthanc::FontRegistry& fontRegistry)
    {
      fontRegistry_ = &fontRegistry;
    }

  protected:
    void ReadLayerGeometry(RadiographyLayer::Geometry& geometry, const Json::Value& input);
    virtual RadiographyDicomLayer* LoadDicom(const std::string& instanceId, unsigned int frame, RadiographyLayer::Geometry* geometry);
  };


  class RadiographySceneReader : public RadiographySceneBuilder
  {
    Deprecated::OrthancApiClient&             orthancApiClient_;

  public:
    RadiographySceneReader(RadiographyScene& scene, Deprecated::OrthancApiClient& orthancApiClient) :
      RadiographySceneBuilder(scene),
      orthancApiClient_(orthancApiClient)
    {
    }

    void Read(const Json::Value& input);

  protected:
    virtual RadiographyDicomLayer*  LoadDicom(const std::string& instanceId, unsigned int frame, RadiographyLayer::Geometry* geometry);
  };
}
