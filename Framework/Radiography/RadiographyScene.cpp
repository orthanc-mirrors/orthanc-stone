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


#include "RadiographyScene.h"

#include "../Toolbox/ImageGeometry.h"
#include "../Toolbox/DicomFrameConverter.h"

#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PamReader.h>
#include <Core/Images/PamWriter.h>
#include <Core/Images/PngWriter.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/DicomDatasetReader.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>


namespace OrthancStone
{
  static double Square(double x)
  {
    return x * x;
  }


  void RadiographyScene::Layer::UpdateTransform()
  {
    transform_ = AffineTransform2D::CreateScaling(pixelSpacingX_, pixelSpacingY_);

    double centerX, centerY;
    GetCenter(centerX, centerY);

    transform_ = AffineTransform2D::Combine(
      AffineTransform2D::CreateOffset(panX_ + centerX, panY_ + centerY),
      AffineTransform2D::CreateRotation(angle_),
      AffineTransform2D::CreateOffset(-centerX, -centerY),
      transform_);

    transformInverse_ = AffineTransform2D::Invert(transform_);
  }


  void RadiographyScene::Layer::AddToExtent(Extent2D& extent,
                                            double x,
                                            double y) const
  {
    transform_.Apply(x, y);
    extent.AddPoint(x, y);
  }


