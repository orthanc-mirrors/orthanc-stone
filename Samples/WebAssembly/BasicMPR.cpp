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



#include <emscripten.h>
#include <emscripten/fetch.h>
#include <emscripten/html5.h>

#include "../../Framework/Loaders/OrthancSeriesVolumeProgressiveLoader.h"
#include "../../Framework/OpenGL/WebAssemblyOpenGLContext.h"
#include "../../Framework/Scene2D/GrayscaleStyleConfigurator.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/RotateSceneTracker.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"
#include "../../Framework/Scene2DViewport/IFlexiblePointerTracker.h"
#include "../../Framework/Scene2DViewport/ViewportController.h"
#include "../../Framework/StoneInitialization.h"
#include "../../Framework/Volumes/VolumeSceneLayerSource.h"

#include <Core/OrthancException.h>
#include <Core/Toolbox.h>


static const unsigned int FONT_SIZE = 32;


namespace OrthancStone
{
  class WebAssemblyViewport : public boost::noncopyable
  {
  private:
    // the construction order is important because compositor_
    // will hold a reference to the scene that belong to the 
    // controller_ object
    OpenGL::WebAssemblyOpenGLContext       context_;
    boost::shared_ptr<ViewportController>  controller_;
    OpenGLCompositor                       compositor_;

    void SetupEvents(const std::string& canvas);

  public:
    WebAssemblyViewport(MessageBroker& broker,
                        const std::string& canvas) :
      context_(canvas),
      controller_(new ViewportController(broker)),
      compositor_(context_, *controller_->GetScene())
    {
      compositor_.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, 
                          FONT_SIZE, Orthanc::Encoding_Latin1);
      SetupEvents(canvas);
    }

    Scene2D& GetScene()
    {
      return *controller_->GetScene();
    }

    const boost::shared_ptr<ViewportController>& GetController()
    {
      return controller_;
    }

    void UpdateSize()
    {
      context_.UpdateSize();
      compositor_.UpdateSize();
      Refresh();
    }

    void Refresh()
    {
      compositor_.Refresh();
    }

    const std::string& GetCanvasIdentifier() const
    {
      return context_.GetCanvasIdentifier();
    }

    ScenePoint2D GetPixelCenterCoordinates(int x, int y) const
    {
      return compositor_.GetPixelCenterCoordinates(x, y);
    }

    unsigned int GetCanvasWidth() const
    {
      return context_.GetCanvasWidth();
    }

