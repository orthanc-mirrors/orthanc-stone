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
#include <Core/Logging.h>
#include <Plugins/Samples/Common/DicomDatasetReader.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>


#include <boost/math/constants/constants.hpp>

namespace OrthancStone
{
  class BitmapStack :
    public IObserver,
    public IObservable
  {
  public:
    typedef OriginMessage<MessageType_Widget_GeometryChanged, BitmapStack> GeometryChangedMessage;
    typedef OriginMessage<MessageType_Widget_ContentChanged, BitmapStack> ContentChangedMessage;


    enum Corner
    {
      Corner_TopLeft,
      Corner_TopRight,
      Corner_BottomLeft,
      Corner_BottomRight
    };



    class Bitmap : public boost::noncopyable
    {
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


    public:
      Bitmap(size_t index) :
        index_(index),
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

      virtual ~Bitmap()
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


      void CheckSize(unsigned int width,
                     unsigned int height)
      {
        if (hasSize_ &&
            (width != width_ ||
             height != height_))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize);
        }
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


      bool Contains(double x,
                    double y) const
      {
        ApplyTransform(x, y, transformInverse_);
        
        unsigned int cropX, cropY, cropWidth, cropHeight;
        GetCrop(cropX, cropY, cropWidth, cropHeight);

        return (x >= cropX && x <= cropX + cropWidth &&
                y >= cropY && y <= cropY + cropHeight);
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
                                       float& width) const
      {
        return false;
      }

      virtual void Render(Orthanc::ImageAccessor& buffer,
                          const ViewportGeometry& view,
                          ImageInterpolation interpolation) const = 0;

      virtual bool GetRange(float& minValue,
                            float& maxValue) const = 0;
    }; 


    class BitmapAccessor : public boost::noncopyable
    {
    private:
      BitmapStack&  stack_;
      size_t        index_;
      Bitmap*       bitmap_;

    public:
      BitmapAccessor(BitmapStack& stack,
                     size_t index) :
        stack_(stack),
        index_(index)
      {
        Bitmaps::iterator bitmap = stack.bitmaps_.find(index);
        if (bitmap == stack.bitmaps_.end())
        {
          bitmap_ = NULL;
        }
        else
        {
          assert(bitmap->second != NULL);
          bitmap_ = bitmap->second;
        }
      }

      BitmapAccessor(BitmapStack& stack,
                     double x,
                     double y) :
        stack_(stack),
        index_(0)  // Dummy initialization
      {
        if (stack.LookupBitmap(index_, x, y))
        {
          Bitmaps::iterator bitmap = stack.bitmaps_.find(index_);
          
          if (bitmap == stack.bitmaps_.end())
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
          }
          else
          {
            assert(bitmap->second != NULL);
            bitmap_ = bitmap->second;
          }
        }
        else
        {
          bitmap_ = NULL;
        }
      }

      void Invalidate()
      {
        bitmap_ = NULL;
      }

      bool IsValid() const
      {
        return bitmap_ != NULL;
      }

      BitmapStack& GetStack() const
      {
        if (IsValid())
        {
          return stack_;
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

      Bitmap& GetBitmap() const
      {
        if (IsValid())
        {
          return *bitmap_;
        }
        else
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
        }
      }    
    };    


    class AlphaBitmap : public Bitmap
    {
    private:
      const BitmapStack&                     stack_;
      std::auto_ptr<Orthanc::ImageAccessor>  alpha_;      // Grayscale8
      bool                                   useWindowing_;
      float                                  foreground_;

    public:
      AlphaBitmap(size_t index,
                  const BitmapStack& stack) :
        Bitmap(index),
        stack_(stack),
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


      virtual void Render(Orthanc::ImageAccessor& buffer,
                          const ViewportGeometry& view,
                          ImageInterpolation interpolation) const
      {
        if (buffer.GetFormat() != Orthanc::PixelFormat_Float32)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
        }

        unsigned int cropX, cropY, cropWidth, cropHeight;
        GetCrop(cropX, cropY, cropWidth, cropHeight);

