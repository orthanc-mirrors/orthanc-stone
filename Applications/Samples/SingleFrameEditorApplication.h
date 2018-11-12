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

#include "SampleApplicationBase.h"

#include "../../Framework/Toolbox/ImageGeometry.h"
#include "../../Framework/Toolbox/OrthancApiClient.h"
#include "../../Framework/Toolbox/DicomFrameConverter.h"

#include <Core/Images/FontRegistry.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PamReader.h>
#include <Core/Images/PamWriter.h>
#include <Core/Images/PngWriter.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/DicomDatasetReader.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>


// Export using PAM is faster than using PNG, but requires Orthanc
// core >= 1.4.3
#define EXPORT_USING_PAM  1


#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/round.hpp>


namespace OrthancStone
{
  class RadiologyScene :
    public IObserver,
    public IObservable
  {
  public:
    typedef OriginMessage<MessageType_Widget_GeometryChanged, RadiologyScene> GeometryChangedMessage;
    typedef OriginMessage<MessageType_Widget_ContentChanged, RadiologyScene> ContentChangedMessage;


    enum Corner
    {
      Corner_TopLeft,
      Corner_TopRight,
      Corner_BottomLeft,
      Corner_BottomRight
    };



    class Layer : public boost::noncopyable
    {
      friend class RadiologyScene;
      
    private:
      size_t        index_;
      bool          hasSize_;
      unsigned int  width_;
      unsigned int  height_;
      bool          hasCrop_;
      unsigned int  cropX_;
      unsigned int  cropY_;
      unsigned int  cropWidth_;
      unsigned int  cropHeight_;
      Matrix        transform_;
      Matrix        transformInverse_;
      double        pixelSpacingX_;
      double        pixelSpacingY_;
      double        panX_;
      double        panY_;
      double        angle_;
      bool          resizeable_;


    protected:
      const Matrix& GetTransform() const
      {
        return transform_;
      }


    private:
      static void ApplyTransform(double& x /* inout */,
                                 double& y /* inout */,
                                 const Matrix& transform)
      {
        Vector p;
        LinearAlgebra::AssignVector(p, x, y, 1);

        Vector q = LinearAlgebra::Product(transform, p);

        if (!LinearAlgebra::IsNear(q[2], 1.0))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
        else
        {
          x = q[0];
          y = q[1];
        }
      }
      
      
      void UpdateTransform()
      {
        transform_ = CreateScalingMatrix(pixelSpacingX_, pixelSpacingY_);

        double centerX, centerY;
        GetCenter(centerX, centerY);

        transform_ = LinearAlgebra::Product(
          CreateOffsetMatrix(panX_ + centerX, panY_ + centerY),
          CreateRotationMatrix(angle_),
          CreateOffsetMatrix(-centerX, -centerY),
          transform_);

        LinearAlgebra::InvertMatrix(transformInverse_, transform_);
      }


      void AddToExtent(Extent2D& extent,
                       double x,
                       double y) const
      {
        ApplyTransform(x, y, transform_);
        extent.AddPoint(x, y);
      }


      void GetCornerInternal(double& x,
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

        ApplyTransform(x, y, transform_);
      }


      void SetIndex(size_t index)
      {
        index_ = index;
      }

      
      bool Contains(double x,
                    double y) const
      {
        ApplyTransform(x, y, transformInverse_);
        
        unsigned int cropX, cropY, cropWidth, cropHeight;
        GetCrop(cropX, cropY, cropWidth, cropHeight);

        return (x >= cropX && x <= cropX + cropWidth &&
                y >= cropY && y <= cropY + cropHeight);
      }


      void DrawBorders(CairoContext& context,
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
        ApplyTransform(x, y, transform_);
        cairo_move_to(cr, x, y);

        x = dx + dwidth;
        y = dy;
        ApplyTransform(x, y, transform_);
        cairo_line_to(cr, x, y);

        x = dx + dwidth;
        y = dy + dheight;
        ApplyTransform(x, y, transform_);
        cairo_line_to(cr, x, y);

        x = dx;
        y = dy + dheight;
        ApplyTransform(x, y, transform_);
        cairo_line_to(cr, x, y);

        x = dx;
        y = dy;
        ApplyTransform(x, y, transform_);
        cairo_line_to(cr, x, y);

        cairo_stroke(cr);
      }


      static double Square(double x)
      {
        return x * x;
      }


    public:
      Layer() :
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

      virtual ~Layer()
      {
      }

      size_t GetIndex() const
      {
        return index_;
      }

      void ResetCrop()
      {
        hasCrop_ = false;
      }

      void SetCrop(unsigned int x,
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

      void GetCrop(unsigned int& x,
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

      void SetAngle(double angle)
      {
        angle_ = angle;
        UpdateTransform();
      }

      double GetAngle() const
      {
        return angle_;
      }

      void SetSize(unsigned int width,
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


      unsigned int GetWidth() const
      {
        return width_;
      }
        

      unsigned int GetHeight() const
      {
        return height_;
      }       


      Extent2D GetExtent() const
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


      bool GetPixel(unsigned int& imageX,
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
          ApplyTransform(sceneX, sceneY, transformInverse_);
        
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


      void SetPan(double x,
                  double y)
      {
        panX_ = x;
        panY_ = y;
        UpdateTransform();
      }


      void SetPixelSpacing(double x,
                           double y)
      {
        pixelSpacingX_ = x;
        pixelSpacingY_ = y;
        UpdateTransform();
      }

      double GetPixelSpacingX() const
      {
        return pixelSpacingX_;
      }   

      double GetPixelSpacingY() const
      {
        return pixelSpacingY_;
      }   

      double GetPanX() const
      {
        return panX_;
      }

      double GetPanY() const
      {
        return panY_;
      }

      void GetCenter(double& centerX,
                     double& centerY) const
      {
        centerX = static_cast<double>(width_) / 2.0;
        centerY = static_cast<double>(height_) / 2.0;
        ApplyTransform(centerX, centerY, transform_);
      }


      void GetCorner(double& x /* out */,
                     double& y /* out */,
                     Corner corner) const
      {
        unsigned int cropX, cropY, cropWidth, cropHeight;
        GetCrop(cropX, cropY, cropWidth, cropHeight);
        GetCornerInternal(x, y, corner, cropX, cropY, cropWidth, cropHeight);
      }
      
      
      bool LookupCorner(Corner& corner /* out */,
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

      bool IsResizeable() const
      {
        return resizeable_;
      }

      void SetResizeable(bool resizeable)
      {
        resizeable_ = resizeable;
      }

      virtual bool GetDefaultWindowing(float& center,
                                       float& width) const = 0;

      virtual void Render(Orthanc::ImageAccessor& buffer,
                          const Matrix& viewTransform,
                          ImageInterpolation interpolation) const = 0;

      virtual bool GetRange(float& minValue,
                            float& maxValue) const = 0;
    }; 


    class LayerAccessor : public boost::noncopyable
    {
    private:
      RadiologyScene&  scene_;
      size_t           index_;
      Layer*           layer_;

    public:
      LayerAccessor(RadiologyScene& scene,
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

      LayerAccessor(RadiologyScene& scene,
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

      void Invalidate()
      {
        layer_ = NULL;
      }

      bool IsValid() const
      {
        return layer_ != NULL;
      }

      RadiologyScene& GetScene() const
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

      size_t GetIndex() const
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

      Layer& GetLayer() const
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
    };    


  private:
    class AlphaLayer : public Layer
    {
    private:
      const RadiologyScene&                  scene_;
      std::auto_ptr<Orthanc::ImageAccessor>  alpha_;      // Grayscale8
      bool                                   useWindowing_;
      float                                  foreground_;

    public:
      AlphaLayer(const RadiologyScene& scene) :
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
                          const Matrix& viewTransform,
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

        Matrix m = LinearAlgebra::Product(viewTransform,
                                          GetTransform(),
                                          CreateOffsetMatrix(cropX, cropY));

        Orthanc::ImageAccessor cropped;
        alpha_->GetRegion(cropped, cropX, cropY, cropWidth, cropHeight);
        
        Orthanc::Image tmp(Orthanc::PixelFormat_Grayscale8, buffer.GetWidth(), buffer.GetHeight(), false);
        ApplyProjectiveTransform(tmp, cropped, m, interpolation, true /* clear */);

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
    
    

  private:
    static Matrix CreateOffsetMatrix(double dx,
                                     double dy)
    {
      Matrix m = LinearAlgebra::IdentityMatrix(3);
      m(0, 2) = dx;
      m(1, 2) = dy;
      return m;
    }
      

    static Matrix CreateScalingMatrix(double sx,
                                      double sy)
    {
      Matrix m = LinearAlgebra::IdentityMatrix(3);
      m(0, 0) = sx;
      m(1, 1) = sy;
      return m;
    }
      

    static Matrix CreateRotationMatrix(double angle)
    {
      Matrix m;
      const double v[] = { cos(angle), -sin(angle), 0,
                           sin(angle), cos(angle), 0,
                           0, 0, 1 };
      LinearAlgebra::FillMatrix(m, 3, 3, v);
      return m;
    }
      

    class DicomLayer : public Layer
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
                          const Matrix& viewTransform,
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

          Matrix m = LinearAlgebra::Product(viewTransform,
                                            GetTransform(),
                                            CreateOffsetMatrix(cropX, cropY));

          Orthanc::ImageAccessor cropped;
          converted_->GetRegion(cropped, cropX, cropY, cropWidth, cropHeight);
        
          ApplyProjectiveTransform(buffer, cropped, m, interpolation, false);
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




    typedef std::map<size_t, Layer*>  Layers;
        
    OrthancApiClient&  orthanc_;
    size_t             countLayers_;
    bool               hasWindowing_;
    float              windowingCenter_;
    float              windowingWidth_;
    Layers             layers_;


    Layer& RegisterLayer(Layer* layer)
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
    

  public:
    RadiologyScene(MessageBroker& broker,
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


    virtual ~RadiologyScene()
    {
      for (Layers::iterator it = layers_.begin(); it != layers_.end(); it++)
      {
        assert(it->second != NULL);
        delete it->second;
      }
    }


    bool GetWindowing(float& center,
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


    void GetWindowingWithDefault(float& center,
                                 float& width) const
    {
      if (!GetWindowing(center, width))
      {
        center = 128;
        width = 256;
      }
    }


    void SetWindowing(float center,
                      float width)

    {
      hasWindowing_ = true;
      windowingCenter_ = center;
      windowingWidth_ = width;
    }


    Layer& LoadText(const Orthanc::Font& font,
                    const std::string& utf8)
    {
      std::auto_ptr<AlphaLayer>  alpha(new AlphaLayer(*this));
      alpha->LoadText(font, utf8);

      return RegisterLayer(alpha.release());
    }

    
    Layer& LoadTestBlock(unsigned int width,
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

    
    Layer& LoadDicomFrame(const std::string& instance,
                          unsigned int frame,
                          bool httpCompression)
    {
      Layer& layer = RegisterLayer(new DicomLayer);

      {
        IWebService::Headers headers;
        std::string uri = "/instances/" + instance + "/tags";
        orthanc_.GetBinaryAsync(uri, headers,
                                new Callable<RadiologyScene, OrthancApiClient::BinaryResponseReadyMessage>
                                (*this, &RadiologyScene::OnTagsReceived), NULL,
                                new Orthanc::SingleValueObject<size_t>(layer.GetIndex()));
      }

      {
        IWebService::Headers headers;
        headers["Accept"] = "image/x-portable-arbitrarymap";

        if (httpCompression)
        {
          headers["Accept-Encoding"] = "gzip";
        }
        
        std::string uri = "/instances/" + instance + "/frames/" + boost::lexical_cast<std::string>(frame) + "/image-uint16";
        orthanc_.GetBinaryAsync(uri, headers,
                                new Callable<RadiologyScene, OrthancApiClient::BinaryResponseReadyMessage>
                                (*this, &RadiologyScene::OnFrameReceived), NULL,
                                new Orthanc::SingleValueObject<size_t>(layer.GetIndex()));
      }

      return layer;
    }

    
    void OnTagsReceived(const OrthancApiClient::BinaryResponseReadyMessage& message)
    {
      size_t index = dynamic_cast<const Orthanc::SingleValueObject<size_t>&>(message.GetPayload()).GetValue();

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
    

    void OnFrameReceived(const OrthancApiClient::BinaryResponseReadyMessage& message)
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


    Extent2D GetSceneExtent() const
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
    

    void Render(Orthanc::ImageAccessor& buffer,
                const Matrix& viewTransform,
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


    bool LookupLayer(size_t& index /* out */,
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

    
    void DrawBorder(CairoContext& context,
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


    void GetRange(float& minValue,
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
    void Export(const Orthanc::DicomMap& dicom,
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

      Matrix view = LinearAlgebra::Product(
        CreateScalingMatrix(1.0 / pixelSpacingX, 1.0 / pixelSpacingY),
        CreateOffsetMatrix(-extent.GetX1(), -extent.GetY1()));
      
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
        new Callable<RadiologyScene, OrthancApiClient::JsonResponseReadyMessage>
        (*this, &RadiologyScene::OnDicomExported),
        NULL, NULL);
    }


    void OnDicomExported(const OrthancApiClient::JsonResponseReadyMessage& message)
    {
      LOG(INFO) << "DICOM export was successful:"
                << message.GetJson().toStyledString();
    }
  };


  class UndoRedoStack : public boost::noncopyable
  {
  public:
    class ICommand : public boost::noncopyable
    {
    public:
      virtual ~ICommand()
      {
      }
      
      virtual void Undo() const = 0;
      
      virtual void Redo() const = 0;
    };

  private:
    typedef std::list<ICommand*>  Stack;

    Stack            stack_;
    Stack::iterator  current_;

    void Clear(Stack::iterator from)
    {
      for (Stack::iterator it = from; it != stack_.end(); ++it)
      {
        assert(*it != NULL);
        delete *it;
      }

      stack_.erase(from, stack_.end());
    }

  public:
    UndoRedoStack() :
      current_(stack_.end())
    {
    }
    
    ~UndoRedoStack()
    {
      Clear(stack_.begin());
    }

    void Add(ICommand* command)
    {
      if (command == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
      
      Clear(current_);

      stack_.push_back(command);
      current_ = stack_.end();
    }

    void Undo()
    {
      if (current_ != stack_.begin())
      {
        --current_;
        
        assert(*current_ != NULL);
        (*current_)->Undo();
      }
    }

    void Redo()
    {
      if (current_ != stack_.end())
      {
        assert(*current_ != NULL);
        (*current_)->Redo();

        ++current_;
      }
    }
  };


  class RadiologyLayerCommand : public UndoRedoStack::ICommand
  {
  private:
    RadiologyScene&  scene_;
    size_t           layer_;

  protected:
    virtual void UndoInternal(RadiologyScene::Layer& layer) const = 0;

    virtual void RedoInternal(RadiologyScene::Layer& layer) const = 0;

  public:
    RadiologyLayerCommand(RadiologyScene& scene,
                          size_t layer) :
      scene_(scene),
      layer_(layer)
    {
    }

    RadiologyLayerCommand(const RadiologyScene::LayerAccessor& accessor) :
      scene_(accessor.GetScene()),
      layer_(accessor.GetIndex())
    {
    }

    virtual void Undo() const
    {
      RadiologyScene::LayerAccessor accessor(scene_, layer_);

      if (accessor.IsValid())
      {
        UndoInternal(accessor.GetLayer());
      }
    }

    virtual void Redo() const
    {
      RadiologyScene::LayerAccessor accessor(scene_, layer_);

      if (accessor.IsValid())
      {
        RedoInternal(accessor.GetLayer());
      }
    }
  };


  class RadiologyLayerRotateTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&                 undoRedoStack_;
    RadiologyScene::LayerAccessor  accessor_;
    double                         centerX_;
    double                         centerY_;
    double                         originalAngle_;
    double                         clickAngle_;
    bool                           roundAngles_;

    bool ComputeAngle(double& angle /* out */,
                      double sceneX,
                      double sceneY) const
    {
      Vector u;
      LinearAlgebra::AssignVector(u, sceneX - centerX_, sceneY - centerY_);

      double nu = boost::numeric::ublas::norm_2(u);

      if (!LinearAlgebra::IsCloseToZero(nu))
      {
        u /= nu;
        angle = atan2(u[1], u[0]);
        return true;
      }
      else
      {
        return false;
      }
    }


    class UndoRedoCommand : public RadiologyLayerCommand
    {
    private:
      double  sourceAngle_;
      double  targetAngle_;

      static int ToDegrees(double angle)
      {
        return boost::math::iround(angle * 180.0 / boost::math::constants::pi<double>());
      }
      
    protected:
      virtual void UndoInternal(RadiologyScene::Layer& layer) const
      {
        LOG(INFO) << "Undo - Set angle to " << ToDegrees(sourceAngle_) << " degrees";
        layer.SetAngle(sourceAngle_);
      }

      virtual void RedoInternal(RadiologyScene::Layer& layer) const
      {
        LOG(INFO) << "Redo - Set angle to " << ToDegrees(sourceAngle_) << " degrees";
        layer.SetAngle(targetAngle_);
      }

    public:
      UndoRedoCommand(const RadiologyLayerRotateTracker& tracker) :
        RadiologyLayerCommand(tracker.accessor_),
        sourceAngle_(tracker.originalAngle_),
        targetAngle_(tracker.accessor_.GetLayer().GetAngle())
      {
      }
    };

      
  public:
    RadiologyLayerRotateTracker(UndoRedoStack& undoRedoStack,
                                RadiologyScene& scene,
                                const ViewportGeometry& view,
                                size_t layer,
                                double x,
                                double y,
                                bool roundAngles) :
      undoRedoStack_(undoRedoStack),
      accessor_(scene, layer),
      roundAngles_(roundAngles)
    {
      if (accessor_.IsValid())
      {
        accessor_.GetLayer().GetCenter(centerX_, centerY_);
        originalAngle_ = accessor_.GetLayer().GetAngle();

        double sceneX, sceneY;
        view.MapDisplayToScene(sceneX, sceneY, x, y);

        if (!ComputeAngle(clickAngle_, x, y))
        {
          accessor_.Invalidate();
        }
      }
    }

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    virtual void MouseUp()
    {
      if (accessor_.IsValid())
      {
        undoRedoStack_.Add(new UndoRedoCommand(*this));
      }
    }

    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY)
    {
      static const double ROUND_ANGLE = 15.0 / 180.0 * boost::math::constants::pi<double>(); 
        
      double angle;
        
      if (accessor_.IsValid() &&
          ComputeAngle(angle, sceneX, sceneY))
      {
        angle = angle - clickAngle_ + originalAngle_;

        if (roundAngles_)
        {
          angle = boost::math::round<double>((angle / ROUND_ANGLE) * ROUND_ANGLE);
        }
          
        accessor_.GetLayer().SetAngle(angle);
      }
    }
  };
    
    
  class RadiologyLayerMoveTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&                 undoRedoStack_;
    RadiologyScene::LayerAccessor  accessor_;
    double                         clickX_;
    double                         clickY_;
    double                         panX_;
    double                         panY_;
    bool                           oneAxis_;

    class UndoRedoCommand : public RadiologyLayerCommand
    {
    private:
      double  sourceX_;
      double  sourceY_;
      double  targetX_;
      double  targetY_;

    protected:
      virtual void UndoInternal(RadiologyScene::Layer& layer) const
      {
        layer.SetPan(sourceX_, sourceY_);
      }

      virtual void RedoInternal(RadiologyScene::Layer& layer) const
      {
        layer.SetPan(targetX_, targetY_);
      }

    public:
      UndoRedoCommand(const RadiologyLayerMoveTracker& tracker) :
        RadiologyLayerCommand(tracker.accessor_),
        sourceX_(tracker.panX_),
        sourceY_(tracker.panY_),
        targetX_(tracker.accessor_.GetLayer().GetPanX()),
        targetY_(tracker.accessor_.GetLayer().GetPanY())
      {
      }
    };


  public:
    RadiologyLayerMoveTracker(UndoRedoStack& undoRedoStack,
                              RadiologyScene& scene,
                              size_t layer,
                              double x,
                              double y,
                              bool oneAxis) :
      undoRedoStack_(undoRedoStack),
      accessor_(scene, layer),
      clickX_(x),
      clickY_(y),
      oneAxis_(oneAxis)
    {
      if (accessor_.IsValid())
      {
        panX_ = accessor_.GetLayer().GetPanX();
        panY_ = accessor_.GetLayer().GetPanY();
      }
    }

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    virtual void MouseUp()
    {
      if (accessor_.IsValid())
      {
        undoRedoStack_.Add(new UndoRedoCommand(*this));
      }
    }

    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY)
    {
      if (accessor_.IsValid())
      {
        double dx = sceneX - clickX_;
        double dy = sceneY - clickY_;

        if (oneAxis_)
        {
          if (fabs(dx) > fabs(dy))
          {
            accessor_.GetLayer().SetPan(dx + panX_, panY_);
          }
          else
          {
            accessor_.GetLayer().SetPan(panX_, dy + panY_);
          }
        }
        else
        {
          accessor_.GetLayer().SetPan(dx + panX_, dy + panY_);
        }
      }
    }
  };


  class RadiologyLayerCropTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&                 undoRedoStack_;
    RadiologyScene::LayerAccessor  accessor_;
    RadiologyScene::Corner         corner_;
    unsigned int                   cropX_;
    unsigned int                   cropY_;
    unsigned int                   cropWidth_;
    unsigned int                   cropHeight_;

    class UndoRedoCommand : public RadiologyLayerCommand
    {
    private:
      unsigned int  sourceCropX_;
      unsigned int  sourceCropY_;
      unsigned int  sourceCropWidth_;
      unsigned int  sourceCropHeight_;
      unsigned int  targetCropX_;
      unsigned int  targetCropY_;
      unsigned int  targetCropWidth_;
      unsigned int  targetCropHeight_;

    protected:
      virtual void UndoInternal(RadiologyScene::Layer& layer) const
      {
        layer.SetCrop(sourceCropX_, sourceCropY_, sourceCropWidth_, sourceCropHeight_);
      }

      virtual void RedoInternal(RadiologyScene::Layer& layer) const
      {
        layer.SetCrop(targetCropX_, targetCropY_, targetCropWidth_, targetCropHeight_);
      }

    public:
      UndoRedoCommand(const RadiologyLayerCropTracker& tracker) :
        RadiologyLayerCommand(tracker.accessor_),
        sourceCropX_(tracker.cropX_),
        sourceCropY_(tracker.cropY_),
        sourceCropWidth_(tracker.cropWidth_),
        sourceCropHeight_(tracker.cropHeight_)
      {
        tracker.accessor_.GetLayer().GetCrop(targetCropX_, targetCropY_,
                                             targetCropWidth_, targetCropHeight_);
      }
    };


  public:
    RadiologyLayerCropTracker(UndoRedoStack& undoRedoStack,
                              RadiologyScene& scene,
                              const ViewportGeometry& view,
                              size_t layer,
                              double x,
                              double y,
                              RadiologyScene::Corner corner) :
      undoRedoStack_(undoRedoStack),
      accessor_(scene, layer),
      corner_(corner)
    {
      if (accessor_.IsValid())
      {
        accessor_.GetLayer().GetCrop(cropX_, cropY_, cropWidth_, cropHeight_);          
      }
    }

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    virtual void MouseUp()
    {
      if (accessor_.IsValid())
      {
        undoRedoStack_.Add(new UndoRedoCommand(*this));
      }
    }

    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY)
    {
      if (accessor_.IsValid())
      {
        unsigned int x, y;
        
        RadiologyScene::Layer& layer = accessor_.GetLayer();
        if (layer.GetPixel(x, y, sceneX, sceneY))
        {
          unsigned int targetX, targetWidth;

          if (corner_ == RadiologyScene::Corner_TopLeft ||
              corner_ == RadiologyScene::Corner_BottomLeft)
          {
            targetX = std::min(x, cropX_ + cropWidth_);
            targetWidth = cropX_ + cropWidth_ - targetX;
          }
          else
          {
            targetX = cropX_;
            targetWidth = std::max(x, cropX_) - cropX_;
          }

          unsigned int targetY, targetHeight;

          if (corner_ == RadiologyScene::Corner_TopLeft ||
              corner_ == RadiologyScene::Corner_TopRight)
          {
            targetY = std::min(y, cropY_ + cropHeight_);
            targetHeight = cropY_ + cropHeight_ - targetY;
          }
          else
          {
            targetY = cropY_;
            targetHeight = std::max(y, cropY_) - cropY_;
          }

          layer.SetCrop(targetX, targetY, targetWidth, targetHeight);
        }
      }
    }
  };
    
    
  class RadiologyLayerResizeTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&                 undoRedoStack_;
    RadiologyScene::LayerAccessor  accessor_;
    bool                           roundScaling_;
    double                         originalSpacingX_;
    double                         originalSpacingY_;
    double                         originalPanX_;
    double                         originalPanY_;
    RadiologyScene::Corner         oppositeCorner_;
    double                         oppositeX_;
    double                         oppositeY_;
    double                         baseScaling_;

    static double ComputeDistance(double x1,
                                  double y1,
                                  double x2,
                                  double y2)
    {
      double dx = x1 - x2;
      double dy = y1 - y2;
      return sqrt(dx * dx + dy * dy);
    }
      
    class UndoRedoCommand : public RadiologyLayerCommand
    {
    private:
      double   sourceSpacingX_;
      double   sourceSpacingY_;
      double   sourcePanX_;
      double   sourcePanY_;
      double   targetSpacingX_;
      double   targetSpacingY_;
      double   targetPanX_;
      double   targetPanY_;

    protected:
      virtual void UndoInternal(RadiologyScene::Layer& layer) const
      {
        layer.SetPixelSpacing(sourceSpacingX_, sourceSpacingY_);
        layer.SetPan(sourcePanX_, sourcePanY_);
      }

      virtual void RedoInternal(RadiologyScene::Layer& layer) const
      {
        layer.SetPixelSpacing(targetSpacingX_, targetSpacingY_);
        layer.SetPan(targetPanX_, targetPanY_);
      }

    public:
      UndoRedoCommand(const RadiologyLayerResizeTracker& tracker) :
        RadiologyLayerCommand(tracker.accessor_),
        sourceSpacingX_(tracker.originalSpacingX_),
        sourceSpacingY_(tracker.originalSpacingY_),
        sourcePanX_(tracker.originalPanX_),
        sourcePanY_(tracker.originalPanY_),
        targetSpacingX_(tracker.accessor_.GetLayer().GetPixelSpacingX()),
        targetSpacingY_(tracker.accessor_.GetLayer().GetPixelSpacingY()),
        targetPanX_(tracker.accessor_.GetLayer().GetPanX()),
        targetPanY_(tracker.accessor_.GetLayer().GetPanY())
      {
      }
    };


  public:
    RadiologyLayerResizeTracker(UndoRedoStack& undoRedoStack,
                                RadiologyScene& scene,
                                size_t layer,
                                double x,
                                double y,
                                RadiologyScene::Corner corner,
                                bool roundScaling) :
      undoRedoStack_(undoRedoStack),
      accessor_(scene, layer),
      roundScaling_(roundScaling)
    {
      if (accessor_.IsValid() &&
          accessor_.GetLayer().IsResizeable())
      {
        originalSpacingX_ = accessor_.GetLayer().GetPixelSpacingX();
        originalSpacingY_ = accessor_.GetLayer().GetPixelSpacingY();
        originalPanX_ = accessor_.GetLayer().GetPanX();
        originalPanY_ = accessor_.GetLayer().GetPanY();

        switch (corner)
        {
          case RadiologyScene::Corner_TopLeft:
            oppositeCorner_ = RadiologyScene::Corner_BottomRight;
            break;

          case RadiologyScene::Corner_TopRight:
            oppositeCorner_ = RadiologyScene::Corner_BottomLeft;
            break;

          case RadiologyScene::Corner_BottomLeft:
            oppositeCorner_ = RadiologyScene::Corner_TopRight;
            break;

          case RadiologyScene::Corner_BottomRight:
            oppositeCorner_ = RadiologyScene::Corner_TopLeft;
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        accessor_.GetLayer().GetCorner(oppositeX_, oppositeY_, oppositeCorner_);

        double d = ComputeDistance(x, y, oppositeX_, oppositeY_);
        if (d >= std::numeric_limits<float>::epsilon())
        {
          baseScaling_ = 1.0 / d;
        }
        else
        {
          // Avoid division by zero in extreme cases
          accessor_.Invalidate();
        }
      }
    }

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    virtual void MouseUp()
    {
      if (accessor_.IsValid() &&
          accessor_.GetLayer().IsResizeable())
      {
        undoRedoStack_.Add(new UndoRedoCommand(*this));
      }
    }

    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY)
    {
      static const double ROUND_SCALING = 0.1;
        
      if (accessor_.IsValid() &&
          accessor_.GetLayer().IsResizeable())
      {
        double scaling = ComputeDistance(oppositeX_, oppositeY_, sceneX, sceneY) * baseScaling_;

        if (roundScaling_)
        {
          scaling = boost::math::round<double>((scaling / ROUND_SCALING) * ROUND_SCALING);
        }
          
        RadiologyScene::Layer& layer = accessor_.GetLayer();
        layer.SetPixelSpacing(scaling * originalSpacingX_,
                              scaling * originalSpacingY_);

        // Keep the opposite corner at a fixed location
        double ox, oy;
        layer.GetCorner(ox, oy, oppositeCorner_);
        layer.SetPan(layer.GetPanX() + oppositeX_ - ox,
                     layer.GetPanY() + oppositeY_ - oy);
      }
    }
  };


  class RadiologyWindowingTracker : public IWorldSceneMouseTracker
  {   
  public:
    enum Action
    {
      Action_IncreaseWidth,
      Action_DecreaseWidth,
      Action_IncreaseCenter,
      Action_DecreaseCenter
    };
    
  private:
    UndoRedoStack&    undoRedoStack_;
    RadiologyScene&   scene_;
    int               clickX_;
    int               clickY_;
    Action            leftAction_;
    Action            rightAction_;
    Action            upAction_;
    Action            downAction_;
    float             strength_;
    float             sourceCenter_;
    float             sourceWidth_;

    static void ComputeAxisEffect(int& deltaCenter,
                                  int& deltaWidth,
                                  int delta,
                                  Action actionNegative,
                                  Action actionPositive)
    {
      if (delta < 0)
      {
        switch (actionNegative)
        {
          case Action_IncreaseWidth:
            deltaWidth = -delta;
            break;

          case Action_DecreaseWidth:
            deltaWidth = delta;
            break;

          case Action_IncreaseCenter:
            deltaCenter = -delta;
            break;

          case Action_DecreaseCenter:
            deltaCenter = delta;
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
      }
      else if (delta > 0)
      {
        switch (actionPositive)
        {
          case Action_IncreaseWidth:
            deltaWidth = delta;
            break;

          case Action_DecreaseWidth:
            deltaWidth = -delta;
            break;

          case Action_IncreaseCenter:
            deltaCenter = delta;
            break;

          case Action_DecreaseCenter:
            deltaCenter = -delta;
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
      }
    }
    
    
    class UndoRedoCommand : public UndoRedoStack::ICommand
    {
    private:
      RadiologyScene&  scene_;
      float            sourceCenter_;
      float            sourceWidth_;
      float            targetCenter_;
      float            targetWidth_;

    public:
      UndoRedoCommand(const RadiologyWindowingTracker& tracker) :
        scene_(tracker.scene_),
        sourceCenter_(tracker.sourceCenter_),
        sourceWidth_(tracker.sourceWidth_)
      {
        scene_.GetWindowingWithDefault(targetCenter_, targetWidth_);
      }

      virtual void Undo() const
      {
        scene_.SetWindowing(sourceCenter_, sourceWidth_);
      }
      
      virtual void Redo() const
      {
        scene_.SetWindowing(targetCenter_, targetWidth_);
      }
    };


  public:
    RadiologyWindowingTracker(UndoRedoStack& undoRedoStack,
                              RadiologyScene& scene,
                              int x,
                              int y,
                              Action leftAction,
                              Action rightAction,
                              Action upAction,
                              Action downAction) :
      undoRedoStack_(undoRedoStack),
      scene_(scene),
      clickX_(x),
      clickY_(y),
      leftAction_(leftAction),
      rightAction_(rightAction),
      upAction_(upAction),
      downAction_(downAction)
    {
      scene_.GetWindowingWithDefault(sourceCenter_, sourceWidth_);

      float minValue, maxValue;
      scene.GetRange(minValue, maxValue);

      assert(minValue <= maxValue);

      float tmp;
      
      float delta = (maxValue - minValue);
      if (delta <= 1)
      {
        tmp = 0;
      }
      else
      {
        // NB: Visual Studio 2008 does not provide "log2f()", so we
        // implement it by ourselves
        tmp = logf(delta) / logf(2.0f);
      }

      strength_ = tmp - 7;
      if (strength_ < 1)
      {
        strength_ = 1;
      }
    }

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    virtual void MouseUp()
    {
      undoRedoStack_.Add(new UndoRedoCommand(*this));
    }


    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY)
    {
      // https://bitbucket.org/osimis/osimis-webviewer-plugin/src/master/frontend/src/app/viewport/image-plugins/windowing-viewport-tool.class.js

      static const float SCALE = 1.0;
      
      int deltaCenter = 0;
      int deltaWidth = 0;

      ComputeAxisEffect(deltaCenter, deltaWidth, displayX - clickX_, leftAction_, rightAction_);
      ComputeAxisEffect(deltaCenter, deltaWidth, displayY - clickY_, upAction_, downAction_);

      float newCenter = sourceCenter_ + (deltaCenter / SCALE * strength_);
      float newWidth = sourceWidth_ + (deltaWidth / SCALE * strength_);
      scene_.SetWindowing(newCenter, newWidth);
    }
  };


  class RadiologyWidget :
    public WorldSceneWidget,
    public IObserver
  {
  private:
    RadiologyScene&                scene_;
    std::auto_ptr<Orthanc::Image>  floatBuffer_;
    std::auto_ptr<CairoSurface>    cairoBuffer_;
    bool                           invert_;
    ImageInterpolation             interpolation_;
    bool                           hasSelection_;
    size_t                         selectedLayer_;

    virtual bool RenderInternal(unsigned int width,
                                unsigned int height,
                                ImageInterpolation interpolation)
    {
      float windowCenter, windowWidth;
      scene_.GetWindowingWithDefault(windowCenter, windowWidth);
      
      float x0 = windowCenter - windowWidth / 2.0f;
      float x1 = windowCenter + windowWidth / 2.0f;

      if (windowWidth <= 0.001f)  // Avoid division by zero at (*)
      {
        return false;
      }
      else
      {
        if (floatBuffer_.get() == NULL ||
            floatBuffer_->GetWidth() != width ||
            floatBuffer_->GetHeight() != height)
        {
          floatBuffer_.reset(new Orthanc::Image(Orthanc::PixelFormat_Float32, width, height, false));
        }

        if (cairoBuffer_.get() == NULL ||
            cairoBuffer_->GetWidth() != width ||
            cairoBuffer_->GetHeight() != height)
        {
          cairoBuffer_.reset(new CairoSurface(width, height));
        }

        scene_.Render(*floatBuffer_, GetView().GetMatrix(), interpolation);
        
        // Conversion from Float32 to BGRA32 (cairo). Very similar to
        // GrayscaleFrameRenderer => TODO MERGE?

        Orthanc::ImageAccessor target;
        cairoBuffer_->GetWriteableAccessor(target);

        float scaling = 255.0f / (x1 - x0);
        
        for (unsigned int y = 0; y < height; y++)
        {
          const float* p = reinterpret_cast<const float*>(floatBuffer_->GetConstRow(y));
          uint8_t* q = reinterpret_cast<uint8_t*>(target.GetRow(y));

          for (unsigned int x = 0; x < width; x++, p++, q += 4)
          {
            uint8_t v = 0;
            if (*p >= x1)
            {
              v = 255;
            }
            else if (*p <= x0)
            {
              v = 0;
            }
            else
            {
              // https://en.wikipedia.org/wiki/Linear_interpolation
              v = static_cast<uint8_t>(scaling * (*p - x0));  // (*)
            }

            if (invert_)
            {
              v = 255 - v;
            }

            q[0] = v;
            q[1] = v;
            q[2] = v;
            q[3] = 255;
          }
        }

        return true;
      }
    }

  protected:
    virtual Extent2D GetSceneExtent()
    {
      return scene_.GetSceneExtent();
    }

    virtual bool RenderScene(CairoContext& context,
                             const ViewportGeometry& view)
    {
      cairo_t* cr = context.GetObject();

      if (RenderInternal(context.GetWidth(), context.GetHeight(), interpolation_))
      {
        // https://www.cairographics.org/FAQ/#paint_from_a_surface
        cairo_save(cr);
        cairo_identity_matrix(cr);
        cairo_set_source_surface(cr, cairoBuffer_->GetObject(), 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
      }
      else
      {
        // https://www.cairographics.org/FAQ/#clear_a_surface
        context.SetSourceColor(0, 0, 0);
        cairo_paint(cr);
      }

      if (hasSelection_)
      {
        scene_.DrawBorder(context, selectedLayer_, view.GetZoom());
      }

      return true;
    }

  public:
    RadiologyWidget(MessageBroker& broker,
                    RadiologyScene& scene,
                    const std::string& name) :
      WorldSceneWidget(name),
      IObserver(broker),
      scene_(scene),
      invert_(false),
      interpolation_(ImageInterpolation_Nearest),
      hasSelection_(false),
      selectedLayer_(0)    // Dummy initialization
    {
      scene.RegisterObserverCallback(
        new Callable<RadiologyWidget, RadiologyScene::GeometryChangedMessage>
        (*this, &RadiologyWidget::OnGeometryChanged));

      scene.RegisterObserverCallback(
        new Callable<RadiologyWidget, RadiologyScene::ContentChangedMessage>
        (*this, &RadiologyWidget::OnContentChanged));
    }

    RadiologyScene& GetScene() const
    {
      return scene_;
    }

    void Unselect()
    {
      hasSelection_ = false;
    }

    void Select(size_t layer)
    {
      hasSelection_ = true;
      selectedLayer_ = layer;
    }

    bool LookupSelectedLayer(size_t& layer)
    {
      if (hasSelection_)
      {
        layer = selectedLayer_;
        return true;
      }
      else
      {
        return false;
      }
    }

    void OnGeometryChanged(const RadiologyScene::GeometryChangedMessage& message)
    {
      LOG(INFO) << "Geometry has changed";
      FitContent();
    }

    void OnContentChanged(const RadiologyScene::ContentChangedMessage& message)
    {
      LOG(INFO) << "Content has changed";
      NotifyContentChanged();
    }

    void SetInvert(bool invert)
    {
      if (invert_ != invert)
      {
        invert_ = invert;
        NotifyContentChanged();
      }
    }

    void SwitchInvert()
    {
      invert_ = !invert_;
      NotifyContentChanged();
    }

    bool IsInverted() const
    {
      return invert_;
    }

    void SetInterpolation(ImageInterpolation interpolation)
    {
      if (interpolation_ != interpolation)
      {
        interpolation_ = interpolation;
        NotifyContentChanged();
      }
    }

    ImageInterpolation GetInterpolation() const
    {
      return interpolation_;
    }
  };

  
  namespace Samples
  {
    class RadiologyEditorInteractor :
      public IWorldSceneInteractor,
      public IObserver
    {
    private:
      enum Tool
      {
        Tool_Move,
        Tool_Rotate,
        Tool_Crop,
        Tool_Resize,
        Tool_Windowing
      };
        

      UndoRedoStack      undoRedoStack_;
      Tool               tool_;


      static double GetHandleSize()
      {
        return 10.0;
      }
    
         
    public:
      RadiologyEditorInteractor(MessageBroker& broker) :
        IObserver(broker),
        tool_(Tool_Move)
      {
      }
    
      virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& worldWidget,
                                                          const ViewportGeometry& view,
                                                          MouseButton button,
                                                          KeyboardModifiers modifiers,
                                                          int viewportX,
                                                          int viewportY,
                                                          double x,
                                                          double y,
                                                          IStatusBar* statusBar)
      {
        RadiologyWidget& widget = dynamic_cast<RadiologyWidget&>(worldWidget);

        if (button == MouseButton_Left)
        {
          size_t selected;
        
          if (tool_ == Tool_Windowing)
          {
            return new RadiologyWindowingTracker(undoRedoStack_, widget.GetScene(),
                                                 viewportX, viewportY,
                                                 RadiologyWindowingTracker::Action_DecreaseWidth,
                                                 RadiologyWindowingTracker::Action_IncreaseWidth,
                                                 RadiologyWindowingTracker::Action_DecreaseCenter,
                                                 RadiologyWindowingTracker::Action_IncreaseCenter);
          }
          else if (!widget.LookupSelectedLayer(selected))
          {
            // No layer is currently selected
            size_t layer;
            if (widget.GetScene().LookupLayer(layer, x, y))
            {
              widget.Select(layer);
            }

            return NULL;
          }
          else if (tool_ == Tool_Crop ||
                   tool_ == Tool_Resize)
          {
            RadiologyScene::LayerAccessor accessor(widget.GetScene(), selected);
            RadiologyScene::Corner corner;
            if (accessor.GetLayer().LookupCorner(corner, x, y, view.GetZoom(), GetHandleSize()))
            {
              switch (tool_)
              {
                case Tool_Crop:
                  return new RadiologyLayerCropTracker(undoRedoStack_, widget.GetScene(), view, selected, x, y, corner);

                case Tool_Resize:
                  return new RadiologyLayerResizeTracker(undoRedoStack_, widget.GetScene(), selected, x, y, corner,
                                                         (modifiers & KeyboardModifiers_Shift));

                default:
                  throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
              }
            }
            else
            {
              size_t layer;
            
              if (widget.GetScene().LookupLayer(layer, x, y))
              {
                widget.Select(layer);
              }
              else
              {
                widget.Unselect();
              }
            
              return NULL;
            }
          }
          else
          {
            size_t layer;

            if (widget.GetScene().LookupLayer(layer, x, y))
            {
              if (layer == selected)
              {
                switch (tool_)
                {
                  case Tool_Move:
                    return new RadiologyLayerMoveTracker(undoRedoStack_, widget.GetScene(), layer, x, y,
                                                         (modifiers & KeyboardModifiers_Shift));

                  case Tool_Rotate:
                    return new RadiologyLayerRotateTracker(undoRedoStack_, widget.GetScene(), view, layer, x, y,
                                                           (modifiers & KeyboardModifiers_Shift));
                
                  default:
                    break;
                }

                return NULL;
              }
              else
              {
                widget.Select(layer);
                return NULL;
              }
            }
            else
            {
              widget.Unselect();
              return NULL;
            }
          }
        }
        else
        {
          return NULL;
        }
      }

      virtual void MouseOver(CairoContext& context,
                             WorldSceneWidget& worldWidget,
                             const ViewportGeometry& view,
                             double x,
                             double y,
                             IStatusBar* statusBar)
      {
        RadiologyWidget& widget = dynamic_cast<RadiologyWidget&>(worldWidget);

#if 0
        if (statusBar != NULL)
        {
          char buf[64];
          sprintf(buf, "X = %.02f Y = %.02f (in cm)", x / 10.0, y / 10.0);
          statusBar->SetMessage(buf);
        }
#endif

        size_t selected;

        if (widget.LookupSelectedLayer(selected) &&
            (tool_ == Tool_Crop ||
             tool_ == Tool_Resize))
        {
          RadiologyScene::LayerAccessor accessor(widget.GetScene(), selected);
        
          RadiologyScene::Corner corner;
          if (accessor.GetLayer().LookupCorner(corner, x, y, view.GetZoom(), GetHandleSize()))
          {
            accessor.GetLayer().GetCorner(x, y, corner);
          
            double z = 1.0 / view.GetZoom();
          
            context.SetSourceColor(255, 0, 0);
            cairo_t* cr = context.GetObject();
            cairo_set_line_width(cr, 2.0 * z);
            cairo_move_to(cr, x - GetHandleSize() * z, y - GetHandleSize() * z);
            cairo_line_to(cr, x + GetHandleSize() * z, y - GetHandleSize() * z);
            cairo_line_to(cr, x + GetHandleSize() * z, y + GetHandleSize() * z);
            cairo_line_to(cr, x - GetHandleSize() * z, y + GetHandleSize() * z);
            cairo_line_to(cr, x - GetHandleSize() * z, y - GetHandleSize() * z);
            cairo_stroke(cr);
          }
        }
      }

      virtual void MouseWheel(WorldSceneWidget& widget,
                              MouseWheelDirection direction,
                              KeyboardModifiers modifiers,
                              IStatusBar* statusBar)
      {
      }

      virtual void KeyPressed(WorldSceneWidget& worldWidget,
                              KeyboardKeys key,
                              char keyChar,
                              KeyboardModifiers modifiers,
                              IStatusBar* statusBar)
      {
        RadiologyWidget& widget = dynamic_cast<RadiologyWidget&>(worldWidget);

        switch (keyChar)
        {
          case 'a':
            widget.FitContent();
            break;

          case 'c':
            tool_ = Tool_Crop;
            break;

          case 'e':
          {
            Orthanc::DicomMap tags;

            // Minimal set of tags to generate a valid CR image
            tags.SetValue(Orthanc::DICOM_TAG_ACCESSION_NUMBER, "NOPE", false);
            tags.SetValue(Orthanc::DICOM_TAG_BODY_PART_EXAMINED, "PELVIS", false);
            tags.SetValue(Orthanc::DICOM_TAG_INSTANCE_NUMBER, "1", false);
            //tags.SetValue(Orthanc::DICOM_TAG_LATERALITY, "", false);
            tags.SetValue(Orthanc::DICOM_TAG_MANUFACTURER, "OSIMIS", false);
            tags.SetValue(Orthanc::DICOM_TAG_MODALITY, "CR", false);
            tags.SetValue(Orthanc::DICOM_TAG_PATIENT_BIRTH_DATE, "20000101", false);
            tags.SetValue(Orthanc::DICOM_TAG_PATIENT_ID, "hello", false);
            tags.SetValue(Orthanc::DICOM_TAG_PATIENT_NAME, "HELLO^WORLD", false);
            tags.SetValue(Orthanc::DICOM_TAG_PATIENT_ORIENTATION, "", false);
            tags.SetValue(Orthanc::DICOM_TAG_PATIENT_SEX, "M", false);
            tags.SetValue(Orthanc::DICOM_TAG_REFERRING_PHYSICIAN_NAME, "HOUSE^MD", false);
            tags.SetValue(Orthanc::DICOM_TAG_SERIES_NUMBER, "1", false);
            tags.SetValue(Orthanc::DICOM_TAG_SOP_CLASS_UID, "1.2.840.10008.5.1.4.1.1.1", false);
            tags.SetValue(Orthanc::DICOM_TAG_STUDY_ID, "STUDY", false);
            tags.SetValue(Orthanc::DICOM_TAG_VIEW_POSITION, "", false);

            widget.GetScene().Export(tags, 0.1, 0.1, widget.IsInverted(),
                                     widget.GetInterpolation(), EXPORT_USING_PAM);
            break;
          }

          case 'i':
            widget.SwitchInvert();
            break;
        
          case 'm':
            tool_ = Tool_Move;
            break;

          case 'n':
          {
            switch (widget.GetInterpolation())
            {
              case ImageInterpolation_Nearest:
                LOG(INFO) << "Switching to bilinear interpolation";
                widget.SetInterpolation(ImageInterpolation_Bilinear);
                break;
              
              case ImageInterpolation_Bilinear:
                LOG(INFO) << "Switching to nearest neighbor interpolation";
                widget.SetInterpolation(ImageInterpolation_Nearest);
                break;

              default:
                throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
            }
          
            break;
          }
        
          case 'r':
            tool_ = Tool_Rotate;
            break;

          case 's':
            tool_ = Tool_Resize;
            break;

          case 'w':
            tool_ = Tool_Windowing;
            break;

          case 'y':
            if (modifiers & KeyboardModifiers_Control)
            {
              undoRedoStack_.Redo();
              widget.NotifyContentChanged();
            }
            break;

          case 'z':
            if (modifiers & KeyboardModifiers_Control)
            {
              undoRedoStack_.Undo();
              widget.NotifyContentChanged();
            }
            break;
        
          default:
            break;
        }
      }
    };

  
  
    class SingleFrameEditorApplication :
      public SampleSingleCanvasApplicationBase,
      public IObserver
    {
    private:
      std::auto_ptr<OrthancApiClient>  orthancApiClient_;
      std::auto_ptr<RadiologyScene>    scene_;
      RadiologyEditorInteractor        interactor_;

    public:
      SingleFrameEditorApplication(MessageBroker& broker) :
        IObserver(broker),
        interactor_(broker)
      {
      }

      virtual ~SingleFrameEditorApplication()
      {
        LOG(WARNING) << "Destroying the application";
      }
      
      virtual void DeclareStartupOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
          ("instance", boost::program_options::value<std::string>(),
           "Orthanc ID of the instance")
          ("frame", boost::program_options::value<unsigned int>()->default_value(0),
           "Number of the frame, for multi-frame DICOM instances")
          ;

        options.add(generic);
      }

      virtual void Initialize(StoneApplicationContext* context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        context_ = context;

        statusBar.SetMessage("Use the key \"a\" to reinitialize the layout");
        statusBar.SetMessage("Use the key \"c\" to crop");
        statusBar.SetMessage("Use the key \"e\" to export DICOM to the Orthanc server");
        statusBar.SetMessage("Use the key \"f\" to switch full screen");
        statusBar.SetMessage("Use the key \"i\" to invert contrast");
        statusBar.SetMessage("Use the key \"m\" to move objects");
        statusBar.SetMessage("Use the key \"n\" to switch between nearest neighbor and bilinear interpolation");
        statusBar.SetMessage("Use the key \"r\" to rotate objects");
        statusBar.SetMessage("Use the key \"s\" to resize objects (not applicable to DICOM layers)");
        statusBar.SetMessage("Use the key \"w\" to change windowing");
        
        statusBar.SetMessage("Use the key \"ctrl-z\" to undo action");
        statusBar.SetMessage("Use the key \"ctrl-y\" to redo action");

        if (parameters.count("instance") != 1)
        {
          LOG(ERROR) << "The instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string instance = parameters["instance"].as<std::string>();
        int frame = parameters["frame"].as<unsigned int>();

        orthancApiClient_.reset(new OrthancApiClient(GetBroker(), context_->GetWebService()));

        Orthanc::FontRegistry fonts;
        fonts.AddFromResource(Orthanc::EmbeddedResources::FONT_UBUNTU_MONO_BOLD_16);
        
        scene_.reset(new RadiologyScene(GetBroker(), *orthancApiClient_));
        scene_->LoadDicomFrame(instance, frame, false); //.SetPan(200, 0);
        //scene_->LoadDicomFrame("61f3143e-96f34791-ad6bbb8d-62559e75-45943e1b", 0, false);

        {
          RadiologyScene::Layer& layer = scene_->LoadText(fonts.GetFont(0), "Hello\nworld");
          //dynamic_cast<RadiologyScene::Layer&>(layer).SetForegroundValue(256);
          dynamic_cast<RadiologyScene::Layer&>(layer).SetResizeable(true);
        }
        
        {
          RadiologyScene::Layer& layer = scene_->LoadTestBlock(100, 50);
          //dynamic_cast<RadiologyScene::Layer&>(layer).SetForegroundValue(256);
          dynamic_cast<RadiologyScene::Layer&>(layer).SetResizeable(true);
          dynamic_cast<RadiologyScene::Layer&>(layer).SetPan(0, 200);
        }
        
        
        mainWidget_ = new RadiologyWidget(GetBroker(), *scene_, "main-widget");
        mainWidget_->SetTransmitMouseOver(true);
        mainWidget_->SetInteractor(interactor_);

        //scene_->SetWindowing(128, 256);
      }
    };
  }
}