  void RadiographyScene::Layer::GetCornerInternal(double& x,
                                                  double& y,
                                                  Corner corner,
                                                  unsigned int cropX,
                                                  unsigned int cropY,
                                                  unsigned int cropWidth,
                                                  unsigned int cropHeight) const
  {
    double dx = static_cast<double>(cropX);
    double dy = static_cast<double>(cropY);
    double dwidth = static_cast<double>(cropWidth);
    double dheight = static_cast<double>(cropHeight);

    switch (corner)
    {
      case Corner_TopLeft:
        x = dx;
        y = dy;
        break;

      case Corner_TopRight:
        x = dx + dwidth;
        y = dy;
        break;

      case Corner_BottomLeft:
        x = dx;
        y = dy + dheight;
        break;

      case Corner_BottomRight:
        x = dx + dwidth;
        y = dy + dheight;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    transform_.Apply(x, y);
  }


  bool RadiographyScene::Layer::Contains(double x,
                                         double y) const
  {
    transformInverse_.Apply(x, y);
        
    unsigned int cropX, cropY, cropWidth, cropHeight;
    GetCrop(cropX, cropY, cropWidth, cropHeight);

    return (x >= cropX && x <= cropX + cropWidth &&
            y >= cropY && y <= cropY + cropHeight);
  }


  void RadiographyScene::Layer::DrawBorders(CairoContext& context,
                                            double zoom)
  {
    unsigned int cx, cy, width, height;
    GetCrop(cx, cy, width, height);

    double dx = static_cast<double>(cx);
    double dy = static_cast<double>(cy);
    double dwidth = static_cast<double>(width);
    double dheight = static_cast<double>(height);

    cairo_t* cr = context.GetObject();
    cairo_set_line_width(cr, 2.0 / zoom);
        
    double x, y;
    x = dx;
    y = dy;
    transform_.Apply(x, y);
    cairo_move_to(cr, x, y);

    x = dx + dwidth;
    y = dy;
    transform_.Apply(x, y);
    cairo_line_to(cr, x, y);

    x = dx + dwidth;
    y = dy + dheight;
    transform_.Apply(x, y);
    cairo_line_to(cr, x, y);

    x = dx;
    y = dy + dheight;
    transform_.Apply(x, y);
    cairo_line_to(cr, x, y);

    x = dx;
    y = dy;
    transform_.Apply(x, y);
    cairo_line_to(cr, x, y);

    cairo_stroke(cr);
  }


  RadiographyScene::Layer::Layer() :
    index_(0),
    hasSize_(false),
    width_(0),
    height_(0),
    hasCrop_(false),
    pixelSpacingX_(1),
    pixelSpacingY_(1),
    panX_(0),
    panY_(0),
    angle_(0),
    resizeable_(false)
  {
    UpdateTransform();
  }


  void RadiographyScene::Layer::SetCrop(unsigned int x,
                                        unsigned int y,
                                        unsigned int width,
                                        unsigned int height)
  {
    if (!hasSize_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
        
    if (x + width > width_ ||
        y + height > height_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
        
    hasCrop_ = true;
    cropX_ = x;
    cropY_ = y;
    cropWidth_ = width;
    cropHeight_ = height;

    UpdateTransform();
  }

      
  void RadiographyScene::Layer::GetCrop(unsigned int& x,
                                        unsigned int& y,
                                        unsigned int& width,
                                        unsigned int& height) const
  {
    if (hasCrop_)
    {
      x = cropX_;
      y = cropY_;
      width = cropWidth_;
      height = cropHeight_;
    }
    else 
    {
      x = 0;
      y = 0;
      width = width_;
      height = height_;
    }
  }

      
  void RadiographyScene::Layer::SetAngle(double angle)
  {
    angle_ = angle;
    UpdateTransform();
  }


  void RadiographyScene::Layer::SetSize(unsigned int width,
                                        unsigned int height)
  {
    if (hasSize_ &&
        (width != width_ ||
         height != height_))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize);
    }
        
    hasSize_ = true;
    width_ = width;
    height_ = height;

    UpdateTransform();
  }


  Extent2D RadiographyScene::Layer::GetExtent() const
  {
    Extent2D extent;
       
    unsigned int x, y, width, height;
    GetCrop(x, y, width, height);

    double dx = static_cast<double>(x);
    double dy = static_cast<double>(y);
    double dwidth = static_cast<double>(width);
    double dheight = static_cast<double>(height);

    AddToExtent(extent, dx, dy);
    AddToExtent(extent, dx + dwidth, dy);
    AddToExtent(extent, dx, dy + dheight);
    AddToExtent(extent, dx + dwidth, dy + dheight);
        
    return extent;
  }


  bool RadiographyScene::Layer::GetPixel(unsigned int& imageX,
                                         unsigned int& imageY,
                                         double sceneX,
                                         double sceneY) const
  {
    if (width_ == 0 ||
        height_ == 0)
    {
      return false;
    }
    else
    {
      transformInverse_.Apply(sceneX, sceneY);
        
      int x = static_cast<int>(std::floor(sceneX));
      int y = static_cast<int>(std::floor(sceneY));

      if (x < 0)
      {
        imageX = 0;
      }
      else if (x >= static_cast<int>(width_))
      {
        imageX = width_;
      }
      else
      {
        imageX = static_cast<unsigned int>(x);
      }

      if (y < 0)
      {
        imageY = 0;
      }
      else if (y >= static_cast<int>(height_))
      {
        imageY = height_;
      }
      else
      {
        imageY = static_cast<unsigned int>(y);
      }

      return true;
    }
  }


  void RadiographyScene::Layer::SetPan(double x,
                                       double y)
  {
    panX_ = x;
    panY_ = y;
    UpdateTransform();
  }


  void RadiographyScene::Layer::SetPixelSpacing(double x,
                                                double y)
  {
    pixelSpacingX_ = x;
    pixelSpacingY_ = y;
    UpdateTransform();
  }


  void RadiographyScene::Layer::GetCenter(double& centerX,
                                          double& centerY) const
  {
    centerX = static_cast<double>(width_) / 2.0;
    centerY = static_cast<double>(height_) / 2.0;
    transform_.Apply(centerX, centerY);
  }


  void RadiographyScene::Layer::GetCorner(double& x /* out */,
                                          double& y /* out */,
                                          Corner corner) const
  {
    unsigned int cropX, cropY, cropWidth, cropHeight;
    GetCrop(cropX, cropY, cropWidth, cropHeight);
    GetCornerInternal(x, y, corner, cropX, cropY, cropWidth, cropHeight);
  }
      
      
  bool RadiographyScene::Layer::LookupCorner(Corner& corner /* out */,
                                             double x,
                                             double y,
                                             double zoom,
                                             double viewportDistance) const
  {
    static const Corner CORNERS[] = {
      Corner_TopLeft,
      Corner_TopRight,
      Corner_BottomLeft,
      Corner_BottomRight
    };
        
    unsigned int cropX, cropY, cropWidth, cropHeight;
    GetCrop(cropX, cropY, cropWidth, cropHeight);

    double threshold = Square(viewportDistance / zoom);
        
    for (size_t i = 0; i < 4; i++)
    {
      double cx, cy;
      GetCornerInternal(cx, cy, CORNERS[i], cropX, cropY, cropWidth, cropHeight);

      double d = Square(cx - x) + Square(cy - y);
        
      if (d <= threshold)
      {
        corner = CORNERS[i];
        return true;
      }
    }
        
    return false;
  }

      

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

  
  RadiographyScene::Layer& RadiographyScene::LayerAccessor::GetLayer() const
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



  class RadiographyScene::AlphaLayer : public Layer
  {
  private:
    const RadiographyScene&                scene_;
    std::auto_ptr<Orthanc::ImageAccessor>  alpha_;      // Grayscale8
    bool                                   useWindowing_;
    float                                  foreground_;

  public:
    AlphaLayer(const RadiographyScene& scene) :
      scene_(scene),
      useWindowing_(true),
      foreground_(0)
    {
    }


    void SetForegroundValue(float foreground)
    {
      useWindowing_ = false;
      foreground_ = foreground;
    }
      
      
    void SetAlpha(Orthanc::ImageAccessor* image)
    {
      std::auto_ptr<Orthanc::ImageAccessor> raii(image);
        
      if (image == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      if (image->GetFormat() != Orthanc::PixelFormat_Grayscale8)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }

      SetSize(image->GetWidth(), image->GetHeight());
      alpha_ = raii;
    }


    void LoadText(const Orthanc::Font& font,
                  const std::string& utf8)
    {
      SetAlpha(font.RenderAlpha(utf8));
    }                   


    virtual bool GetDefaultWindowing(float& center,
                                     float& width) const
    {
      return false;
    }
      

    virtual void Render(Orthanc::ImageAccessor& buffer,
                        const AffineTransform2D& viewTransform,
                        ImageInterpolation interpolation) const
    {
      if (alpha_.get() == NULL)
      {
        return;
      }
        
      if (buffer.GetFormat() != Orthanc::PixelFormat_Float32)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }

      unsigned int cropX, cropY, cropWidth, cropHeight;
      GetCrop(cropX, cropY, cropWidth, cropHeight);

      const AffineTransform2D t = AffineTransform2D::Combine(
        viewTransform, GetTransform(),
        AffineTransform2D::CreateOffset(cropX, cropY));

      Orthanc::ImageAccessor cropped;
      alpha_->GetRegion(cropped, cropX, cropY, cropWidth, cropHeight);
        
      Orthanc::Image tmp(Orthanc::PixelFormat_Grayscale8, buffer.GetWidth(), buffer.GetHeight(), false);
      
      t.Apply(tmp, cropped, interpolation, true /* clear */);

      // Blit
      const unsigned int width = buffer.GetWidth();
      const unsigned int height = buffer.GetHeight();

      float value = foreground_;
        
      if (useWindowing_)
      {
        float center, width;
        if (scene_.GetWindowing(center, width))
        {
          value = center + width / 2.0f;
        }
      }
        
      for (unsigned int y = 0; y < height; y++)
      {
        float *q = reinterpret_cast<float*>(buffer.GetRow(y));
        const uint8_t *p = reinterpret_cast<uint8_t*>(tmp.GetRow(y));

        for (unsigned int x = 0; x < width; x++, p++, q++)
        {
          float a = static_cast<float>(*p) / 255.0f;
            
          *q = (a * value + (1.0f - a) * (*q));
        }
      }        
    }

      
    virtual bool GetRange(float& minValue,
                          float& maxValue) const
    {
      if (useWindowing_)
      {
        return false;
      }
      else
      {
        minValue = 0;
        maxValue = 0;

        if (foreground_ < 0)
        {
          minValue = foreground_;
        }

        if (foreground_ > 0)
        {
          maxValue = foreground_;
        }

        return true;
      }
    }
  };
    
    