        Matrix m = LinearAlgebra::Product(view.GetMatrix(),
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
          if (stack_.GetWindowing(center, width))
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
    class DicomBitmap : public Bitmap
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
      DicomBitmap(size_t index) :
        Bitmap(index)
      {
      }
      
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
                          const ViewportGeometry& view,
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

          Matrix m = LinearAlgebra::Product(view.GetMatrix(),
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




    typedef std::map<size_t, Bitmap*>  Bitmaps;
        
    OrthancApiClient&  orthanc_;
    size_t             countBitmaps_;
    bool               hasWindowing_;
    float              windowingCenter_;
    float              windowingWidth_;
    Bitmaps            bitmaps_;
    bool               hasSelection_;
    size_t             selectedBitmap_;

  public:
    BitmapStack(MessageBroker& broker,
                OrthancApiClient& orthanc) :
      IObserver(broker),
      IObservable(broker),
      orthanc_(orthanc),
      countBitmaps_(0),
      hasWindowing_(false),
      windowingCenter_(0),  // Dummy initialization
      windowingWidth_(0),   // Dummy initialization
      hasSelection_(false),
      selectedBitmap_(0)    // Dummy initialization
    {
    }


    void Unselect()
    {
      hasSelection_ = false;
    }


    void Select(size_t bitmap)
    {
      hasSelection_ = true;
      selectedBitmap_ = bitmap;
    }


    bool GetSelectedBitmap(size_t& bitmap) const
    {
      if (hasSelection_)
      {
        bitmap = selectedBitmap_;
        return true;
      }
      else
      {
        return false;
      }
    }
    
    
    virtual ~BitmapStack()
    {
      for (Bitmaps::iterator it = bitmaps_.begin(); it != bitmaps_.end(); it++)
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

      //EmitMessage(ContentChangedMessage(*this));
    }
    

    Bitmap& LoadText(const Orthanc::Font& font,
                     const std::string& utf8)
    {
      size_t bitmap = countBitmaps_++;

      std::auto_ptr<AlphaBitmap>  alpha(new AlphaBitmap(bitmap, *this));
      alpha->LoadText(font, utf8);

      AlphaBitmap* ptr = alpha.get();
      bitmaps_[bitmap] = alpha.release();

      return *ptr;
    }

    
    Bitmap& LoadTestBlock(unsigned int width,
                          unsigned int height)
    {
      size_t bitmap = countBitmaps_++;

      std::auto_ptr<AlphaBitmap>  alpha(new AlphaBitmap(bitmap, *this));

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

      alpha->SetAlpha(block.release());

      AlphaBitmap* ptr = alpha.get();
      bitmaps_[bitmap] = alpha.release();
      
      return *ptr;
    }

    
    Bitmap& LoadFrame(const std::string& instance,
                     unsigned int frame,
                     bool httpCompression)
    {
      size_t bitmap = countBitmaps_++;

      bitmaps_[bitmap] = new DicomBitmap(bitmap);
      
      {
        IWebService::Headers headers;
        std::string uri = "/instances/" + instance + "/tags";
        orthanc_.GetBinaryAsync(uri, headers,
                                new Callable<BitmapStack, OrthancApiClient::BinaryResponseReadyMessage>
                                (*this, &BitmapStack::OnTagsReceived), NULL,
                                new Orthanc::SingleValueObject<size_t>(bitmap));
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
                                new Callable<BitmapStack, OrthancApiClient::BinaryResponseReadyMessage>
                                (*this, &BitmapStack::OnFrameReceived), NULL,
                                new Orthanc::SingleValueObject<size_t>(bitmap));
      }

      return *bitmaps_[bitmap];
    }

    
    void OnTagsReceived(const OrthancApiClient::BinaryResponseReadyMessage& message)
    {
      size_t index = dynamic_cast<Orthanc::SingleValueObject<size_t>*>(message.Payload.get())->GetValue();

      LOG(INFO) << "JSON received: " << message.Uri.c_str()
                << " (" << message.AnswerSize << " bytes) for bitmap " << index;
      
      Bitmaps::iterator bitmap = bitmaps_.find(index);
      if (bitmap != bitmaps_.end())
      {
        assert(bitmap->second != NULL);
        
        OrthancPlugins::FullOrthancDataset dicom(message.Answer, message.AnswerSize);
        dynamic_cast<DicomBitmap*>(bitmap->second)->SetDicomTags(dicom);

        float c, w;
        if (!hasWindowing_ &&
            bitmap->second->GetDefaultWindowing(c, w))
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
      size_t index = dynamic_cast<Orthanc::SingleValueObject<size_t>*>(message.Payload.get())->GetValue();
      
      LOG(INFO) << "DICOM frame received: " << message.Uri.c_str()
                << " (" << message.AnswerSize << " bytes) for bitmap " << index;
      
      Bitmaps::iterator bitmap = bitmaps_.find(index);
      if (bitmap != bitmaps_.end())
      {
        assert(bitmap->second != NULL);

        std::string content;
        if (message.AnswerSize > 0)
        {
          content.assign(reinterpret_cast<const char*>(message.Answer), message.AnswerSize);
        }
        
        std::auto_ptr<Orthanc::PamReader> reader(new Orthanc::PamReader);
        reader->ReadFromMemory(content);
        dynamic_cast<DicomBitmap*>(bitmap->second)->SetSourceImage(reader.release());

        EmitMessage(ContentChangedMessage(*this));
      }
    }


    Extent2D GetSceneExtent() const
    {
      Extent2D extent;

      for (Bitmaps::const_iterator it = bitmaps_.begin();
           it != bitmaps_.end(); ++it)
      {
        assert(it->second != NULL);
        extent.Union(it->second->GetExtent());
      }

      return extent;
    }
    

    void Render(Orthanc::ImageAccessor& buffer,
                const ViewportGeometry& view,
                ImageInterpolation interpolation) const
    {
      Orthanc::ImageProcessing::Set(buffer, 0);

      // Render layers in the background-to-foreground order
      for (size_t index = 0; index < countBitmaps_; index++)
      {
        Bitmaps::const_iterator it = bitmaps_.find(index);
        if (it != bitmaps_.end())
        {
          assert(it->second != NULL);
          it->second->Render(buffer, view, interpolation);
        }
      }
    }


    bool LookupBitmap(size_t& index /* out */,
                      double x,
                      double y) const
    {
      // Render layers in the foreground-to-background order
      for (size_t i = countBitmaps_; i > 0; i--)
      {
        index = i - 1;
        Bitmaps::const_iterator it = bitmaps_.find(index);
        if (it != bitmaps_.end())
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

    void DrawControls(CairoContext& context,
                      double zoom)
    {
      if (hasSelection_)
      {
        Bitmaps::const_iterator bitmap = bitmaps_.find(selectedBitmap_);
        
        if (bitmap != bitmaps_.end())
        {
          context.SetSourceColor(255, 0, 0);
          //view.ApplyTransform(context);
          bitmap->second->DrawBorders(context, zoom);
        }
      }
    }


    void GetRange(float& minValue,
                  float& maxValue) const
    {
      bool first = true;
      
      for (Bitmaps::const_iterator it = bitmaps_.begin();
           it != bitmaps_.end(); it++)
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


  class BitmapCommandBase : public UndoRedoStack::ICommand
  {
  private:
    BitmapStack&  stack_;
    size_t        bitmap_;

  protected:
    virtual void UndoInternal(BitmapStack::Bitmap& bitmap) const = 0;

    virtual void RedoInternal(BitmapStack::Bitmap& bitmap) const = 0;

  public:
    BitmapCommandBase(BitmapStack& stack,
                      size_t bitmap) :
      stack_(stack),
      bitmap_(bitmap)
    {
    }

    BitmapCommandBase(const BitmapStack::BitmapAccessor& accessor) :
      stack_(accessor.GetStack()),
      bitmap_(accessor.GetIndex())
    {
    }

    virtual void Undo() const
    {
      BitmapStack::BitmapAccessor accessor(stack_, bitmap_);

      if (accessor.IsValid())
      {
        UndoInternal(accessor.GetBitmap());
      }
    }

    virtual void Redo() const
    {
      BitmapStack::BitmapAccessor accessor(stack_, bitmap_);

      if (accessor.IsValid())
      {
        RedoInternal(accessor.GetBitmap());
      }
    }
  };


  class RotateBitmapTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&               undoRedoStack_;
    BitmapStack::BitmapAccessor  accessor_;
    double                       centerX_;
    double                       centerY_;
    double                       originalAngle_;
    double                       clickAngle_;
    bool                         roundAngles_;

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


    class UndoRedoCommand : public BitmapCommandBase
    {
    private:
      double  sourceAngle_;
      double  targetAngle_;

      static int ToDegrees(double angle)
      {
        return static_cast<int>(round(angle * 180.0 / boost::math::constants::pi<double>()));
      }
      
    protected:
      virtual void UndoInternal(BitmapStack::Bitmap& bitmap) const
      {
        LOG(INFO) << "Undo - Set angle to " << ToDegrees(sourceAngle_) << " degrees";
        bitmap.SetAngle(sourceAngle_);
      }

      virtual void RedoInternal(BitmapStack::Bitmap& bitmap) const
      {
        LOG(INFO) << "Redo - Set angle to " << ToDegrees(sourceAngle_) << " degrees";
        bitmap.SetAngle(targetAngle_);
      }

    public:
      UndoRedoCommand(const RotateBitmapTracker& tracker) :
        BitmapCommandBase(tracker.accessor_),
        sourceAngle_(tracker.originalAngle_),
        targetAngle_(tracker.accessor_.GetBitmap().GetAngle())
      {
      }
    };

      
  public:
    RotateBitmapTracker(UndoRedoStack& undoRedoStack,
                        BitmapStack& stack,
                        const ViewportGeometry& view,
                        size_t bitmap,
                        double x,
                        double y,
                        bool roundAngles) :
      undoRedoStack_(undoRedoStack),
      accessor_(stack, bitmap),
      roundAngles_(roundAngles)
    {
      if (accessor_.IsValid())
      {
        accessor_.GetBitmap().GetCenter(centerX_, centerY_);
        originalAngle_ = accessor_.GetBitmap().GetAngle();

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
          angle = round(angle / ROUND_ANGLE) * ROUND_ANGLE;
        }
          
        accessor_.GetBitmap().SetAngle(angle);
      }
    }
  };
    
    
  class MoveBitmapTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&               undoRedoStack_;
    BitmapStack::BitmapAccessor  accessor_;
    double                       clickX_;
    double                       clickY_;
    double                       panX_;
    double                       panY_;
    bool                         oneAxis_;

    class UndoRedoCommand : public BitmapCommandBase
    {
    private:
      double  sourceX_;
      double  sourceY_;
      double  targetX_;
      double  targetY_;

    protected:
      virtual void UndoInternal(BitmapStack::Bitmap& bitmap) const
      {
        bitmap.SetPan(sourceX_, sourceY_);
      }

      virtual void RedoInternal(BitmapStack::Bitmap& bitmap) const
      {
        bitmap.SetPan(targetX_, targetY_);
      }

    public:
      UndoRedoCommand(const MoveBitmapTracker& tracker) :
        BitmapCommandBase(tracker.accessor_),
        sourceX_(tracker.panX_),
        sourceY_(tracker.panY_),
        targetX_(tracker.accessor_.GetBitmap().GetPanX()),
        targetY_(tracker.accessor_.GetBitmap().GetPanY())
      {
      }
    };


  public:
    MoveBitmapTracker(UndoRedoStack& undoRedoStack,
                      BitmapStack& stack,
                      size_t bitmap,
                      double x,
                      double y,
                      bool oneAxis) :
      undoRedoStack_(undoRedoStack),
      accessor_(stack, bitmap),
      clickX_(x),
      clickY_(y),
      oneAxis_(oneAxis)
    {
      if (accessor_.IsValid())
      {
        panX_ = accessor_.GetBitmap().GetPanX();
        panY_ = accessor_.GetBitmap().GetPanY();
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
            accessor_.GetBitmap().SetPan(dx + panX_, panY_);
          }
          else
          {
            accessor_.GetBitmap().SetPan(panX_, dy + panY_);
          }
        }
        else
        {
          accessor_.GetBitmap().SetPan(dx + panX_, dy + panY_);
        }
      }
    }
  };


