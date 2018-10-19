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

#include "../../Framework/Layers/OrthancFrameLayerSource.h"

#include <Core/DicomFormat/DicomArray.h>
#include <Core/Images/PamReader.h>
#include <Core/Logging.h>
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>

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
      std::string                            uuid_;   // TODO is this necessary?
      std::auto_ptr<Orthanc::ImageAccessor>  source_;
      std::auto_ptr<Orthanc::ImageAccessor>  converted_;  // Float32 or RGB24
      std::auto_ptr<Orthanc::Image>          alpha_;  // Grayscale8 (if any)
      std::auto_ptr<DicomFrameConverter>     converter_;

      void ApplyConverter()
      {
        if (source_.get() != NULL &&
            converter_.get() != NULL)
        {
          printf("CONVERTED!\n");
          converted_.reset(converter_->ConvertFrame(*source_));
        }
      }
      
    public:
      Bitmap(const std::string& uuid) :
        uuid_(uuid)
      {
      }
      
      void SetDicomTags(const OrthancPlugins::FullOrthancDataset& dataset)
      {
        converter_.reset(new DicomFrameConverter);
        converter_->ReadParameters(dataset);
        ApplyConverter();
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
        }
      }
    }; 


    typedef std::map<std::string, Bitmap*>  Bitmaps;
        
    OrthancApiClient&    orthanc_;
    bool                 hasWindowing_;
    float                windowingCenter_;
    float                windowingWidth_;
    Bitmaps              bitmaps_;

  public:
    BitmapStack(MessageBroker& broker,
                OrthancApiClient& orthanc) :
      IObserver(broker),
      IObservable(broker),
      orthanc_(orthanc),
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
    

    std::string LoadFrame(const std::string& instance,
                          unsigned int frame,
                          bool httpCompression)
    {
      std::string uuid;
      
      for (;;)
      {
        uuid = Orthanc::Toolbox::GenerateUuid();
        if (bitmaps_.find(uuid) == bitmaps_.end())
        {
          break;
        }
      }

      bitmaps_[uuid] = new Bitmap(uuid);
      

      {
        IWebService::Headers headers;
        std::string uri = "/instances/" + instance + "/tags";
        orthanc_.GetBinaryAsync(uri, headers,
                                new Callable<BitmapStack, OrthancApiClient::BinaryResponseReadyMessage>
                                (*this, &BitmapStack::OnTagsReceived), NULL,
                                new Orthanc::SingleValueObject<std::string>(uuid));
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
                                new Orthanc::SingleValueObject<std::string>(uuid));
      }

      return uuid;
    }

    
    void OnTagsReceived(const OrthancApiClient::BinaryResponseReadyMessage& message)
    {
      const std::string& uuid = dynamic_cast<Orthanc::SingleValueObject<std::string>*>(message.Payload)->GetValue();
      
      printf("JSON received: [%s] (%d bytes) for bitmap %s\n",
             message.Uri.c_str(), message.AnswerSize, uuid.c_str());

      Bitmaps::iterator bitmap = bitmaps_.find(uuid);
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
      const std::string& uuid = dynamic_cast<Orthanc::SingleValueObject<std::string>*>(message.Payload)->GetValue();

      printf("Frame received: [%s] (%d bytes) for bitmap %s\n", message.Uri.c_str(), message.AnswerSize, uuid.c_str());
      
      Bitmaps::iterator bitmap = bitmaps_.find(uuid);
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
  };

  
  class BitmapStackWidget :
    public WorldSceneWidget,
    public IObservable,
    public IObserver
  {
  private:
    BitmapStack&   stack_;

  protected:
    virtual Extent2D GetSceneExtent()
    {
      return Extent2D(-1, -1, 1, 1);
    }

    virtual bool RenderScene(CairoContext& context,
                             const ViewportGeometry& view)
    {
      return true;
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
        stack_->LoadFrame(instance, frame, false);
        
        mainWidget_ = new BitmapStackWidget(IObserver::broker_, *stack_, "main-widget");
        mainWidget_->SetTransmitMouseOver(true);

        mainWidgetInteractor_.reset(new Interactor(*this));
        mainWidget_->SetInteractor(*mainWidgetInteractor_);
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
