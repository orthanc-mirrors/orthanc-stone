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


#include "RadiographyScene.h"

#include "RadiographyAlphaLayer.h"
#include "RadiographyDicomLayer.h"
#include "RadiographyTextLayer.h"
#include "RadiographyMaskLayer.h"
#include "../Deprecated/Toolbox/DicomFrameConverter.h"

#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PamReader.h>
#include <Core/Images/PamWriter.h>
#include <Core/Images/PngWriter.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/DicomDatasetReader.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>

#include <boost/math/special_functions/round.hpp>


namespace OrthancStone
{
  RadiographyScene::LayerAccessor::LayerAccessor(RadiographyScene& scene,
                                                 size_t index) :
    scene_(scene),
    index_(index)
  {
    Layers::iterator layer = scene.layers_.find(index);
    if (layer == scene.layers_.end())
    {
      layer_ = NULL;
    }
    else
    {
      assert(layer->second != NULL);
      layer_ = layer->second;
    }
  }


  RadiographyScene::LayerAccessor::LayerAccessor(RadiographyScene& scene,
                                                 double x,
                                                 double y) :
    scene_(scene),
    index_(0)  // Dummy initialization
  {
    if (scene.LookupLayer(index_, x, y))
    {
      Layers::iterator layer = scene.layers_.find(index_);

      if (layer == scene.layers_.end())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      else
      {
        assert(layer->second != NULL);
        layer_ = layer->second;
      }
    }
    else
    {
      layer_ = NULL;
    }
  }