  class CropBitmapTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&               undoRedoStack_;
    BitmapStack::BitmapAccessor  accessor_;
    BitmapStack::Corner          corner_;
    unsigned int                 cropX_;
    unsigned int                 cropY_;
    unsigned int                 cropWidth_;
    unsigned int                 cropHeight_;

    class UndoRedoCommand : public BitmapCommandBase
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
      virtual void UndoInternal(BitmapStack::Bitmap& bitmap) const
      {
        bitmap.SetCrop(sourceCropX_, sourceCropY_, sourceCropWidth_, sourceCropHeight_);
      }

      virtual void RedoInternal(BitmapStack::Bitmap& bitmap) const
      {
        bitmap.SetCrop(targetCropX_, targetCropY_, targetCropWidth_, targetCropHeight_);
      }

    public:
      UndoRedoCommand(const CropBitmapTracker& tracker) :
        BitmapCommandBase(tracker.accessor_),
        sourceCropX_(tracker.cropX_),
        sourceCropY_(tracker.cropY_),
        sourceCropWidth_(tracker.cropWidth_),
        sourceCropHeight_(tracker.cropHeight_)
      {
        tracker.accessor_.GetBitmap().GetCrop(targetCropX_, targetCropY_,
                                              targetCropWidth_, targetCropHeight_);
      }
    };


