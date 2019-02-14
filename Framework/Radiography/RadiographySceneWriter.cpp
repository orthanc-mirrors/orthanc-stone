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


#include "RadiographySceneWriter.h"

#include <Core/OrthancException.h>
#include <Core/Images/PngWriter.h>
#include <Core/Toolbox.h>

namespace OrthancStone
{
  void RadiographySceneWriter::Write(Json::Value& output, const RadiographyScene& scene)
  {
    output["version"] = 1;
    output["layers"] = Json::arrayValue;

    std::vector<size_t> layersIndexes;
    scene.GetLayersIndexes(layersIndexes);

    for (std::vector<size_t>::iterator itLayerIndex = layersIndexes.begin(); itLayerIndex < layersIndexes.end(); itLayerIndex++)
    {
      Json::Value layer;
      WriteLayer(layer, scene.GetLayer(*itLayerIndex));
      output["layers"].append(layer);
    }
  }

  void RadiographySceneWriter::WriteLayer(Json::Value& output, const RadiographyDicomLayer& layer)
  {
    output["type"] = "dicom";
    output["instanceId"] = layer.GetInstanceId();
    output["frame"] = layer.GetFrame();
  }

  void RadiographySceneWriter::WriteLayer(Json::Value& output, const RadiographyTextLayer& layer)
  {
    output["type"] = "text";
    output["text"] = layer.GetText();
    output["fontName"] = layer.GetFontName();
  }

  void RadiographySceneWriter::WriteLayer(Json::Value& output, const RadiographyMaskLayer& layer)
  {
    output["type"] = "mask";
    output["instanceId"] = layer.GetInstanceId(); // the dicom layer it's being linked to
    output["foreground"] = layer.GetForeground();
    output["corners"] = Json::arrayValue;
    const std::vector<MaskPoint>& corners = layer.GetCorners();
    for (size_t i = 0; i < corners.size(); i++)
    {
      Json::Value corner;
      corner["x"] = corners[i].x;
      corner["y"] = corners[i].y;
      output["corners"].append(corner);
    }
  }

  void RadiographySceneWriter::WriteLayer(Json::Value& output, const RadiographyAlphaLayer& layer)
  {
    output["type"] = "alpha";

    //output["bitmap"] =
    const Orthanc::ImageAccessor& alpha = layer.GetAlpha();

    Orthanc::PngWriter pngWriter;
    std::string pngContent;
    std::string pngContentBase64;
    pngWriter.WriteToMemory(pngContent, alpha);

    Orthanc::Toolbox::EncodeDataUriScheme(pngContentBase64, "image/png", pngContent);
    output["content"] = pngContentBase64;
    output["foreground"] = layer.GetForegroundValue();
    output["isUsingWindowing"] = layer.IsUsingWindowing();
  }

  void RadiographySceneWriter::WriteLayer(Json::Value& output, const RadiographyLayer& layer)
  {
    const RadiographyLayer::Geometry& geometry = layer.GetGeometry();

    {// crop
      Json::Value crop;
      if (geometry.HasCrop())
      {
        unsigned int x, y, width, height;
        geometry.GetCrop(x, y, width, height);
        crop["hasCrop"] = true;
        crop["x"] = x;
        crop["y"] = y;
        crop["width"] = width;
        crop["height"] = height;
      }
      else
      {
        crop["hasCrop"] = false;
      }

      output["crop"] = crop;
    }

    output["angle"] = geometry.GetAngle();
    output["isResizable"] = geometry.IsResizeable();

    {// pan
      Json::Value pan;
      pan["x"] = geometry.GetPanX();
      pan["y"] = geometry.GetPanY();
      output["pan"] = pan;
    }

    {// pixelSpacing
      Json::Value pan;
      pan["x"] = geometry.GetPixelSpacingX();
      pan["y"] = geometry.GetPixelSpacingY();
      output["pixelSpacing"] = pan;
    }

    if (dynamic_cast<const RadiographyTextLayer*>(&layer) != NULL)
    {
      WriteLayer(output, dynamic_cast<const RadiographyTextLayer&>(layer));
    }
    else if (dynamic_cast<const RadiographyDicomLayer*>(&layer) != NULL)
    {
      WriteLayer(output, dynamic_cast<const RadiographyDicomLayer&>(layer));
    }
    else if (dynamic_cast<const RadiographyAlphaLayer*>(&layer) != NULL)
    {
      WriteLayer(output, dynamic_cast<const RadiographyAlphaLayer&>(layer));
    }
    else if (dynamic_cast<const RadiographyMaskLayer*>(&layer) != NULL)
    {
      WriteLayer(output, dynamic_cast<const RadiographyMaskLayer&>(layer));
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }
}
