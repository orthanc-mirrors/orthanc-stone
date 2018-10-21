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
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PamReader.h>
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

  private:
    class Bitmap : public boost::noncopyable
    {
    private:
      bool                                   visible_;
      std::auto_ptr<Orthanc::ImageAccessor>  source_;
      std::auto_ptr<Orthanc::ImageAccessor>  converted_;  // Float32 or RGB24
      std::auto_ptr<Orthanc::Image>          alpha_;  // Grayscale8 (if any)
      std::auto_ptr<DicomFrameConverter>     converter_;
      unsigned int                           width_;
      unsigned int                           height_;
      bool                                   hasCrop_;
      unsigned int                           cropX_;
      unsigned int                           cropY_;
      unsigned int                           cropWidth_;
      unsigned int                           cropHeight_;
      Matrix                                 transform_;

      void ApplyConverter()
      {
        if (source_.get() != NULL &&
            converter_.get() != NULL)
        {
          if (width_ != source_->GetWidth() ||
              height_ != source_->GetHeight())
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
          }
                      
          printf("CONVERTED! %dx%d\n", width_, height_);
          converted_.reset(converter_->ConvertFrame(*source_));
        }
      }
      
      void AddToExtent(Extent2D& extent,
                       double x,
                       double y) const
      {
        Vector p;
        LinearAlgebra::AssignVector(p, x, y, 1);

        Vector q = LinearAlgebra::Product(transform_, p);

        if (!LinearAlgebra::IsNear(q[2], 1.0))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
        else
        {
          extent.AddPoint(q[0], q[1]);
        }
      }
      

    public:
      Bitmap() :
        visible_(true),
        width_(0),
        height_(0),
        hasCrop_(false),
        transform_(LinearAlgebra::IdentityMatrix(3))
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

      bool IsVisible() const
      {
        return visible_;
      }

      void SetVisible(bool visible)
      {
        visible_ = visible;
      }


      static OrthancPlugins::DicomTag  ConvertTag(const Orthanc::DicomTag& tag)
      {
        return OrthancPlugins::DicomTag(tag.GetGroup(), tag.GetElement());
      }
      
      void SetDicomTags(const OrthancPlugins::FullOrthancDataset& dataset)
      {
        converter_.reset(new DicomFrameConverter);
        converter_->ReadParameters(dataset);
        ApplyConverter();

        transform_ = LinearAlgebra::IdentityMatrix(3);

        std::string tmp;
        Vector pixelSpacing;
        
        if (dataset.GetStringValue(tmp, ConvertTag(Orthanc::DICOM_TAG_PIXEL_SPACING)) &&
            LinearAlgebra::ParseVector(pixelSpacing, tmp) &&
            pixelSpacing.size() == 2)
        {
          transform_(0, 0) = pixelSpacing[0];
          transform_(1, 1) = pixelSpacing[1];
        }


#if 0
        double a = 10.0 / 180.0 * boost::math::constants::pi<double>();
        Matrix m;
        const double v[] = { cos(a), -sin(a), 0,
                             sin(a), cos(a), 0,
                             0, 0, 1 };
        LinearAlgebra::FillMatrix(m, 3, 3, v);
        transform_ = LinearAlgebra::Product(m, transform_);

#else
        static unsigned int c = 0;
        if (c == 0)
        {
          transform_(0, 2) = 400;
          c ++;
        }
#endif
        
        OrthancPlugins::DicomDatasetReader reader(dataset);

        if (!reader.GetUnsignedIntegerValue(width_, ConvertTag(Orthanc::DICOM_TAG_COLUMNS)) ||
            !reader.GetUnsignedIntegerValue(height_, ConvertTag(Orthanc::DICOM_TAG_ROWS)))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
      }

      
      void SetSourceImage(Orthanc::ImageAccessor* image)   // Takes ownership
      {
        source_.reset(image);
        ApplyConverter();
      }

      
      bool GetDefaultWindowing(float& center,
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


      void Render(Orthanc::ImageAccessor& buffer,
                  const ViewportGeometry& view) const
      {
        if (converted_.get() != NULL)
        {
          Matrix m = LinearAlgebra::Product(view.GetMatrix(), transform_);
          ApplyProjectiveTransform(buffer, *converted_, m, ImageInterpolation_Bilinear, false);
        }
      }


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
          return (q[0] >= 0 &&
                  q[1] >= 0 &&
                  q[0] <= static_cast<double>(width_) &&
                  q[1] <= static_cast<double>(height_));
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

  public:
    BitmapStack(MessageBroker& broker,
                OrthancApiClient& orthanc) :
      IObserver(broker),
      IObservable(broker),
      orthanc_(orthanc),
      countBitmaps_(0),
      hasWindowing_(false),
      windowingCenter_(0),  // Dummy initialization
      windowingWidth_(0)    // Dummy initialization
    {
    }

    
    virtual ~BitmapStack()
    {
      for (Bitmaps::iterator it = bitmaps_.begin(); it != bitmaps_.end(); it++)
      {
        assert(it->second != NULL);
        delete it->second;
      }
    }


    void GetWindowing(float& center,
                      float& width)
    {
      if (hasWindowing_)
      {
        center = windowingCenter_;
        width = windowingWidth_;
      }
      else
      {
        center = 128;
        width = 256;
      }
    }
    

    size_t LoadFrame(const std::string& instance,
                     unsigned int frame,
                     bool httpCompression)
    {
      size_t bitmap = countBitmaps_++;

      bitmaps_[bitmap] = new Bitmap;
      

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
      size_t index = dynamic_cast<Orthanc::SingleValueObject<size_t>*>(message.Payload)->GetValue();
      
      printf("JSON received: [%s] (%ld bytes) for bitmap %ld\n",
             message.Uri.c_str(), message.AnswerSize, index);

      Bitmaps::iterator bitmap = bitmaps_.find(index);
      if (bitmap != bitmaps_.end())
      {
        assert(bitmap->second != NULL);
        
        OrthancPlugins::FullOrthancDataset dicom(message.Answer, message.AnswerSize);
        bitmap->second->SetDicomTags(dicom);

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
      size_t index = dynamic_cast<Orthanc::SingleValueObject<size_t>*>(message.Payload)->GetValue();
      
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
        bitmap->second->SetSourceImage(reader.release());

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
                const ViewportGeometry& view)
    {
      Orthanc::ImageProcessing::Set(buffer, 0);

      for (Bitmaps::const_iterator it = bitmaps_.begin();
           it != bitmaps_.end(); ++it)
      {
        assert(it->second != NULL);
        it->second->Render(buffer, view);
      }
    }
  };


  class BitmapStackInteractor : public IWorldSceneInteractor
  {
  public:
    virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                        const ViewportGeometry& view,
                                                        MouseButton button,
                                                        KeyboardModifiers modifiers,
                                                        double x,
                                                        double y,
                                                        IStatusBar* statusBar)
    {
      printf("CLICK\n");
      return NULL;
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
      printf("Get extent\n");
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
      stack_(stack)
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
      Orthanc::Image buffer(Orthanc::PixelFormat_Float32, target.GetWidth(), target.GetHeight(), false);
      stack_.Render(buffer, GetView());

      // As in GrayscaleFrameRenderer => TODO MERGE

      float windowCenter, windowWidth;
      stack_.GetWindowing(windowCenter, windowWidth);
      
      float x0 = windowCenter - windowWidth / 2.0f;
      float x1 = windowCenter + windowWidth / 2.0f;

      const unsigned int width = target.GetWidth();
      const unsigned int height = target.GetHeight();
    
      for (unsigned int y = 0; y < height; y++)
      {
        const float* p = reinterpret_cast<const float*>(buffer.GetConstRow(y));
        uint8_t* q = reinterpret_cast<uint8_t*>(target.GetRow(y));

        for (unsigned int x = 0; x < width; x++, p++, q += 4)
        {
          uint8_t v = 0;
          if (windowWidth >= 0.001f)  // Avoid division by zero
          {
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
              v = static_cast<uint8_t>(255.0f * (*p - x0) / (x1 - x0));
            }

            // TODO MONOCHROME1
            /*if (invert_)
              {
              v = 255 - v;
              }*/
          }

          q[3] = 255;
          q[2] = v;
          q[1] = v;
          q[0] = v;
        }
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

        stack_.reset(new BitmapStack(IObserver::broker_, *orthancApiClient_));
        stack_->LoadFrame(instance, frame, false);
        stack_->LoadFrame("61f3143e-96f34791-ad6bbb8d-62559e75-45943e1b", frame, false);
        
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