  public:
    CropBitmapTracker(UndoRedoStack& undoRedoStack,
                      BitmapStack& stack,
                      const ViewportGeometry& view,
                      size_t bitmap,
                      double x,
                      double y,
                      BitmapStack::Corner corner) :
      undoRedoStack_(undoRedoStack),
      accessor_(stack, bitmap),
      corner_(corner)
    {
      if (accessor_.IsValid())
      {
        accessor_.GetBitmap().GetCrop(cropX_, cropY_, cropWidth_, cropHeight_);          
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
        
        BitmapStack::Bitmap& bitmap = accessor_.GetBitmap();
        if (bitmap.GetPixel(x, y, sceneX, sceneY))
        {
          unsigned int targetX, targetWidth;

          if (corner_ == BitmapStack::Corner_TopLeft ||
              corner_ == BitmapStack::Corner_BottomLeft)
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

          if (corner_ == BitmapStack::Corner_TopLeft ||
              corner_ == BitmapStack::Corner_TopRight)
          {
            targetY = std::min(y, cropY_ + cropHeight_);
            targetHeight = cropY_ + cropHeight_ - targetY;
          }
          else
          {
            targetY = cropY_;
            targetHeight = std::max(y, cropY_) - cropY_;
          }

          bitmap.SetCrop(targetX, targetY, targetWidth, targetHeight);
        }
      }
    }
  };
    
    
  class ResizeBitmapTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&               undoRedoStack_;
    BitmapStack::BitmapAccessor  accessor_;
    bool                         roundScaling_;
    double                       originalSpacingX_;
    double                       originalSpacingY_;
    double                       originalPanX_;
    double                       originalPanY_;
    BitmapStack::Corner          oppositeCorner_;
    double                       oppositeX_;
    double                       oppositeY_;
    double                       baseScaling_;

    static double ComputeDistance(double x1,
                                  double y1,
                                  double x2,
                                  double y2)
    {
      double dx = x1 - x2;
      double dy = y1 - y2;
      return sqrt(dx * dx + dy * dy);
    }
      
    class UndoRedoCommand : public BitmapCommandBase
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
      virtual void UndoInternal(BitmapStack::Bitmap& bitmap) const
      {
        bitmap.SetPixelSpacing(sourceSpacingX_, sourceSpacingY_);
        bitmap.SetPan(sourcePanX_, sourcePanY_);
      }

      virtual void RedoInternal(BitmapStack::Bitmap& bitmap) const
      {
        bitmap.SetPixelSpacing(targetSpacingX_, targetSpacingY_);
        bitmap.SetPan(targetPanX_, targetPanY_);
      }

    public:
      UndoRedoCommand(const ResizeBitmapTracker& tracker) :
        BitmapCommandBase(tracker.accessor_),
        sourceSpacingX_(tracker.originalSpacingX_),
        sourceSpacingY_(tracker.originalSpacingY_),
        sourcePanX_(tracker.originalPanX_),
        sourcePanY_(tracker.originalPanY_),
        targetSpacingX_(tracker.accessor_.GetBitmap().GetPixelSpacingX()),
        targetSpacingY_(tracker.accessor_.GetBitmap().GetPixelSpacingY()),
        targetPanX_(tracker.accessor_.GetBitmap().GetPanX()),
        targetPanY_(tracker.accessor_.GetBitmap().GetPanY())
      {
      }
    };


  public:
    ResizeBitmapTracker(UndoRedoStack& undoRedoStack,
                        BitmapStack& stack,
                        size_t bitmap,
                        double x,
                        double y,
                        BitmapStack::Corner corner,
                        bool roundScaling) :
      undoRedoStack_(undoRedoStack),
      accessor_(stack, bitmap),
      roundScaling_(roundScaling)
    {
      if (accessor_.IsValid() &&
          accessor_.GetBitmap().IsResizeable())
      {
        originalSpacingX_ = accessor_.GetBitmap().GetPixelSpacingX();
        originalSpacingY_ = accessor_.GetBitmap().GetPixelSpacingY();
        originalPanX_ = accessor_.GetBitmap().GetPanX();
        originalPanY_ = accessor_.GetBitmap().GetPanY();

        switch (corner)
        {
          case BitmapStack::Corner_TopLeft:
            oppositeCorner_ = BitmapStack::Corner_BottomRight;
            break;

          case BitmapStack::Corner_TopRight:
            oppositeCorner_ = BitmapStack::Corner_BottomLeft;
            break;

          case BitmapStack::Corner_BottomLeft:
            oppositeCorner_ = BitmapStack::Corner_TopRight;
            break;

          case BitmapStack::Corner_BottomRight:
            oppositeCorner_ = BitmapStack::Corner_TopLeft;
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        accessor_.GetBitmap().GetCorner(oppositeX_, oppositeY_, oppositeCorner_);

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
          accessor_.GetBitmap().IsResizeable())
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
          accessor_.GetBitmap().IsResizeable())
      {
        double scaling = ComputeDistance(oppositeX_, oppositeY_, sceneX, sceneY) * baseScaling_;

        if (roundScaling_)
        {
          scaling = round(scaling / ROUND_SCALING) * ROUND_SCALING;
        }
          
        BitmapStack::Bitmap& bitmap = accessor_.GetBitmap();
        bitmap.SetPixelSpacing(scaling * originalSpacingX_,
                               scaling * originalSpacingY_);

        // Keep the opposite corner at a fixed location
        double ox, oy;
        bitmap.GetCorner(ox, oy, oppositeCorner_);
        bitmap.SetPan(bitmap.GetPanX() + oppositeX_ - ox,
                      bitmap.GetPanY() + oppositeY_ - oy);
      }
    }
  };


  class WindowingTracker : public IWorldSceneMouseTracker
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
    UndoRedoStack&  undoRedoStack_;
    BitmapStack&    stack_;
    int             clickX_;
    int             clickY_;
    Action          leftAction_;
    Action          rightAction_;
    Action          upAction_;
    Action          downAction_;
    float           strength_;
    float           sourceCenter_;
    float           sourceWidth_;

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
      BitmapStack&  stack_;
      float         sourceCenter_;
      float         sourceWidth_;
      float         targetCenter_;
      float         targetWidth_;

    public:
      UndoRedoCommand(const WindowingTracker& tracker) :
        stack_(tracker.stack_),
        sourceCenter_(tracker.sourceCenter_),
        sourceWidth_(tracker.sourceWidth_)
      {
        stack_.GetWindowingWithDefault(targetCenter_, targetWidth_);
      }

      virtual void Undo() const
      {
        stack_.SetWindowing(sourceCenter_, sourceWidth_);
      }
      
      virtual void Redo() const
      {
        stack_.SetWindowing(targetCenter_, targetWidth_);
      }
    };


  public:
    WindowingTracker(UndoRedoStack& undoRedoStack,
                     BitmapStack& stack,
                     int x,
                     int y,
                     Action leftAction,
                     Action rightAction,
                     Action upAction,
                     Action downAction) :
      undoRedoStack_(undoRedoStack),
      stack_(stack),
      clickX_(x),
      clickY_(y),
      leftAction_(leftAction),
      rightAction_(rightAction),
      upAction_(upAction),
      downAction_(downAction)
    {
      stack_.GetWindowingWithDefault(sourceCenter_, sourceWidth_);

      float minValue, maxValue;
      stack.GetRange(minValue, maxValue);

      assert(minValue <= maxValue);

      float tmp;
      
      float delta = (maxValue - minValue);
      if (delta <= 1)
      {
        tmp = 0;
      }
      else
      {
        tmp = log2(delta);
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
      stack_.SetWindowing(newCenter, newWidth);
    }
  };


  class BitmapStackWidget :
    public WorldSceneWidget,
    public IObservable,
    public IObserver
  {
  private:
    BitmapStack&                   stack_;
    std::auto_ptr<Orthanc::Image>  floatBuffer_;
    std::auto_ptr<CairoSurface>    cairoBuffer_;
    bool                           invert_;
    ImageInterpolation             interpolation_;

    virtual bool RenderInternal(unsigned int width,
                                unsigned int height,
                                ImageInterpolation interpolation)
    {
      float windowCenter, windowWidth;
      stack_.GetWindowingWithDefault(windowCenter, windowWidth);
      
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

        stack_.Render(*floatBuffer_, GetView(), interpolation);
        
        // Very similar to GrayscaleFrameRenderer => TODO MERGE?

        Orthanc::ImageAccessor target;
        cairoBuffer_->GetAccessor(target);
        
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
              v = static_cast<uint8_t>(255.0f * (*p - x0) / (x1 - x0));  // (*)
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
      return stack_.GetSceneExtent();
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

      stack_.DrawControls(context, view.GetZoom());

      return true;
    }

  public:
    BitmapStackWidget(MessageBroker& broker,
                      BitmapStack& stack,
                      const std::string& name) :
      WorldSceneWidget(name),
      IObservable(broker),
      IObserver(broker),
      stack_(stack),
      invert_(false),
      interpolation_(ImageInterpolation_Nearest)
    {
      stack.RegisterObserverCallback(new Callable<BitmapStackWidget, BitmapStack::GeometryChangedMessage>(*this, &BitmapStackWidget::OnGeometryChanged));
      stack.RegisterObserverCallback(new Callable<BitmapStackWidget, BitmapStack::ContentChangedMessage>(*this, &BitmapStackWidget::OnContentChanged));
    }

    BitmapStack& GetStack() const
    {
      return stack_;
    }

    void OnGeometryChanged(const BitmapStack::GeometryChangedMessage& message)
    {
      LOG(INFO) << "Geometry has changed";
      FitContent();
    }

    void OnContentChanged(const BitmapStack::ContentChangedMessage& message)
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

    bool IsInvert() const
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

  
  class BitmapStackInteractor : public IWorldSceneInteractor
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
    OrthancApiClient  *orthanc_;


    static double GetHandleSize()
    {
      return 10.0;
    }
    
      
    static BitmapStackWidget& GetWidget(WorldSceneWidget& widget)
    {
      return dynamic_cast<BitmapStackWidget&>(widget);
    }


    static BitmapStack& GetStack(WorldSceneWidget& widget)
    {
      return GetWidget(widget).GetStack();
    }
    
    
  public:
    BitmapStackInteractor() :
      tool_(Tool_Move),
      orthanc_(NULL)
    {
    }
    
    virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                        const ViewportGeometry& view,
                                                        MouseButton button,
                                                        KeyboardModifiers modifiers,
                                                        int viewportX,
                                                        int viewportY,
                                                        double x,
                                                        double y,
                                                        IStatusBar* statusBar)
    {
      if (button == MouseButton_Left)
      {
        size_t selected;

        if (tool_ == Tool_Windowing)
        {
          return new WindowingTracker(undoRedoStack_, GetStack(widget),
                                      viewportX, viewportY,
                                      WindowingTracker::Action_DecreaseWidth,
                                      WindowingTracker::Action_IncreaseWidth,
                                      WindowingTracker::Action_DecreaseCenter,
                                      WindowingTracker::Action_IncreaseCenter);
        }
        else if (!GetStack(widget).GetSelectedBitmap(selected))
        {
          size_t bitmap;
          if (GetStack(widget).LookupBitmap(bitmap, x, y))
          {
            LOG(INFO) << "Click on bitmap " << bitmap;
            GetStack(widget).Select(bitmap);
          }

          return NULL;
        }
        else if (tool_ == Tool_Crop ||
                 tool_ == Tool_Resize)
        {
          BitmapStack::BitmapAccessor accessor(GetStack(widget), selected);
          BitmapStack::Corner corner;
          if (accessor.GetBitmap().LookupCorner(corner, x, y, view.GetZoom(), GetHandleSize()))
          {
            switch (tool_)
            {
              case Tool_Crop:
                return new CropBitmapTracker(undoRedoStack_, GetStack(widget), view, selected, x, y, corner);

              case Tool_Resize:
                return new ResizeBitmapTracker(undoRedoStack_, GetStack(widget), selected, x, y, corner,
                                               (modifiers & KeyboardModifiers_Shift));

              default:
                throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
            }
          }
          else
          {
            size_t bitmap;
            
            if (!GetStack(widget).LookupBitmap(bitmap, x, y) ||
                bitmap != selected)
            {
              GetStack(widget).Unselect();
            }
            
            return NULL;
          }
        }
        else
        {
          size_t bitmap;

          if (GetStack(widget).LookupBitmap(bitmap, x, y) &&
              bitmap == selected)
          {
            switch (tool_)
            {
              case Tool_Move:
                return new MoveBitmapTracker(undoRedoStack_, GetStack(widget), bitmap, x, y,
                                             (modifiers & KeyboardModifiers_Shift));

              case Tool_Rotate:
                return new RotateBitmapTracker(undoRedoStack_, GetStack(widget), view, bitmap, x, y,
                                               (modifiers & KeyboardModifiers_Shift));
                
              default:
                break;
            }

            return NULL;
          }
          else
          {
            LOG(INFO) << "Click out of any bitmap";
            GetStack(widget).Unselect();
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
                           WorldSceneWidget& widget,
                           const ViewportGeometry& view,
                           double x,
                           double y,
                           IStatusBar* statusBar)
    {
#if 0
      if (statusBar != NULL)
      {
        char buf[64];
        sprintf(buf, "X = %.02f Y = %.02f (in cm)", x / 10.0, y / 10.0);
        statusBar->SetMessage(buf);
      }
#endif

      size_t selected;
      if (GetStack(widget).GetSelectedBitmap(selected) &&
          (tool_ == Tool_Crop ||
           tool_ == Tool_Resize))
      {
        BitmapStack::BitmapAccessor accessor(GetStack(widget), selected);
        
        BitmapStack::Corner corner;
        if (accessor.GetBitmap().LookupCorner(corner, x, y, view.GetZoom(), GetHandleSize()))
        {
          accessor.GetBitmap().GetCorner(x, y, corner);
          
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

    virtual void KeyPressed(WorldSceneWidget& widget,
                            KeyboardKeys key,
                            char keyChar,
                            KeyboardModifiers modifiers,
                            IStatusBar* statusBar)
    {
      switch (keyChar)
      {
        case 'a':
          widget.FitContent();
          break;

        case 'c':
          tool_ = Tool_Crop;
          break;

        case 'e':
          Export();
          break;

        case 'i':
          dynamic_cast<BitmapStackWidget&>(widget).SwitchInvert();
          break;
        
        case 'm':
          tool_ = Tool_Move;
          break;

        case 'n':
        {
          BitmapStackWidget& w = dynamic_cast<BitmapStackWidget&>(widget);

          switch (w.GetInterpolation())
          {
            case ImageInterpolation_Nearest:
              LOG(INFO) << "Switching to bilinear interpolation";
              w.SetInterpolation(ImageInterpolation_Bilinear);
              break;
              
            case ImageInterpolation_Bilinear:
              LOG(INFO) << "Switching to nearest neighbor interpolation";
              w.SetInterpolation(ImageInterpolation_Nearest);
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


    void SetOrthanc(OrthancApiClient& orthanc)
    {
      orthanc_ = &orthanc;
    }


    void Export()
    {
      if (orthanc_ == NULL)
      {
        return;
      }
      
      // TODO
      LOG(WARNING) << "Exporting DICOM";
    }
  };

  
  
  namespace Samples
  {
    class SingleFrameEditorApplication :
      public SampleSingleCanvasApplicationBase,
      public IObserver
    {
    private:
      std::auto_ptr<OrthancApiClient>  orthancApiClient_;
      std::auto_ptr<BitmapStack>       stack_;
      BitmapStackInteractor            interactor_;

    public:
      SingleFrameEditorApplication(MessageBroker& broker) :
        IObserver(broker)
      {
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
        statusBar.SetMessage("Use the key \"s\" to resize objects (not applicable to DICOM bitmaps)");
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

        orthancApiClient_.reset(new OrthancApiClient(IObserver::broker_, context_->GetWebService()));
        interactor_.SetOrthanc(*orthancApiClient_);

        Orthanc::FontRegistry fonts;
        fonts.AddFromResource(Orthanc::EmbeddedResources::FONT_UBUNTU_MONO_BOLD_16);
        
        stack_.reset(new BitmapStack(IObserver::broker_, *orthancApiClient_));
        stack_->LoadFrame(instance, frame, false).SetPan(200, 0);
        //stack_->LoadFrame("61f3143e-96f34791-ad6bbb8d-62559e75-45943e1b", 0, false);

        {
          BitmapStack::Bitmap& bitmap = stack_->LoadText(fonts.GetFont(0), "Hello\nworld");
          //dynamic_cast<BitmapStack::AlphaBitmap&>(bitmap).SetForegroundValue(256);
          dynamic_cast<BitmapStack::AlphaBitmap&>(bitmap).SetResizeable(true);
        }
        
        {
          BitmapStack::Bitmap& bitmap = stack_->LoadTestBlock(100, 50);
          //dynamic_cast<BitmapStack::AlphaBitmap&>(bitmap).SetForegroundValue(256);
          dynamic_cast<BitmapStack::AlphaBitmap&>(bitmap).SetResizeable(true);
          dynamic_cast<BitmapStack::AlphaBitmap&>(bitmap).SetPan(0, 200);
        }
        
        
        mainWidget_ = new BitmapStackWidget(IObserver::broker_, *stack_, "main-widget");
        mainWidget_->SetTransmitMouseOver(true);
        mainWidget_->SetInteractor(interactor_);

        //stack_->SetWindowing(128, 256);
      }
    };
  }
}