  class RadiographyScene::DicomLayer : public Layer
  {
  private:
    std::auto_ptr<Orthanc::ImageAccessor>  source_;  // Content of PixelData
    std::auto_ptr<DicomFrameConverter>     converter_;
    std::auto_ptr<Orthanc::ImageAccessor>  converted_;  // Float32

    static OrthancPlugins::DicomTag  ConvertTag(const Orthanc::DicomTag& tag)
    {
      return OrthancPlugins::DicomTag(tag.GetGroup(), tag.GetElement());
    }
      

    void ApplyConverter()
    {
      if (source_.get() != NULL &&
          converter_.get() != NULL)
      {
        converted_.reset(converter_->ConvertFrame(*source_));
      }
    }
      
  public:
    void SetDicomTags(const OrthancPlugins::FullOrthancDataset& dataset)
    {
      converter_.reset(new DicomFrameConverter);
      converter_->ReadParameters(dataset);
      ApplyConverter();

      std::string tmp;
      Vector pixelSpacing;
        
      if (dataset.GetStringValue(tmp, ConvertTag(Orthanc::DICOM_TAG_PIXEL_SPACING)) &&
          LinearAlgebra::ParseVector(pixelSpacing, tmp) &&
          pixelSpacing.size() == 2)
      {
        SetPixelSpacing(pixelSpacing[0], pixelSpacing[1]);
      }

      //SetPan(-0.5 * GetPixelSpacingX(), -0.5 * GetPixelSpacingY());
      
      OrthancPlugins::DicomDatasetReader reader(dataset);

      unsigned int width, height;
      if (!reader.GetUnsignedIntegerValue(width, ConvertTag(Orthanc::DICOM_TAG_COLUMNS)) ||
          !reader.GetUnsignedIntegerValue(height, ConvertTag(Orthanc::DICOM_TAG_ROWS)))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
      else
      {
        SetSize(width, height);
      }
    }

      
    void SetSourceImage(Orthanc::ImageAccessor* image)   // Takes ownership
    {
      std::auto_ptr<Orthanc::ImageAccessor> raii(image);
        
      if (image == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      SetSize(image->GetWidth(), image->GetHeight());
        
      source_ = raii;
      ApplyConverter();
    }

      
    virtual void Render(Orthanc::ImageAccessor& buffer,
                        const AffineTransform2D& viewTransform,
                        ImageInterpolation interpolation) const
    {
      if (converted_.get() != NULL)
      {
        if (converted_->GetFormat() != Orthanc::PixelFormat_Float32)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        unsigned int cropX, cropY, cropWidth, cropHeight;
        GetCrop(cropX, cropY, cropWidth, cropHeight);

        AffineTransform2D t = AffineTransform2D::Combine(
          viewTransform, GetTransform(),
          AffineTransform2D::CreateOffset(cropX, cropY));

        Orthanc::ImageAccessor cropped;
        converted_->GetRegion(cropped, cropX, cropY, cropWidth, cropHeight);
        
        t.Apply(buffer, cropped, interpolation, false);
      }
    }


    virtual bool GetDefaultWindowing(float& center,
                                     float& width) const
    {
      if (converter_.get() != NULL &&
          converter_->HasDefaultWindow())
      {
        center = static_cast<float>(converter_->GetDefaultWindowCenter());
        width = static_cast<float>(converter_->GetDefaultWindowWidth());
        return true;
      }
      else
      {
        return false;
      }
    }


    virtual bool GetRange(float& minValue,
                          float& maxValue) const
    {
      if (converted_.get() != NULL)
      {
        if (converted_->GetFormat() != Orthanc::PixelFormat_Float32)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        Orthanc::ImageProcessing::GetMinMaxFloatValue(minValue, maxValue, *converted_);
        return true;
      }
      else
      {
        return false;
      }
    }
  };