    unsigned int GetCanvasHeight() const
    {
      return context_.GetCanvasHeight();
    }
  };

  class ActiveTracker : public boost::noncopyable
  {
  private:
    boost::shared_ptr<IFlexiblePointerTracker> tracker_;
    std::string                             canvasIdentifier_;
    bool                                    insideCanvas_;
    
  public:
    ActiveTracker(const boost::shared_ptr<IFlexiblePointerTracker>& tracker,
                  const WebAssemblyViewport& viewport) :
      tracker_(tracker),
      canvasIdentifier_(viewport.GetCanvasIdentifier()),
      insideCanvas_(true)
    {
      if (tracker_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    bool IsAlive() const
    {
      return tracker_->IsAlive();
    }

    void PointerMove(const PointerEvent& event)
    {
      tracker_->PointerMove(event);
    }

    void PointerUp(const PointerEvent& event)
    {
      tracker_->PointerUp(event);
    }
  };
}

static OrthancStone::PointerEvent* ConvertMouseEvent(
  const EmscriptenMouseEvent&        source,
  OrthancStone::WebAssemblyViewport& viewport)
{
  std::auto_ptr<OrthancStone::PointerEvent> target(
    new OrthancStone::PointerEvent);

  target->AddPosition(viewport.GetPixelCenterCoordinates(
    source.targetX, source.targetY));
  target->SetAltModifier(source.altKey);
  target->SetControlModifier(source.ctrlKey);
  target->SetShiftModifier(source.shiftKey);

  return target.release();
}

std::auto_ptr<OrthancStone::ActiveTracker> tracker_;

EM_BOOL OnMouseEvent(int eventType, 
                     const EmscriptenMouseEvent *mouseEvent, 
                     void *userData)
{
  if (mouseEvent != NULL &&
      userData != NULL)
  {
    OrthancStone::WebAssemblyViewport& viewport = 
      *reinterpret_cast<OrthancStone::WebAssemblyViewport*>(userData);

    switch (eventType)
    {
      case EMSCRIPTEN_EVENT_CLICK:
      {
        static unsigned int count = 0;
        char buf[64];
        sprintf(buf, "click %d", count++);

        std::auto_ptr<OrthancStone::TextSceneLayer> layer(new OrthancStone::TextSceneLayer);
        layer->SetText(buf);
        viewport.GetScene().SetLayer(100, layer.release());
        viewport.Refresh();
        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEDOWN:
      {
        boost::shared_ptr<OrthancStone::IFlexiblePointerTracker> t;

        {
          std::auto_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, viewport));

          switch (mouseEvent->button)
          {
            case 0:  // Left button
              emscripten_console_log("Creating RotateSceneTracker");
              t.reset(new OrthancStone::RotateSceneTracker(
                viewport.GetController(), *event));
              break;

            case 1:  // Middle button
              emscripten_console_log("Creating PanSceneTracker");
              LOG(INFO) << "Creating PanSceneTracker" ;
              t.reset(new OrthancStone::PanSceneTracker(
                viewport.GetController(), *event));
              break;

            case 2:  // Right button
              emscripten_console_log("Creating ZoomSceneTracker");
              t.reset(new OrthancStone::ZoomSceneTracker(
                viewport.GetController(), *event, viewport.GetCanvasWidth()));
              break;

            default:
              break;
          }
        }

        if (t.get() != NULL)
        {
          tracker_.reset(
            new OrthancStone::ActiveTracker(t, viewport));
          viewport.Refresh();
        }

        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEMOVE:
        if (tracker_.get() != NULL)
        {
          std::auto_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, viewport));
          tracker_->PointerMove(*event);
          viewport.Refresh();
        }
        break;

      case EMSCRIPTEN_EVENT_MOUSEUP:
        if (tracker_.get() != NULL)
        {
          std::auto_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, viewport));
          tracker_->PointerUp(*event);
          viewport.Refresh();
          if (!tracker_->IsAlive())
            tracker_.reset();
        }
        break;

      default:
        break;
    }
  }

  return true;
}


void OrthancStone::WebAssemblyViewport::SetupEvents(const std::string& canvas)
{
  emscripten_set_mousedown_callback(canvas.c_str(), this, false, OnMouseEvent);
  emscripten_set_mousemove_callback(canvas.c_str(), this, false, OnMouseEvent);
  emscripten_set_mouseup_callback(canvas.c_str(), this, false, OnMouseEvent);
}




namespace OrthancStone
{
  class VolumeSlicerViewport : public IObserver
  {
  private:
    OrthancStone::WebAssemblyViewport      viewport_;
    std::auto_ptr<VolumeSceneLayerSource>  source_;
    VolumeProjection                       projection_;
    std::vector<CoordinateSystem3D>        planes_;
    size_t                                 currentPlane_;

    void Handle(const DicomVolumeImage::GeometryReadyMessage& message)
    {
      LOG(INFO) << "Geometry is available";

      const VolumeImageGeometry& geometry = message.GetOrigin().GetGeometry();

      const unsigned int depth = geometry.GetProjectionDepth(projection_);
      currentPlane_ = depth / 2;
      
      planes_.resize(depth);

      for (unsigned int z = 0; z < depth; z++)
      {
        planes_[z] = geometry.GetProjectionSlice(projection_, z);
      }
    }
    
  public:
    VolumeSlicerViewport(MessageBroker& broker,
                         const std::string& canvas,
                         VolumeProjection projection) :
      IObserver(broker),
      viewport_(broker, canvas),
      projection_(projection),
      currentPlane_(0)
    {
    }

    void UpdateSize()
    {
      viewport_.UpdateSize();
    }

    void SetSlicer(int layerDepth,
                   const boost::shared_ptr<IVolumeSlicer>& slicer,
                   IObservable& loader,
                   ILayerStyleConfigurator* configurator)
    {
      if (source_.get() != NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls,
                                        "Only one slicer can be registered");
      }
      
      loader.RegisterObserverCallback(
        new Callable<VolumeSlicerViewport, DicomVolumeImage::GeometryReadyMessage>
        (*this, &VolumeSlicerViewport::Handle));

      source_.reset(new VolumeSceneLayerSource(viewport_.GetScene(), layerDepth, slicer));

      if (configurator != NULL)
      {
        source_->SetConfigurator(configurator);
      }
    }    

