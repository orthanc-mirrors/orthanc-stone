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

#include "../../Framework/Toolbox/GeometryToolbox.h"
#include "../../Framework/Toolbox/ImageGeometry.h"
#include "../../Framework/Layers/OrthancFrameLayerSource.h"

#include <Core/DicomFormat/DicomArray.h>
#include <Core/Images/FontRegistry.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PamReader.h>
#include <Core/Images/PngWriter.h>   //TODO 
#include <Core/Logging.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>
#include <Plugins/Samples/Common/DicomDatasetReader.h>


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

    class Bitmap : public boost::noncopyable
    {
    private:
      bool          visible_;
      bool          hasSize_;
      unsigned int  width_;
      unsigned int  height_;
      bool          hasCrop_;
      unsigned int  cropX_;
      unsigned int  cropY_;
      unsigned int  cropWidth_;
      unsigned int  cropHeight_;
      Matrix        transform_;
      double        pixelSpacingX_;
      double        pixelSpacingY_;
      double        panX_;
      double        panY_;
      double        angle_;


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
      }


      void AddToExtent(Extent2D& extent,
                       double x,
                       double y) const
      {
        ApplyTransform(x, y, transform_);
        extent.AddPoint(x, y);
      }
      

    public:
      Bitmap() :
        visible_(true),
        hasSize_(false),
        width_(0),
        height_(0),
        hasCrop_(false),
        pixelSpacingX_(1),
        pixelSpacingY_(1),
        panX_(0),
        panY_(0),
        angle_(0)
      {
        UpdateTransform();
      }

      virtual ~Bitmap()
      {
      }

      void ResetCrop()
      {
        hasCrop_ = false;
      }

      void Crop(unsigned int x,
                unsigned int y,
                unsigned int width,
                unsigned int height)
      {
        hasCrop_ = true;
        cropX_ = x;
        cropY_ = y;
        cropWidth_ = width;
        cropHeight_ = height;
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

      bool IsVisible() const
      {
        return visible_;
      }

      void SetVisible(bool visible)
      {
        visible_ = visible;
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

        double dx = static_cast<double>(x) - 0.5;
        double dy = static_cast<double>(y) - 0.5;
        double dwidth = static_cast<double>(width);
        double dheight = static_cast<double>(height);

        AddToExtent(extent, dx, dy);
        AddToExtent(extent, dx + dwidth, dy);
        AddToExtent(extent, dx, dy + dheight);
        AddToExtent(extent, dx + dwidth, dy + dheight);
        
        return extent;
      }


      virtual void Render(Orthanc::ImageAccessor& buffer,
                          const ViewportGeometry& view,
                          ImageInterpolation interpolation) const = 0;


      bool Contains(double x,
                    double y) const
      {
        Matrix inv;
        LinearAlgebra::InvertMatrix(inv, transform_);

        Vector p;
        LinearAlgebra::AssignVector(p, x, y, 1);

        Vector q = LinearAlgebra::Product(inv, p);

        if (!LinearAlgebra::IsNear(q[2], 1.0))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
        else
        {
          printf("at: (%.02f, %.02f)\n", q[0], q[1]);
          return (q[0] >= 0 &&
                  q[1] >= 0 &&
                  q[0] <= static_cast<double>(width_) &&
                  q[1] <= static_cast<double>(height_));
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

      virtual bool GetDefaultWindowing(float& center,
                                       float& width) const
      {
        return false;
      }

      const Matrix& GetTransform() const
      {
        return transform_;
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
    }; 


    class BitmapAccessor : public boost::noncopyable
    {
    private:
      size_t    index_;
      Bitmap*   bitmap_;

    public:
      BitmapAccessor(BitmapStack& stack,
                     size_t index) :
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


  private:
    class DicomBitmap : public Bitmap
    {
    private:
      std::auto_ptr<Orthanc::ImageAccessor>  source_;  // Content of PixelData
      std::auto_ptr<DicomFrameConverter>     converter_;
      std::auto_ptr<Orthanc::ImageAccessor>  converted_;  // Float32 or RGB24


      void ApplyConverter()
      {
        if (source_.get() != NULL &&
            converter_.get() != NULL)
        {
          converted_.reset(converter_->ConvertFrame(*source_));
        }
      }
      
    public:
      static OrthancPlugins::DicomTag  ConvertTag(const Orthanc::DicomTag& tag)
      {
        return OrthancPlugins::DicomTag(tag.GetGroup(), tag.GetElement());
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

        SetPan(-0.5 * GetPixelSpacingX(), -0.5 * GetPixelSpacingY());
      
        static unsigned int c = 0;
        if (c == 0)
        {
          SetPan(400, 0);
          c ++;
        }
        
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
          Matrix m = LinearAlgebra::Product(view.GetMatrix(), GetTransform());
          ApplyProjectiveTransform(buffer, *converted_, m, interpolation, false);
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
    };




    class AlphaBitmap : public Bitmap
    {
    private:
      const BitmapStack&                     stack_;
      std::auto_ptr<Orthanc::ImageAccessor>  alpha_;      // Grayscale8
      bool                                   useWindowing_;
      float                                  foreground_;

    public:
      AlphaBitmap(const BitmapStack& stack) :
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

        Matrix m = LinearAlgebra::Product(view.GetMatrix(), GetTransform());
        
        Orthanc::Image tmp(Orthanc::PixelFormat_Grayscale8, buffer.GetWidth(), buffer.GetHeight(), false);
        ApplyProjectiveTransform(tmp, *alpha_, m, interpolation, true /* clear */);

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
    

    size_t LoadText(const Orthanc::Font& font,
                    const std::string& utf8,
                    double x,
                    double y)
    {
      std::auto_ptr<AlphaBitmap>  alpha(new AlphaBitmap(*this));
      alpha->LoadText(font, utf8);

      size_t bitmap = countBitmaps_++;

      bitmaps_[bitmap] = alpha.release();
      
      return bitmap;
    }

    
    size_t LoadFrame(const std::string& instance,
                     unsigned int frame,
                     bool httpCompression)
    {
      size_t bitmap = countBitmaps_++;

      bitmaps_[bitmap] = new DicomBitmap;
      

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

      return bitmap;
    }

    
    void OnTagsReceived(const OrthancApiClient::BinaryResponseReadyMessage& message)
    {
      size_t index = dynamic_cast<Orthanc::SingleValueObject<size_t>*>(message.Payload.get())->GetValue();
      
      printf("JSON received: [%s] (%ld bytes) for bitmap %ld\n",
             message.Uri.c_str(), message.AnswerSize, index);

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
      
      printf("Frame received: [%s] (%ld bytes) for bitmap %ld\n",
             message.Uri.c_str(), message.AnswerSize, index);
      
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

    void DrawControls(CairoSurface& surface,
                      const ViewportGeometry& view)
    {
      if (hasSelection_)
      {
        Bitmaps::const_iterator bitmap = bitmaps_.find(selectedBitmap_);
        
        if (bitmap != bitmaps_.end())
        {
          CairoContext context(surface);
      
          context.SetSourceColor(255, 0, 0);
          view.ApplyTransform(context);
          bitmap->second->DrawBorders(context, view.GetZoom());
        }
      }
    }
  };


  class BitmapStackInteractor : public IWorldSceneInteractor
  {
  private:
    enum Tool
    {
      Tool_Move,
      Tool_Rotate
    };
        

    BitmapStack&  stack_;
    Tool          tool_;
    

    class RotateBitmapTracker : public IWorldSceneMouseTracker
    {
    private:
      BitmapStack::BitmapAccessor  accessor_;
      double                       centerX_;
      double                       centerY_;
      double                       originalAngle_;
      double                       clickAngle_;
      bool                         roundAngles_;

      bool ComputeAngle(double& angle /* out */,
                        double sceneX,
                        double sceneY)
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

      
    public:
      RotateBitmapTracker(BitmapStack& stack,
                          const ViewportGeometry& view,
                          size_t bitmap,
                          double x,
                          double y,
                          bool roundAngles) :
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
      BitmapStack::BitmapAccessor  accessor_;
      double                       clickX_;
      double                       clickY_;
      double                       panX_;
      double                       panY_;
      bool                         oneAxis_;

    public:
      MoveBitmapTracker(BitmapStack& stack,
                        size_t bitmap,
                        double x,
                        double y,
                        bool oneAxis) :
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
    
    
  public:
    BitmapStackInteractor(BitmapStack& stack) :
      stack_(stack),
      tool_(Tool_Move)
    {
    }
    
    virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                        const ViewportGeometry& view,
                                                        MouseButton button,
                                                        KeyboardModifiers modifiers,
                                                        double x,
                                                        double y,
                                                        IStatusBar* statusBar)
    {
      if (button == MouseButton_Left)
      {
        size_t bitmap;

        if (stack_.LookupBitmap(bitmap, x, y))
        {
          printf("CLICK on bitmap %ld\n", bitmap);

          size_t selected;
          if (stack_.GetSelectedBitmap(selected) &&
              bitmap == selected)
          {
            switch (tool_)
            {
              case Tool_Move:
                return new MoveBitmapTracker(stack_, bitmap, x, y,
                                             (modifiers & KeyboardModifiers_Shift));

              case Tool_Rotate:
                return new RotateBitmapTracker(stack_, view, bitmap, x, y,
                                               (modifiers & KeyboardModifiers_Shift));

              default:
                return NULL;
            }
          }
          else
          {
            stack_.Select(bitmap);
            return NULL;
          }
        }
        else
        {
          printf("CLICK outside\n");
          stack_.Unselect();
          return NULL;
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
        case 's':
          widget.FitContent();
          break;

        case 'm':
          tool_ = Tool_Move;
          break;

        case 'r':
          tool_ = Tool_Rotate;
          break;

        default:
          break;
      }
    }
  };

  
  
  class BitmapStackWidget :
    public WorldSceneWidget,
    public IObservable,
    public IObserver
  {
  private:
    BitmapStack&           stack_;
    BitmapStackInteractor  myInteractor_;

  protected:
    virtual Extent2D GetSceneExtent()
    {
      return stack_.GetSceneExtent();
    }

    virtual bool RenderScene(CairoContext& context,
                             const ViewportGeometry& view)
    {
      // "Render()" has been replaced
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

  public:
    BitmapStackWidget(MessageBroker& broker,
                      BitmapStack& stack,
                      const std::string& name) :
      WorldSceneWidget(name),
      IObservable(broker),
      IObserver(broker),
      stack_(stack),
      myInteractor_(stack_)
    {
      stack.RegisterObserverCallback(new Callable<BitmapStackWidget, BitmapStack::GeometryChangedMessage>(*this, &BitmapStackWidget::OnGeometryChanged));
      stack.RegisterObserverCallback(new Callable<BitmapStackWidget, BitmapStack::ContentChangedMessage>(*this, &BitmapStackWidget::OnContentChanged));

      SetInteractor(myInteractor_);
    }

    void OnGeometryChanged(const BitmapStack::GeometryChangedMessage& message)
    {
      printf("Geometry has changed\n");
      FitContent();
    }

    void OnContentChanged(const BitmapStack::ContentChangedMessage& message)
    {
      printf("Content has changed\n");
      NotifyContentChanged();
    }

    virtual bool Render(Orthanc::ImageAccessor& target)
    {
      Orthanc::Image buffer(Orthanc::PixelFormat_Float32, target.GetWidth(),
                            target.GetHeight(), false);

      // TODO => rendering quality
      //stack_.Render(buffer, GetView(), ImageInterpolation_Nearest);
      stack_.Render(buffer, GetView(), ImageInterpolation_Bilinear);

      // As in GrayscaleFrameRenderer => TODO MERGE?

      float windowCenter, windowWidth;
      if (!stack_.GetWindowing(windowCenter, windowWidth))
      {
        windowCenter = 128;
        windowWidth = 256;
      }
      
      float x0 = windowCenter - windowWidth / 2.0f;
      float x1 = windowCenter + windowWidth / 2.0f;

      if (windowWidth >= 0.001f)  // Avoid division by zero at (*)
      {
        const unsigned int width = target.GetWidth();
        const unsigned int height = target.GetHeight();
    
        for (unsigned int y = 0; y < height; y++)
        {
          const float* p = reinterpret_cast<const float*>(buffer.GetConstRow(y));
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

            // TODO MONOCHROME1
            /*if (invert_)
              {
              v = 255 - v;
              }*/

            q[0] = v;
            q[1] = v;
            q[2] = v;
            q[3] = 255;
          }
        }
      }
      else
      {
        Orthanc::ImageProcessing::Set(target, 0, 0, 0, 255);
      }

      {
        // TODO => REFACTOR
        CairoSurface surface(target);
        stack_.DrawControls(surface, GetView());
      }

      return true;
    }

  };

  
  namespace Samples
  {
    class SingleFrameEditorApplication :
      public SampleSingleCanvasApplicationBase,
      public IObserver
    {
      enum Tools
      {
        Tools_Crop,
        Tools_Windowing,
        Tools_Zoom,
        Tools_Pan
      };

      enum Actions
      {
        Actions_Invert,
        Actions_RotateLeft,
        Actions_RotateRight
      };

    private:
      class Interactor : public IWorldSceneInteractor
      {
      private:
        SingleFrameEditorApplication&  application_;
        
      public:
        Interactor(SingleFrameEditorApplication&  application) :
          application_(application)
        {
        }
        
        virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                            const ViewportGeometry& view,
                                                            MouseButton button,
                                                            KeyboardModifiers modifiers,
                                                            double x,
                                                            double y,
                                                            IStatusBar* statusBar)
        {
          switch (application_.currentTool_) {
            case Tools_Zoom:
              printf("ZOOM\n");

            case Tools_Crop:
            case Tools_Windowing:
            case Tools_Pan:
              // TODO return the right mouse tracker
              return NULL;
          }

          return NULL;
        }

        virtual void MouseOver(CairoContext& context,
                               WorldSceneWidget& widget,
                               const ViewportGeometry& view,
                               double x,
                               double y,
                               IStatusBar* statusBar)
        {
          if (statusBar != NULL)
          {
            char buf[64];
            sprintf(buf, "X = %.02f Y = %.02f (in cm)", x / 10.0, y / 10.0);
            statusBar->SetMessage(buf);
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
            case 's':
              widget.FitContent();
              break;
            case 'p':
              application_.currentTool_ = Tools_Pan;
              break;
            case 'z':
              application_.currentTool_ = Tools_Zoom;
              break;
            case 'c':
              application_.currentTool_ = Tools_Crop;
              break;
            case 'w':
              application_.currentTool_ = Tools_Windowing;
              break;
            case 'i':
              application_.Invert();
              break;
            case 'r':
              if (modifiers == KeyboardModifiers_None)
                application_.Rotate(90);
              else
                application_.Rotate(-90);
              break;
            case 'e':
              application_.Export();
              break;
            default:
              break;
          }
        }
      };

      std::auto_ptr<Interactor>        mainWidgetInteractor_;
      std::auto_ptr<OrthancApiClient>  orthancApiClient_;
      std::auto_ptr<BitmapStack>       stack_;
      Tools                            currentTool_;
      const OrthancFrameLayerSource*   source_;
      unsigned int                     slice_;

    public:
      SingleFrameEditorApplication(MessageBroker& broker) :
        IObserver(broker),
        currentTool_(Tools_Zoom),
        source_(NULL),
        slice_(0)
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

        statusBar.SetMessage("Use the key \"s\" to reinitialize the layout, \"p\" to pan, \"z\" to zoom, \"c\" to crop, \"i\" to invert, \"w\" to change windowing, \"r\" to rotate cw,  \"shift+r\" to rotate ccw");

        if (parameters.count("instance") != 1)
        {
          LOG(ERROR) << "The instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string instance = parameters["instance"].as<std::string>();
        int frame = parameters["frame"].as<unsigned int>();

        orthancApiClient_.reset(new OrthancApiClient(IObserver::broker_, context_->GetWebService()));

        Orthanc::FontRegistry fonts;
        fonts.AddFromResource(Orthanc::EmbeddedResources::FONT_UBUNTU_MONO_BOLD_16);
        
        stack_.reset(new BitmapStack(IObserver::broker_, *orthancApiClient_));
        stack_->LoadFrame(instance, frame, false);
        stack_->LoadFrame("61f3143e-96f34791-ad6bbb8d-62559e75-45943e1b", frame, false);
        stack_->LoadText(fonts.GetFont(0), "Hello\nworld\nBonjour, Alain", 0, -50);
        
        mainWidget_ = new BitmapStackWidget(IObserver::broker_, *stack_, "main-widget");
        mainWidget_->SetTransmitMouseOver(true);

        mainWidgetInteractor_.reset(new Interactor(*this));
        //mainWidget_->SetInteractor(*mainWidgetInteractor_);
      }


      void Invert()
      {
        // TODO
      }

      void Rotate(int degrees)
      {
        // TODO
      }

      void Export()
      {
        // TODO: export dicom file to a temporary file
      }
    };


  }
}