  RadiographyScene::Layer& RadiographyScene::RegisterLayer(RadiographyScene::Layer* layer)
  {
    if (layer == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    std::auto_ptr<Layer> raii(layer);
      
    size_t index = countLayers_++;
    raii->SetIndex(index);
    layers_[index] = raii.release();

    EmitMessage(GeometryChangedMessage(*this));
    EmitMessage(ContentChangedMessage(*this));

    return *layer;
  }
    

  RadiographyScene::RadiographyScene(MessageBroker& broker,
                                     OrthancApiClient& orthanc) :
    IObserver(broker),
    IObservable(broker),
    orthanc_(orthanc),
    countLayers_(0),
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
  }


  RadiographyScene::Layer& RadiographyScene::LoadText(const Orthanc::Font& font,
                                                      const std::string& utf8)
  {
    std::auto_ptr<AlphaLayer>  alpha(new AlphaLayer(*this));
    alpha->LoadText(font, utf8);

    return RegisterLayer(alpha.release());
  }

    
  RadiographyScene::Layer& RadiographyScene::LoadTestBlock(unsigned int width,
                                                           unsigned int height)
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

    std::auto_ptr<AlphaLayer>  alpha(new AlphaLayer(*this));
    alpha->SetAlpha(block.release());

    return RegisterLayer(alpha.release());
  }

    
  RadiographyScene::Layer& RadiographyScene::LoadDicomFrame(const std::string& instance,
                                                            unsigned int frame,
                                                            bool httpCompression)
  {
    Layer& layer = RegisterLayer(new DicomLayer);

    {
      IWebService::Headers headers;
      std::string uri = "/instances/" + instance + "/tags";
        
      orthanc_.GetBinaryAsync(
        uri, headers,
        new Callable<RadiographyScene, OrthancApiClient::BinaryResponseReadyMessage>
        (*this, &RadiographyScene::OnTagsReceived), NULL,
        new Orthanc::SingleValueObject<size_t>(layer.GetIndex()));
    }

    {
      IWebService::Headers headers;
      headers["Accept"] = "image/x-portable-arbitrarymap";

      if (httpCompression)
      {
        headers["Accept-Encoding"] = "gzip";
      }
        
      std::string uri = ("/instances/" + instance + "/frames/" +
                         boost::lexical_cast<std::string>(frame) + "/image-uint16");
        
      orthanc_.GetBinaryAsync(
        uri, headers,
        new Callable<RadiographyScene, OrthancApiClient::BinaryResponseReadyMessage>
        (*this, &RadiographyScene::OnFrameReceived), NULL,
        new Orthanc::SingleValueObject<size_t>(layer.GetIndex()));
    }

    return layer;
  }

    
  void RadiographyScene::OnTagsReceived(const OrthancApiClient::BinaryResponseReadyMessage& message)
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
      dynamic_cast<DicomLayer*>(layer->second)->SetDicomTags(dicom);