    void Refresh()
    {
      if (source_.get() != NULL &&
          currentPlane_ < planes_.size())
      {
        source_->Update(planes_[currentPlane_]);
      }
    }
  };




  class WebAssemblyOracle :
    public IOracle,
    public IObservable
  {
  private:
    typedef std::map<std::string, std::string>  HttpHeaders;
    
    class FetchContext : public boost::noncopyable
    {
    private:
      class Emitter : public IMessageEmitter
      {
      private:
        WebAssemblyOracle&  oracle_;

      public:
        Emitter(WebAssemblyOracle&  oracle) :
          oracle_(oracle)
        {
        }

        virtual void EmitMessage(const IObserver& receiver,
                                 const IMessage& message)
        {
          oracle_.EmitMessage(receiver, message);
        }
      };

      Emitter                        emitter_;
      const IObserver&               receiver_;
      std::auto_ptr<IOracleCommand>  command_;
      std::string                    expectedContentType_;

    public:
      FetchContext(WebAssemblyOracle& oracle,
                   const IObserver& receiver,
                   IOracleCommand* command,
                   const std::string& expectedContentType) :
        emitter_(oracle),
        receiver_(receiver),
        command_(command),
        expectedContentType_(expectedContentType)
      {
        if (command == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
      }

      const std::string& GetExpectedContentType() const
      {
        return expectedContentType_;
      }

      void EmitMessage(const IMessage& message)
      {
        emitter_.EmitMessage(receiver_, message);
      }

      IMessageEmitter& GetEmitter()
      {
        return emitter_;
      }

      const IObserver& GetReceiver() const
      {
        return receiver_;
      }

      IOracleCommand& GetCommand() const
      {
        return *command_;
      }

      template <typename T>
      const T& GetTypedCommand() const
      {
        return dynamic_cast<T&>(*command_);
      }
    };
    
    static void FetchSucceeded(emscripten_fetch_t *fetch)
    {
      /**
       * Firstly, make a local copy of the fetched information, and
       * free data associated with the fetch.
       **/
      
      std::auto_ptr<FetchContext> context(reinterpret_cast<FetchContext*>(fetch->userData));

      std::string answer;
      if (fetch->numBytes > 0)
      {
        answer.assign(fetch->data, fetch->numBytes);
      }

      /**
       * TODO - HACK - As of emscripten-1.38.31, the fetch API does
       * not contain a way to retrieve the HTTP headers of the
       * answer. We make the assumption that the "Content-Type" header
       * of the response is the same as the "Accept" header of the
       * query. This should be fixed in future versions of emscripten.
       * https://github.com/emscripten-core/emscripten/pull/8486
       **/

      HttpHeaders headers;
      if (!context->GetExpectedContentType().empty())
      {
        headers["Content-Type"] = context->GetExpectedContentType();
      }
      
      
      emscripten_fetch_close(fetch);


      /**
       * Secondly, use the retrieved data.
       **/

      try
      {
        if (context.get() == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
        else
        {
          switch (context->GetCommand().GetType())
          {
            case IOracleCommand::Type_OrthancRestApi:
            {
              OrthancRestApiCommand::SuccessMessage message
                (context->GetTypedCommand<OrthancRestApiCommand>(), headers, answer);
              context->EmitMessage(message);
              break;
            }
            
            case IOracleCommand::Type_GetOrthancImage:
            {
              context->GetTypedCommand<GetOrthancImageCommand>().ProcessHttpAnswer
                (context->GetEmitter(), context->GetReceiver(), answer, headers);
              break;
            }
          
            case IOracleCommand::Type_GetOrthancWebViewerJpeg:
            {
              context->GetTypedCommand<GetOrthancWebViewerJpegCommand>().ProcessHttpAnswer
                (context->GetEmitter(), context->GetReceiver(), answer);
              break;
            }
          
            default:
              LOG(ERROR) << "Command type not implemented by the WebAssembly Oracle: "
                         << context->GetCommand().GetType();
          }
        }
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(ERROR) << "Error while processing a fetch answer in the oracle: " << e.What();
      }
    }

    static void FetchFailed(emscripten_fetch_t *fetch)
    {
      std::auto_ptr<FetchContext> context(reinterpret_cast<FetchContext*>(fetch->userData));
      
      LOG(ERROR) << "Fetching " << fetch->url << " failed, HTTP failure status code: " << fetch->status;

      /**
       * TODO - The following code leads to an infinite recursion, at
       * least with Firefox running on incognito mode => WHY?
       **/      
      //emscripten_fetch_close(fetch); // Also free data on failure.
    }


    class FetchCommand : public boost::noncopyable
    {
    private:
      WebAssemblyOracle&             oracle_;
      const IObserver&               receiver_;
      std::auto_ptr<IOracleCommand>  command_;
      Orthanc::HttpMethod            method_;
      std::string                    uri_;
      std::string                    body_;
      HttpHeaders                    headers_;
      unsigned int                   timeout_;
      std::string                    expectedContentType_;

    public:
      FetchCommand(WebAssemblyOracle& oracle,
                   const IObserver& receiver,
                   IOracleCommand* command) :
        oracle_(oracle),
        receiver_(receiver),
        command_(command),
        method_(Orthanc::HttpMethod_Get),
        timeout_(0)
      {
        if (command == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
      }

      void SetMethod(Orthanc::HttpMethod method)
      {
        method_ = method;
      }

      void SetUri(const std::string& uri)
      {
        uri_ = uri;
      }

      void SetBody(std::string& body /* will be swapped */)
      {
        body_.swap(body);
      }

      void SetHttpHeaders(const HttpHeaders& headers)
      {
        headers_ = headers;
      }

      void SetTimeout(unsigned int timeout)
      {
        timeout_ = timeout;
      }

      void Execute()
      {
        if (command_.get() == NULL)
        {
          // Cannot call Execute() twice
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);          
        }

        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);

        const char* method;
      
        switch (method_)
        {
          case Orthanc::HttpMethod_Get:
            method = "GET";
            break;

          case Orthanc::HttpMethod_Post:
            method = "POST";
            break;

          case Orthanc::HttpMethod_Delete:
            method = "DELETE";
            break;

          case Orthanc::HttpMethod_Put:
            method = "PUT";
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        }

        strcpy(attr.requestMethod, method);

        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
        attr.onsuccess = FetchSucceeded;
        attr.onerror = FetchFailed;
        attr.timeoutMSecs = timeout_ * 1000;

        std::vector<const char*> headers;
        headers.reserve(2 * headers_.size() + 1);

        std::string expectedContentType;
        
        for (HttpHeaders::const_iterator it = headers_.begin(); it != headers_.end(); ++it)
        {
          std::string key;
          Orthanc::Toolbox::ToLowerCase(key, it->first);
          
          if (key == "accept")
          {
            expectedContentType = it->second;
          }

          if (key != "accept-encoding")  // Web browsers forbid the modification of this HTTP header
          {
            headers.push_back(it->first.c_str());
            headers.push_back(it->second.c_str());
          }
        }
        
        headers.push_back(NULL);  // Termination of the array of HTTP headers

        attr.requestHeaders = &headers[0];

        if (!body_.empty())
        {
          attr.requestDataSize = body_.size();
          attr.requestData = body_.c_str();
        }

        // Must be the last call to prevent memory leak on error
        attr.userData = new FetchContext(oracle_, receiver_, command_.release(), expectedContentType);
        emscripten_fetch(&attr, uri_.c_str());
      }        
    };
    
    
    void Execute(const IObserver& receiver,
                 OrthancRestApiCommand* command)
    {
      FetchCommand fetch(*this, receiver, command);

      fetch.SetMethod(command->GetMethod());
      fetch.SetUri(command->GetUri());
      fetch.SetHttpHeaders(command->GetHttpHeaders());
      fetch.SetTimeout(command->GetTimeout());
      
      if (command->GetMethod() == Orthanc::HttpMethod_Put ||
          command->GetMethod() == Orthanc::HttpMethod_Put)
      {
        std::string body;
        command->SwapBody(body);
        fetch.SetBody(body);
      }
      
      fetch.Execute();
    }
    
    
    void Execute(const IObserver& receiver,
                 GetOrthancImageCommand* command)
    {
      FetchCommand fetch(*this, receiver, command);

      fetch.SetUri(command->GetUri());
      fetch.SetHttpHeaders(command->GetHttpHeaders());
      fetch.SetTimeout(command->GetTimeout());
      
      fetch.Execute();
    }
    
    
    void Execute(const IObserver& receiver,
                 GetOrthancWebViewerJpegCommand* command)
    {
      FetchCommand fetch(*this, receiver, command);

      fetch.SetUri(command->GetUri());
      fetch.SetHttpHeaders(command->GetHttpHeaders());
      fetch.SetTimeout(command->GetTimeout());
      
      fetch.Execute();
    }

    
  public:
    WebAssemblyOracle(MessageBroker& broker) :
      IObservable(broker)
    {
    }
    
    virtual void Schedule(const IObserver& receiver,
                          IOracleCommand* command)
    {
      std::auto_ptr<IOracleCommand> protection(command);

      if (command == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      switch (command->GetType())
      {
        case IOracleCommand::Type_OrthancRestApi:
          Execute(receiver, dynamic_cast<OrthancRestApiCommand*>(protection.release()));
          break;
        
        case IOracleCommand::Type_GetOrthancImage:
          Execute(receiver, dynamic_cast<GetOrthancImageCommand*>(protection.release()));
          break;

        case IOracleCommand::Type_GetOrthancWebViewerJpeg:
          Execute(receiver, dynamic_cast<GetOrthancWebViewerJpegCommand*>(protection.release()));
          break;          
            
        default:
          LOG(ERROR) << "Command type not implemented by the WebAssembly Oracle: " << command->GetType();
      }
    }

    virtual void Start()
    {
    }

    virtual void Stop()
    {
    }
  };
}




boost::shared_ptr<OrthancStone::DicomVolumeImage>  ct_(new OrthancStone::DicomVolumeImage);

boost::shared_ptr<OrthancStone::OrthancSeriesVolumeProgressiveLoader>  loader_;

std::auto_ptr<OrthancStone::VolumeSlicerViewport>  viewport1_;
std::auto_ptr<OrthancStone::VolumeSlicerViewport>  viewport2_;
std::auto_ptr<OrthancStone::VolumeSlicerViewport>  viewport3_;

OrthancStone::MessageBroker  broker_;
OrthancStone::WebAssemblyOracle  oracle_(broker_);


EM_BOOL OnWindowResize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
  try
  {
    if (viewport1_.get() != NULL)
    {
      viewport1_->UpdateSize();
    }
  
    if (viewport2_.get() != NULL)
    {
      viewport2_->UpdateSize();
    }
  
    if (viewport3_.get() != NULL)
    {
      viewport3_->UpdateSize();
    }
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Exception while updating canvas size: " << e.What();
  }
  
  return true;
}