  RadiographyScene& RadiographyScene::LayerAccessor::GetScene() const
  {
    if (IsValid())
    {
      return scene_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  size_t RadiographyScene::LayerAccessor::GetIndex() const
  {
    if (IsValid())
    {
      return index_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  
  RadiographyLayer& RadiographyScene::LayerAccessor::GetLayer() const
  {
    if (IsValid())
    {
      return *layer_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  void RadiographyScene::_RegisterLayer(RadiographyLayer* layer)
  {
    std::auto_ptr<RadiographyLayer> raii(layer);

    // LOG(INFO) << "Registering layer: " << countLayers_;

    size_t index = nextLayerIndex_++;
    raii->SetIndex(index);
    layers_[index] = raii.release();
  }

  RadiographyLayer& RadiographyScene::RegisterLayer(RadiographyLayer* layer)
  {
    if (layer == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    _RegisterLayer(layer);

    BroadcastMessage(GeometryChangedMessage(*this, *layer));
    BroadcastMessage(ContentChangedMessage(*this, *layer));
    layer->RegisterObserverCallback(new Callable<RadiographyScene, RadiographyLayer::LayerEditedMessage>(*this, &RadiographyScene::OnLayerEdited));

    return *layer;
  }

  size_t RadiographyScene::GetApproximateMemoryUsage() const
  {
    size_t size = 0;
    for (Layers::const_iterator it = layers_.begin(); it != layers_.end(); it++)
    {
      size += it->second->GetApproximateMemoryUsage();
    }
    return size;
  }

  void RadiographyScene::OnLayerEdited(const RadiographyLayer::LayerEditedMessage& message)
  {
    BroadcastMessage(RadiographyScene::LayerEditedMessage(*this, message.GetOrigin()));
  }

  RadiographyScene::RadiographyScene(MessageBroker& broker) :
    IObserver(broker),
    IObservable(broker),
    nextLayerIndex_(0),
    hasWindowing_(false),
    windowingCenter_(0),  // Dummy initialization
    windowingWidth_(0)    // Dummy initialization
  {
  }


  RadiographyScene::~RadiographyScene()
  {
    for (Layers::iterator it = layers_.begin(); it != layers_.end(); it++)
    {
      assert(it->second != NULL);
      delete it->second;
    }
  }

  RadiographyPhotometricDisplayMode RadiographyScene::GetPreferredPhotomotricDisplayMode() const
  {
    // return the mode of the first layer who "cares" about its display mode (normaly, the one and only layer that is a DicomLayer)
    for (Layers::const_iterator it = layers_.begin(); it != layers_.end(); it++)
    {
      if (it->second->GetPreferredPhotomotricDisplayMode() != RadiographyPhotometricDisplayMode_Default)
      {
        return it->second->GetPreferredPhotomotricDisplayMode();
      }
    }

    return RadiographyPhotometricDisplayMode_Default;
  }


  void RadiographyScene::GetLayersIndexes(std::vector<size_t>& output) const
  {
    for (Layers::const_iterator it = layers_.begin(); it != layers_.end(); it++)
    {
      output.push_back(it->first);
    }
  }

  void RadiographyScene::RemoveLayer(size_t layerIndex)
  {
    LOG(INFO) << "Removing layer: " << layerIndex;

    Layers::iterator found = layers_.find(layerIndex);

    if (found == layers_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(found->second != NULL);
      delete found->second;
      
      layers_.erase(found);
      
      LOG(INFO) << "Removing layer, there are now : " << layers_.size() << " layers";

      BroadcastMessage(RadiographyScene::LayerRemovedMessage(*this, layerIndex));
    }
  }

  const RadiographyLayer& RadiographyScene::GetLayer(size_t layerIndex) const
  {
    Layers::const_iterator found = layers_.find(layerIndex);
    
    if (found == layers_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(found->second != NULL);
      return *found->second;
    }
  }

  RadiographyLayer& RadiographyScene::GetLayer(size_t layerIndex)
  {
    Layers::const_iterator found = layers_.find(layerIndex);

    if (found == layers_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(found->second != NULL);
      return *found->second;
    }
  }

  bool RadiographyScene::GetWindowing(float& center,
                                      float& width) const
  {
    if (hasWindowing_)
    {
      center = windowingCenter_;
      width = windowingWidth_;
      return true;
    }
    else
    {
      return false;
    }
  }


  void RadiographyScene::GetWindowingWithDefault(float& center,
                                                 float& width) const
  {
    if (!GetWindowing(center, width))
    {
      center = 128;
      width = 256;
    }
  }


  void RadiographyScene::SetWindowing(float center,
                                      float width)
  {
    hasWindowing_ = true;
    windowingCenter_ = center;
    windowingWidth_ = width;

    BroadcastMessage(RadiographyScene::WindowingChangedMessage(*this));
  }


  RadiographyLayer& RadiographyScene::UpdateText(size_t layerIndex,
                                                 const std::string& utf8,
                                                 const std::string& font,
                                                 unsigned int fontSize,
                                                 uint8_t foreground)
  {
    RadiographyTextLayer& textLayer = dynamic_cast<RadiographyTextLayer&>(GetLayer(layerIndex));
    textLayer.SetText(utf8, font, fontSize, foreground);

    BroadcastMessage(RadiographyScene::ContentChangedMessage(*this, textLayer));
    BroadcastMessage(RadiographyScene::LayerEditedMessage(*this, textLayer));
    return textLayer;
  }


  RadiographyLayer& RadiographyScene::LoadText(const std::string& utf8,
                                               const std::string& font,
                                               unsigned int fontSize,
                                               uint8_t foreground,
                                               RadiographyLayer::Geometry* centerGeometry,
                                               bool isCenterGeometry)
  {
    std::auto_ptr<RadiographyTextLayer>  alpha(new RadiographyTextLayer(IObservable::GetBroker(), *this));
    alpha->SetText(utf8, font, fontSize, foreground);
    if (centerGeometry != NULL)
    {
      if (isCenterGeometry)
      {
        // modify geometry to reference the top left corner
        double tlx = centerGeometry->GetPanX();
        double tly = centerGeometry->GetPanY();
        Extent2D textExtent = alpha->GetExtent();
        tlx = tlx - (textExtent.GetWidth() / 2) * centerGeometry->GetPixelSpacingX();
        tly = tly - (textExtent.GetHeight() / 2) * centerGeometry->GetPixelSpacingY();
        centerGeometry->SetPan(tlx, tly);
      }
      alpha->SetGeometry(*centerGeometry);
    }

    RadiographyLayer& registeredLayer = RegisterLayer(alpha.release());

    BroadcastMessage(RadiographyScene::LayerEditedMessage(*this, registeredLayer));
    return registeredLayer;
  }


  RadiographyLayer& RadiographyScene::LoadTestBlock(unsigned int width,
                                                    unsigned int height,
                                                    RadiographyLayer::Geometry* geometry)
  {
    std::auto_ptr<Orthanc::Image>  block(new Orthanc::Image(Orthanc::PixelFormat_Grayscale8, width, height, false));

    for (unsigned int padding = 0;
         (width > 2 * padding) && (height > 2 * padding);
         padding++)
    {
      uint8_t color;
      if (255 > 10 * padding)
      {
        color = 255 - 10 * padding;
      }
      else
      {
        color = 0;
      }

      Orthanc::ImageAccessor region;
      block->GetRegion(region, padding, padding, width - 2 * padding, height - 2 * padding);
      Orthanc::ImageProcessing::Set(region, color);
    }

    return LoadAlphaBitmap(block.release(), geometry);
  }

  RadiographyLayer& RadiographyScene::LoadMask(const std::vector<Orthanc::ImageProcessing::ImagePoint>& corners,
                                               const RadiographyDicomLayer& dicomLayer,
                                               float foreground,
                                               RadiographyLayer::Geometry* geometry)
  {
    std::auto_ptr<RadiographyMaskLayer>  mask(new RadiographyMaskLayer(IObservable::GetBroker(), *this, dicomLayer, foreground));
    mask->SetCorners(corners);
    if (geometry != NULL)
    {
      mask->SetGeometry(*geometry);
    }

    return RegisterLayer(mask.release());
  }


  RadiographyLayer& RadiographyScene::LoadAlphaBitmap(Orthanc::ImageAccessor* bitmap, RadiographyLayer::Geometry *geometry)
  {
    std::auto_ptr<RadiographyAlphaLayer>  alpha(new RadiographyAlphaLayer(IObservable::GetBroker(), *this));
    alpha->SetAlpha(bitmap);
    if (geometry != NULL)
    {
      alpha->SetGeometry(*geometry);
    }

    return RegisterLayer(alpha.release());
  }

  RadiographyLayer& RadiographyScene::LoadDicomImage(Orthanc::ImageAccessor* dicomImage, // takes ownership
                                                     const std::string& instance,
                                                     unsigned int frame,
                                                     Deprecated::DicomFrameConverter* converter,  // takes ownership
                                                     RadiographyPhotometricDisplayMode preferredPhotometricDisplayMode,
                                                     RadiographyLayer::Geometry* geometry)
  {
    RadiographyDicomLayer& layer = dynamic_cast<RadiographyDicomLayer&>(RegisterLayer(new RadiographyDicomLayer(IObservable::GetBroker(), *this)));

    layer.SetInstance(instance, frame);

    if (geometry != NULL)
    {
      layer.SetGeometry(*geometry);
    }

    layer.SetDicomFrameConverter(converter);
    layer.SetSourceImage(dicomImage);
    layer.SetPreferredPhotomotricDisplayMode(preferredPhotometricDisplayMode);

    return layer;
  }

  RadiographyLayer& RadiographyScene::LoadDicomFrame(Deprecated::OrthancApiClient& orthanc,
                                                     const std::string& instance,
                                                     unsigned int frame,
                                                     bool httpCompression,
                                                     RadiographyLayer::Geometry* geometry)
  {
    RadiographyDicomLayer& layer = dynamic_cast<RadiographyDicomLayer&>(RegisterLayer(new RadiographyDicomLayer(IObservable::GetBroker(), *this)));
    layer.SetInstance(instance, frame);

    if (geometry != NULL)
    {
      layer.SetGeometry(*geometry);
    }

    {
      Deprecated::IWebService::HttpHeaders headers;
      std::string uri = "/instances/" + instance + "/tags";

      orthanc.GetBinaryAsync(
            uri, headers,
            new Callable<RadiographyScene, Deprecated::OrthancApiClient::BinaryResponseReadyMessage>
            (*this, &RadiographyScene::OnTagsReceived), NULL,
            new Orthanc::SingleValueObject<size_t>(layer.GetIndex()));
    }

    {
      Deprecated::IWebService::HttpHeaders headers;
      headers["Accept"] = "image/x-portable-arbitrarymap";

      if (httpCompression)
      {
        headers["Accept-Encoding"] = "gzip";
      }

      std::string uri = ("/instances/" + instance + "/frames/" +
                         boost::lexical_cast<std::string>(frame) + "/image-uint16");

      orthanc.GetBinaryAsync(
            uri, headers,
            new Callable<RadiographyScene, Deprecated::OrthancApiClient::BinaryResponseReadyMessage>
            (*this, &RadiographyScene::OnFrameReceived), NULL,
            new Orthanc::SingleValueObject<size_t>(layer.GetIndex()));
    }

    return layer;
  }


  RadiographyLayer& RadiographyScene::LoadDicomWebFrame(Deprecated::IWebService& web)
  {
    RadiographyLayer& layer = RegisterLayer(new RadiographyDicomLayer(IObservable::GetBroker(), *this));


    return layer;
  }



  void RadiographyScene::OnTagsReceived(const Deprecated::OrthancApiClient::BinaryResponseReadyMessage& message)
  {
    size_t index = dynamic_cast<const Orthanc::SingleValueObject<size_t>&>
        (message.GetPayload()).GetValue();

    LOG(INFO) << "JSON received: " << message.GetUri().c_str()
              << " (" << message.GetAnswerSize() << " bytes) for layer " << index;

    Layers::iterator layer = layers_.find(index);
    if (layer != layers_.end())
    {
      assert(layer->second != NULL);

      OrthancPlugins::FullOrthancDataset dicom(message.GetAnswer(), message.GetAnswerSize());
      dynamic_cast<RadiographyDicomLayer*>(layer->second)->SetDicomTags(dicom);

      float c, w;
      if (!hasWindowing_ &&
          layer->second->GetDefaultWindowing(c, w))
      {
        hasWindowing_ = true;
        windowingCenter_ = c;
        windowingWidth_ = w;
      }

      BroadcastMessage(GeometryChangedMessage(*this, *(layer->second)));
    }
  }


  void RadiographyScene::OnFrameReceived(const Deprecated::OrthancApiClient::BinaryResponseReadyMessage& message)
  {
    size_t index = dynamic_cast<const Orthanc::SingleValueObject<size_t>&>(message.GetPayload()).GetValue();

    LOG(INFO) << "DICOM frame received: " << message.GetUri().c_str()
              << " (" << message.GetAnswerSize() << " bytes) for layer " << index;

    Layers::iterator layer = layers_.find(index);
    if (layer != layers_.end())
    {
      assert(layer->second != NULL);

      std::string content;
      if (message.GetAnswerSize() > 0)
      {
        content.assign(reinterpret_cast<const char*>(message.GetAnswer()), message.GetAnswerSize());
      }

      std::auto_ptr<Orthanc::PamReader> reader(new Orthanc::PamReader);
      reader->ReadFromMemory(content);
      dynamic_cast<RadiographyDicomLayer*>(layer->second)->SetSourceImage(reader.release());

      BroadcastMessage(ContentChangedMessage(*this, *(layer->second)));
    }
  }


  Extent2D RadiographyScene::GetSceneExtent() const
  {
    Extent2D extent;

    for (Layers::const_iterator it = layers_.begin();
         it != layers_.end(); ++it)
    {
      assert(it->second != NULL);
      extent.Union(it->second->GetExtent());
    }

    return extent;
  }


  void RadiographyScene::Render(Orthanc::ImageAccessor& buffer,
                                const AffineTransform2D& viewTransform,
                                ImageInterpolation interpolation,
                                bool applyWindowing) const
  {
    // Render layers in the background-to-foreground order
    for (size_t index = 0; index < nextLayerIndex_; index++)
    {
      try
      {
        Layers::const_iterator it = layers_.find(index);
        if (it != layers_.end())
        {
          assert(it->second != NULL);
          it->second->Render(buffer, viewTransform, interpolation, windowingCenter_, windowingWidth_, applyWindowing);
        }
      }
      catch (Orthanc::OrthancException& ex)
      {
        LOG(ERROR) << "RadiographyScene::Render: " << index << ", OrthancException: " << ex.GetDetails();
        throw ex; // rethrow because we want it to crash to see there's a problem !
      }
      catch (...)
      {
        LOG(ERROR) << "RadiographyScene::Render: " << index << ", unkown exception: ";
        throw; // rethrow because we want it to crash to see there's a problem !
      }
    }
  }


  bool RadiographyScene::LookupLayer(size_t& index /* out */,
                                     double x,
                                     double y) const
  {
    // Render layers in the foreground-to-background order
    for (size_t i = nextLayerIndex_; i > 0; i--)
    {
      index = i - 1;
      Layers::const_iterator it = layers_.find(index);
      if (it != layers_.end())
      {
        assert(it->second != NULL);
        if (it->second->Contains(x, y))
        {
          return true;
        }
      }
    }

    return false;
  }


  void RadiographyScene::DrawBorder(CairoContext& context,
                                    unsigned int layer,
                                    double zoom)
  {
    Layers::const_iterator found = layers_.find(layer);

    if (found != layers_.end())
    {
      context.SetSourceColor(255, 0, 0);
      found->second->DrawBorders(context, zoom);
    }
  }


  void RadiographyScene::GetRange(float& minValue,
                                  float& maxValue) const
  {
    bool first = true;

    for (Layers::const_iterator it = layers_.begin();
         it != layers_.end(); it++)
    {
      assert(it->second != NULL);

      float a, b;
      if (it->second->GetRange(a, b))
      {
        if (first)
        {
          minValue = a;
          maxValue = b;
          first = false;
        }
        else
        {
          minValue = std::min(a, minValue);
          maxValue = std::max(b, maxValue);
        }
      }
    }

    if (first)
    {
      minValue = 0;
      maxValue = 0;
    }
  }

  void RadiographyScene::ExtractLayerFromRenderedScene(Orthanc::ImageAccessor& layer,
                                                       const Orthanc::ImageAccessor& renderedScene,
                                                       size_t layerIndex,
                                                       ImageInterpolation interpolation)
  {
    Extent2D sceneExtent = GetSceneExtent();

    double pixelSpacingX = sceneExtent.GetWidth() / renderedScene.GetWidth();
    double pixelSpacingY = sceneExtent.GetHeight() / renderedScene.GetHeight();

    AffineTransform2D view = AffineTransform2D::Combine(
          AffineTransform2D::CreateScaling(1.0 / pixelSpacingX, 1.0 / pixelSpacingY),
          AffineTransform2D::CreateOffset(-sceneExtent.GetX1(), -sceneExtent.GetY1()));

    AffineTransform2D layerToSceneTransform = AffineTransform2D::Combine(
          view,
          GetLayer(layerIndex).GetTransform());

    AffineTransform2D sceneToLayerTransform = AffineTransform2D::Invert(layerToSceneTransform);
    sceneToLayerTransform.Apply(layer, renderedScene, interpolation, false);
  }

  Orthanc::Image* RadiographyScene::ExportToImage(double pixelSpacingX,
                                                  double pixelSpacingY,
                                                  ImageInterpolation interpolation,
                                                  bool invert,
                                                  int64_t maxValue /* for inversion */,
                                                  bool applyWindowing)
  {
    if (pixelSpacingX <= 0 ||
        pixelSpacingY <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    Extent2D extent = GetSceneExtent();

    int w = boost::math::iround(extent.GetWidth() / pixelSpacingX);
    int h = boost::math::iround(extent.GetHeight() / pixelSpacingY);

    if (w < 0 || h < 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    Orthanc::Image layers(Orthanc::PixelFormat_Float32,
                          static_cast<unsigned int>(w),
                          static_cast<unsigned int>(h), false);

    AffineTransform2D view = AffineTransform2D::Combine(
          AffineTransform2D::CreateScaling(1.0 / pixelSpacingX, 1.0 / pixelSpacingY),
          AffineTransform2D::CreateOffset(-extent.GetX1(), -extent.GetY1()));

    // wipe background before rendering
    if (GetPreferredPhotomotricDisplayMode() == RadiographyPhotometricDisplayMode_Monochrome1)
    {
      Orthanc::ImageProcessing::Set(layers, 65535.0f);
    }
    else
    {
      Orthanc::ImageProcessing::Set(layers, 0);
    }

    Render(layers, view, interpolation, applyWindowing);

    std::auto_ptr<Orthanc::Image> rendered(new Orthanc::Image(Orthanc::PixelFormat_Grayscale16,
                                                              layers.GetWidth(), layers.GetHeight(), false));

    Orthanc::ImageProcessing::Convert(*rendered, layers);
    if (invert)
      Orthanc::ImageProcessing::Invert(*rendered, maxValue);

    return rendered.release();
  }


  Orthanc::Image* RadiographyScene::ExportToCreateDicomRequestAndImage(Json::Value& createDicomRequestContent,
                                                                       const Json::Value& dicomTags,
                                                                       const std::string& parentOrthancId,
                                                                       double pixelSpacingX,
                                                                       double pixelSpacingY,
                                                                       bool invert,
                                                                       ImageInterpolation interpolation)
  {
    LOG(INFO) << "Exporting RadiographyScene to DICOM";

    std::auto_ptr<Orthanc::Image> rendered(ExportToImage(pixelSpacingX, pixelSpacingY, interpolation, false)); // note: we don't invert the image in the pixels data because we'll set the PhotometricDisplayMode correctly in the DICOM tags

    createDicomRequestContent["Tags"] = dicomTags;

    RadiographyPhotometricDisplayMode photometricMode = GetPreferredPhotomotricDisplayMode();
    if ((invert && photometricMode != RadiographyPhotometricDisplayMode_Monochrome2) ||
        (!invert && photometricMode == RadiographyPhotometricDisplayMode_Monochrome1))
    {
      createDicomRequestContent["Tags"]["PhotometricInterpretation"] = "MONOCHROME1";
    }
    else
    {
      createDicomRequestContent["Tags"]["PhotometricInterpretation"] = "MONOCHROME2";
    }

    // WARNING: The order of PixelSpacing is Y/X. We use "%0.8f" to
    // avoid floating-point numbers to grow over 16 characters,
    // which would be invalid according to DICOM standard
    // ("dciodvfy" would complain).
    char buf[32];
    sprintf(buf, "%0.8f\\%0.8f", pixelSpacingY, pixelSpacingX);

    createDicomRequestContent["Tags"]["PixelSpacing"] = buf;

    float center, width;
    if (GetWindowing(center, width))
    {
      createDicomRequestContent["Tags"]["WindowCenter"] =
          boost::lexical_cast<std::string>(boost::math::iround(center));

      createDicomRequestContent["Tags"]["WindowWidth"] =
          boost::lexical_cast<std::string>(boost::math::iround(width));
    }

    if (!parentOrthancId.empty())
    {
      createDicomRequestContent["Parent"] = parentOrthancId;
    }

    return rendered.release();
  }


  void RadiographyScene::ExportToCreateDicomRequest(Json::Value& createDicomRequestContent,
                                                    const Json::Value& dicomTags,
                                                    const std::string& parentOrthancId,
                                                    double pixelSpacingX,
                                                    double pixelSpacingY,
                                                    bool invert,
                                                    ImageInterpolation interpolation,
                                                    bool usePam)
  {
    LOG(INFO) << "Exporting RadiographyScene to DICOM";
    VLOG(1) << "Exporting RadiographyScene to: export to image";

    std::auto_ptr<Orthanc::Image> rendered(ExportToCreateDicomRequestAndImage(createDicomRequestContent, dicomTags, parentOrthancId, pixelSpacingX, pixelSpacingY, invert, interpolation));

    // convert the image into base64 for inclusing in the createDicomRequest
    std::string base64;

    {
      std::string content;

      if (usePam)
      {
        VLOG(1) << "Exporting RadiographyScene: convert to PAM";
        Orthanc::PamWriter writer;
        writer.WriteToMemory(content, *rendered);
      }
      else
      {
        Orthanc::PngWriter writer;
        writer.WriteToMemory(content, *rendered);
      }

      VLOG(1) << "Exporting RadiographyScene: encoding to base64";
      Orthanc::Toolbox::EncodeBase64(base64, content);
    }

    // This is Data URI scheme: https://en.wikipedia.org/wiki/Data_URI_scheme
    createDicomRequestContent["Content"] = ("data:" +
                                            std::string(usePam ? Orthanc::MIME_PAM : Orthanc::MIME_PNG) +
                                            ";base64," + base64);

    VLOG(1) << "Exporting RadiographyScene: create-dicom request is ready";
  }


  void RadiographyScene::ExportDicom(Deprecated::OrthancApiClient& orthanc,
                                     const Json::Value& dicomTags,
                                     const std::string& parentOrthancId,
                                     double pixelSpacingX,
                                     double pixelSpacingY,
                                     bool invert,
                                     ImageInterpolation interpolation,
                                     bool usePam)
  {
    Json::Value createDicomRequestContent;

    ExportToCreateDicomRequest(createDicomRequestContent, dicomTags, parentOrthancId, pixelSpacingX, pixelSpacingY, invert, interpolation, usePam);

    orthanc.PostJsonAsyncExpectJson(
          "/tools/create-dicom", createDicomRequestContent,
          new Callable<RadiographyScene, Deprecated::OrthancApiClient::JsonResponseReadyMessage>
          (*this, &RadiographyScene::OnDicomExported),
          NULL, NULL);

  }


  // Export using PAM is faster than using PNG, but requires Orthanc
  // core >= 1.4.3
  void RadiographyScene::ExportDicom(Deprecated::OrthancApiClient& orthanc,
                                     const Orthanc::DicomMap& dicom,
                                     const std::string& parentOrthancId,
                                     double pixelSpacingX,
                                     double pixelSpacingY,
                                     bool invert,
                                     ImageInterpolation interpolation,
                                     bool usePam)
  {
    std::set<Orthanc::DicomTag> tags;
    dicom.GetTags(tags);

    Json::Value jsonTags = Json::objectValue;

    for (std::set<Orthanc::DicomTag>::const_iterator
         tag = tags.begin(); tag != tags.end(); ++tag)
    {
      const Orthanc::DicomValue& value = dicom.GetValue(*tag);
      if (!value.IsNull() &&
          !value.IsBinary())
      {
        jsonTags[tag->Format()] = value.GetContent();
      }
    }

    ExportDicom(orthanc, jsonTags, parentOrthancId, pixelSpacingX, pixelSpacingY, invert, interpolation, usePam);
  }

  void RadiographyScene::OnDicomExported(const Deprecated::OrthancApiClient::JsonResponseReadyMessage& message)
  {
    LOG(INFO) << "DICOM export was successful: "
              << message.GetJson().toStyledString();
  }


  void RadiographyScene::OnDicomWebReceived(const Deprecated::IWebService::HttpRequestSuccessMessage& message)
  {
    LOG(INFO) << "DICOMweb WADO-RS received: " << message.GetAnswerSize() << " bytes";

    const Deprecated::IWebService::HttpHeaders& h = message.GetAnswerHttpHeaders();
    for (Deprecated::IWebService::HttpHeaders::const_iterator
         it = h.begin(); it != h.end(); ++it)
    {
      printf("[%s] = [%s]\n", it->first.c_str(), it->second.c_str());
    }
  }

}
