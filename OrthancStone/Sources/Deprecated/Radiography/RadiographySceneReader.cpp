/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include "RadiographySceneReader.h"

#include "../Deprecated/Toolbox/DicomFrameConverter.h"

#include <Images/FontRegistry.h>
#include <Images/PngReader.h>
#include <OrthancException.h>
#include <Toolbox.h>

namespace OrthancStone
{

  void RadiographySceneBuilder::Read(const Json::Value& input, Orthanc::ImageAccessor* dicomImage /* takes ownership */,
                                     Deprecated::DicomFrameConverter* dicomFrameConverter  /* takes ownership */,
                                     RadiographyPhotometricDisplayMode preferredPhotometricDisplayMode
                                     )
  {
    dicomImage_.reset(dicomImage);
    dicomFrameConverter_.reset(dicomFrameConverter);
    preferredPhotometricDisplayMode_ = preferredPhotometricDisplayMode;
    Read(input);
  }

  RadiographyDicomLayer* RadiographySceneBuilder::LoadDicom(const std::string& instanceId, unsigned int frame, RadiographyLayer::Geometry* geometry)
  {
    return dynamic_cast<RadiographyDicomLayer*>(&(scene_.LoadDicomImage(dicomImage_.release(), instanceId, frame, dicomFrameConverter_.release(), preferredPhotometricDisplayMode_, geometry)));
  }


  RadiographyDicomLayer* RadiographySceneReader::LoadDicom(const std::string& instanceId, unsigned int frame, RadiographyLayer::Geometry* geometry)
  {
    return dynamic_cast<RadiographyDicomLayer*>(&(scene_.LoadDicomFrame(orthancApiClient_, instanceId, frame, false, geometry)));
  }

  RadiographyDicomLayer* RadiographySceneGeometryReader::LoadDicom(const std::string& instanceId, unsigned int frame, RadiographyLayer::Geometry* geometry)
  {
    std::unique_ptr<RadiographyPlaceholderLayer>  layer(new RadiographyPlaceholderLayer(scene_));
    layer->SetGeometry(*geometry);
    layer->SetSize(dicomImageWidth_, dicomImageHeight_);
    scene_.RegisterLayer(layer.get());

    return layer.release();
  }

  void RadiographySceneBuilder::Read(const Json::Value& input)
  {
    unsigned int version = input["version"].asUInt();

    if (version != 1)
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);

    if (input.isMember("hasWindowing") && input["hasWindowing"].asBool())
    {
      scene_.SetWindowing(input["windowCenter"].asFloat(), input["windowWidth"].asFloat());
    }

    RadiographyDicomLayer* dicomLayer = NULL;
    for(size_t layerIndex = 0; layerIndex < input["layers"].size(); layerIndex++)
    {
      const Json::Value& jsonLayer = input["layers"][(int)layerIndex];
      RadiographyLayer::Geometry geometry;

      if (jsonLayer["type"].asString() == "dicom")
      {
        ReadLayerGeometry(geometry, jsonLayer);
        dicomLayer = LoadDicom(jsonLayer["instanceId"].asString(), jsonLayer["frame"].asUInt(), &geometry);
      }
      else if (jsonLayer["type"].asString() == "mask")
      {
        if (dicomLayer == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError); // we always assumed the dicom layer was read before the mask
        }
        ReadLayerGeometry(geometry, jsonLayer);

        float foreground = jsonLayer["foreground"].asFloat();
        std::vector<Orthanc::ImageProcessing::ImagePoint> corners;
        for (size_t i = 0; i < jsonLayer["corners"].size(); i++)
        {
          Orthanc::ImageProcessing::ImagePoint corner(jsonLayer["corners"][(int)i]["x"].asInt(),
              jsonLayer["corners"][(int)i]["y"].asInt());
          corners.push_back(corner);
        }

        scene_.LoadMask(corners, *dicomLayer, foreground, &geometry);
      }
      else if (jsonLayer["type"].asString() == "text")
      {
        ReadLayerGeometry(geometry, jsonLayer);
        scene_.LoadText(jsonLayer["text"].asString(), jsonLayer["font"].asString(), jsonLayer["fontSize"].asUInt(), static_cast<uint8_t>(jsonLayer["foreground"].asUInt()), &geometry, false);
      }
      else if (jsonLayer["type"].asString() == "alpha")
      {
        ReadLayerGeometry(geometry, jsonLayer);

        const std::string& pngContentBase64 = jsonLayer["content"].asString();
        std::string pngContent;
        std::string mimeType;
        Orthanc::Toolbox::DecodeDataUriScheme(mimeType, pngContent, pngContentBase64);

        std::unique_ptr<Orthanc::ImageAccessor>  image;
        if (mimeType == "image/png")
        {
          image.reset(new Orthanc::PngReader());
          dynamic_cast<Orthanc::PngReader*>(image.get())->ReadFromMemory(pngContent);
        }
        else
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);

        RadiographyAlphaLayer& layer = dynamic_cast<RadiographyAlphaLayer&>(scene_.LoadAlphaBitmap(image.release(), &geometry));

        if (!jsonLayer["isUsingWindowing"].asBool())
        {
          layer.SetForegroundValue((float)(jsonLayer["foreground"].asDouble()));
        }
      }
      else
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }




  void RadiographySceneBuilder::ReadDicomLayerGeometry(RadiographyLayer::Geometry& geometry, const Json::Value& input)
  {
    for(size_t layerIndex = 0; layerIndex < input["layers"].size(); layerIndex++)
    {
      const Json::Value& jsonLayer = input["layers"][(int)layerIndex];
      if (jsonLayer["type"].asString() == "dicom")
      {
        ReadLayerGeometry(geometry, jsonLayer);
        return;
      }
    }
  }

  void RadiographySceneBuilder::ReadLayerGeometry(RadiographyLayer::Geometry& geometry, const Json::Value& jsonLayer)
  {
    {// crop
      unsigned int x, y, width, height;
      if (jsonLayer["crop"]["hasCrop"].asBool())
      {
        x = jsonLayer["crop"]["x"].asUInt();
        y = jsonLayer["crop"]["y"].asUInt();
        width = jsonLayer["crop"]["width"].asUInt();
        height = jsonLayer["crop"]["height"].asUInt();
        geometry.SetCrop(x, y, width, height);
      }
    }

    geometry.SetAngle(jsonLayer["angle"].asDouble());
    geometry.SetResizeable(jsonLayer["isResizable"].asBool());
    geometry.SetPan(jsonLayer["pan"]["x"].asDouble(), jsonLayer["pan"]["y"].asDouble());
    geometry.SetPixelSpacing(jsonLayer["pixelSpacing"]["x"].asDouble(), jsonLayer["pixelSpacing"]["y"].asDouble());

    // these fields were introduced later -> they might not exist
    if (jsonLayer.isMember("flipVertical"))
    {
      geometry.SetFlipVertical(jsonLayer["flipVertical"].asBool());
    }
    if (jsonLayer.isMember("flipHorizontal"))
    {
      geometry.SetFlipHorizontal(jsonLayer["flipHorizontal"].asBool());
    }

  }
}
