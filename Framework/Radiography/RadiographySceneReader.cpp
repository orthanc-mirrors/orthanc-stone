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


#include "RadiographySceneReader.h"

#include <Core/Images/FontRegistry.h>
#include <Core/Images/PngReader.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

namespace OrthancStone
{

  void RadiographySceneBuilder::Read(const Json::Value& input, Orthanc::ImageAccessor* dicomImage /* takes ownership */,
                                     DicomFrameConverter* dicomFrameConverter  /* takes ownership */,
                                     PhotometricDisplayMode preferredPhotometricDisplayMode
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

  void RadiographySceneBuilder::Read(const Json::Value& input)
  {
    unsigned int version = input["version"].asUInt();

    if (version != 1)
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);

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
        if (fontRegistry_ == NULL || fontRegistry_->GetSize() == 0)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls); // you must provide a FontRegistry if you need to re-create text layers.
        }

        ReadLayerGeometry(geometry, jsonLayer);
        const Orthanc::Font* font = fontRegistry_->FindFont(jsonLayer["fontName"].asString());
        if (font == NULL) // if not found, take the first font in the registry
        {
          font = &(fontRegistry_->GetFont(0));
        }
        scene_.LoadText(*font, jsonLayer["text"].asString(), &geometry);
      }
      else if (jsonLayer["type"].asString() == "alpha")
      {
        ReadLayerGeometry(geometry, jsonLayer);

        const std::string& pngContentBase64 = jsonLayer["content"].asString();
        std::string pngContent;
        std::string mimeType;
        Orthanc::Toolbox::DecodeDataUriScheme(mimeType, pngContent, pngContentBase64);

        std::auto_ptr<Orthanc::ImageAccessor>  image;
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


  void RadiographySceneReader::Read(const Json::Value& input)
  {
    unsigned int version = input["version"].asUInt();

    if (version != 1)
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);

    RadiographyDicomLayer* dicomLayer = NULL;
    for(size_t layerIndex = 0; layerIndex < input["layers"].size(); layerIndex++)
    {
      const Json::Value& jsonLayer = input["layers"][(int)layerIndex];
      RadiographyLayer::Geometry geometry;

      if (jsonLayer["type"].asString() == "dicom")
      {
        ReadLayerGeometry(geometry, jsonLayer);
        dicomLayer = dynamic_cast<RadiographyDicomLayer*>(&(scene_.LoadDicomFrame(orthancApiClient_, jsonLayer["instanceId"].asString(), jsonLayer["frame"].asUInt(), false, &geometry)));
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
        if (fontRegistry_ == NULL || fontRegistry_->GetSize() == 0)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls); // you must provide a FontRegistry if you need to re-create text layers.
        }

        ReadLayerGeometry(geometry, jsonLayer);
        const Orthanc::Font* font = fontRegistry_->FindFont(jsonLayer["fontName"].asString());
        if (font == NULL) // if not found, take the first font in the registry
        {
          font = &(fontRegistry_->GetFont(0));
        }
        scene_.LoadText(*font, jsonLayer["text"].asString(), &geometry);
      }
      else if (jsonLayer["type"].asString() == "alpha")
      {
        ReadLayerGeometry(geometry, jsonLayer);

        const std::string& pngContentBase64 = jsonLayer["content"].asString();
        std::string pngContent;
        std::string mimeType;
        Orthanc::Toolbox::DecodeDataUriScheme(mimeType, pngContent, pngContentBase64);

        std::auto_ptr<Orthanc::ImageAccessor>  image;
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
    if (jsonLayer.isMember("verticalFlip"))
    {
      geometry.SetVerticalFlip(jsonLayer["verticalFlip"].asBool());
    }
    if (jsonLayer.isMember("horizontalFlip"))
    {
      geometry.SetHorizontalFlip(jsonLayer["horizontalFlip"].asBool());
    }

  }
}