      float c, w;
      if (!hasWindowing_ &&
          layer->second->GetDefaultWindowing(c, w))
      {
        hasWindowing_ = true;
        windowingCenter_ = c;
        windowingWidth_ = w;
      }

      EmitMessage(GeometryChangedMessage(*this));
    }
  }
    

  void RadiographyScene::OnFrameReceived(const OrthancApiClient::BinaryResponseReadyMessage& message)
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
      dynamic_cast<DicomLayer*>(layer->second)->SetSourceImage(reader.release());

      EmitMessage(ContentChangedMessage(*this));
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
                                ImageInterpolation interpolation) const
  {
    Orthanc::ImageProcessing::Set(buffer, 0);

    // Render layers in the background-to-foreground order
    for (size_t index = 0; index < countLayers_; index++)
    {
      Layers::const_iterator it = layers_.find(index);
      if (it != layers_.end())
      {
        assert(it->second != NULL);
        it->second->Render(buffer, viewTransform, interpolation);
      }
    }
  }


  bool RadiographyScene::LookupLayer(size_t& index /* out */,
                                     double x,
                                     double y) const
  {
    // Render layers in the foreground-to-background order
    for (size_t i = countLayers_; i > 0; i--)
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


  // Export using PAM is faster than using PNG, but requires Orthanc
  // core >= 1.4.3
  void RadiographyScene::ExportDicom(const Orthanc::DicomMap& dicom,
                                     double pixelSpacingX,
                                     double pixelSpacingY,
                                     bool invert,
                                     ImageInterpolation interpolation,
                                     bool usePam)
  {
    if (pixelSpacingX <= 0 ||
        pixelSpacingY <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
      
    LOG(INFO) << "Exporting DICOM";

    Extent2D extent = GetSceneExtent();

    int w = std::ceil(extent.GetWidth() / pixelSpacingX);
    int h = std::ceil(extent.GetHeight() / pixelSpacingY);

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
      
    Render(layers, view, interpolation);

    Orthanc::Image rendered(Orthanc::PixelFormat_Grayscale16,
                            layers.GetWidth(), layers.GetHeight(), false);
    Orthanc::ImageProcessing::Convert(rendered, layers);

    std::string base64;

    {
      std::string content;

      if (usePam)
      {
        Orthanc::PamWriter writer;
        writer.WriteToMemory(content, rendered);
      }
      else
      {
        Orthanc::PngWriter writer;
        writer.WriteToMemory(content, rendered);
      }

      Orthanc::Toolbox::EncodeBase64(base64, content);
    }

    std::set<Orthanc::DicomTag> tags;
    dicom.GetTags(tags);

    Json::Value json = Json::objectValue;
    json["Tags"] = Json::objectValue;
           
    for (std::set<Orthanc::DicomTag>::const_iterator
           tag = tags.begin(); tag != tags.end(); ++tag)
    {
      const Orthanc::DicomValue& value = dicom.GetValue(*tag);
      if (!value.IsNull() &&
          !value.IsBinary())
      {
        json["Tags"][tag->Format()] = value.GetContent();
      }
    }

    json["Tags"][Orthanc::DICOM_TAG_PHOTOMETRIC_INTERPRETATION.Format()] =
      (invert ? "MONOCHROME1" : "MONOCHROME2");

    // WARNING: The order of PixelSpacing is Y/X. We use "%0.8f" to
    // avoid floating-point numbers to grow over 16 characters,
    // which would be invalid according to DICOM standard
    // ("dciodvfy" would complain).
    char buf[32];
    sprintf(buf, "%0.8f\\%0.8f", pixelSpacingY, pixelSpacingX);
      
    json["Tags"][Orthanc::DICOM_TAG_PIXEL_SPACING.Format()] = buf;

    float center, width;
    if (GetWindowing(center, width))
    {
      json["Tags"][Orthanc::DICOM_TAG_WINDOW_CENTER.Format()] =
        boost::lexical_cast<std::string>(boost::math::iround(center));

      json["Tags"][Orthanc::DICOM_TAG_WINDOW_WIDTH.Format()] =
        boost::lexical_cast<std::string>(boost::math::iround(width));
    }

    // This is Data URI scheme: https://en.wikipedia.org/wiki/Data_URI_scheme
    json["Content"] = ("data:" +
                       std::string(usePam ? Orthanc::MIME_PAM : Orthanc::MIME_PNG) +
                       ";base64," + base64);

    orthanc_.PostJsonAsyncExpectJson(
      "/tools/create-dicom", json,
      new Callable<RadiographyScene, OrthancApiClient::JsonResponseReadyMessage>
      (*this, &RadiographyScene::OnDicomExported),
      NULL, NULL);
  }


  void RadiographyScene::OnDicomExported(const OrthancApiClient::JsonResponseReadyMessage& message)
  {
    LOG(INFO) << "DICOM export was successful:"
              << message.GetJson().toStyledString();
  }
}