EM_BOOL OnAnimationFrame(double time, void *userData)
{
  try
  {
    if (viewport1_.get() != NULL)
    {
      viewport1_->Refresh();
    }
  
    if (viewport2_.get() != NULL)
    {
      viewport2_->Refresh();
    }
  
    if (viewport3_.get() != NULL)
    {
      viewport3_->Refresh();
    }

    return true;
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Exception in the animation loop, stopping now: " << e.What();
    return false;
  }  
}


extern "C"
{
  int main(int argc, char const *argv[]) 
  {
    OrthancStone::StoneInitialize();
    Orthanc::Logging::EnableInfoLevel(true);
    // Orthanc::Logging::EnableTraceLevel(true);
    EM_ASM(window.dispatchEvent(new CustomEvent("WebAssemblyLoaded")););
  }

  EMSCRIPTEN_KEEPALIVE
  void Initialize()
  {
    try
    {
      loader_.reset(new OrthancStone::OrthancSeriesVolumeProgressiveLoader(ct_, oracle_, oracle_));
    
      viewport1_.reset(new OrthancStone::VolumeSlicerViewport(broker_, "mycanvas1", OrthancStone::VolumeProjection_Axial));
      viewport1_->SetSlicer(0, loader_, *loader_, new OrthancStone::GrayscaleStyleConfigurator);
      viewport1_->UpdateSize();

      viewport2_.reset(new OrthancStone::VolumeSlicerViewport(broker_, "mycanvas2", OrthancStone::VolumeProjection_Coronal));
      viewport2_->SetSlicer(0, loader_, *loader_, new OrthancStone::GrayscaleStyleConfigurator);
      viewport2_->UpdateSize();

      viewport3_.reset(new OrthancStone::VolumeSlicerViewport(broker_, "mycanvas3", OrthancStone::VolumeProjection_Sagittal));
      viewport3_->SetSlicer(0, loader_, *loader_, new OrthancStone::GrayscaleStyleConfigurator);
      viewport3_->UpdateSize();

      emscripten_set_resize_callback("#window", NULL, false, OnWindowResize);
    
      emscripten_request_animation_frame_loop(OnAnimationFrame, NULL);

      oracle_.Start();
      loader_->LoadSeries("a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa");
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "Exception during Initialize(): " << e.What();
    }
  }
}
