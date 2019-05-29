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
#include "../../Framework/Oracle/SleepOracleCommand.h"
#include "../../Framework/Oracle/WebAssemblyOracle.h"
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

    void FitContent()
    {
      GetScene().FitContent(context_.GetCanvasWidth(), context_.GetCanvasHeight());
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

      Refresh();

      viewport_.FitContent();
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
        viewport_.Refresh();
      }
    }

    void Scroll(int delta)
    {
      if (!planes_.empty())
      {
        int tmp = static_cast<int>(currentPlane_) + delta;
        unsigned int next;

        if (tmp < 0)
        {
          next = 0;
        }
        else if (tmp >= static_cast<int>(planes_.size()))
        {
          next = planes_.size() - 1;
        }
        else
        {
          next = static_cast<size_t>(tmp);
        }

        if (next != currentPlane_)
        {
          currentPlane_ = next;
          Refresh();
        }
      }
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


static bool ctrlDown_ = false;


EM_BOOL OnMouseWheel(int eventType,
                     const EmscriptenWheelEvent *wheelEvent,
                     void *userData)
{
  try
  {
    if (userData != NULL)
    {
      int delta = 0;

      if (wheelEvent->deltaY < 0)
      {
        delta = -1;
      }
           
      if (wheelEvent->deltaY > 0)
      {
        delta = 1;
      }

      if (ctrlDown_)
      {
        delta *= 10;
      }
           
      reinterpret_cast<OrthancStone::VolumeSlicerViewport*>(userData)->Scroll(delta);
    }
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Exception in the wheel event: " << e.What();
  }
  
  return true;
}


EM_BOOL OnKey(int eventType,
              const EmscriptenKeyboardEvent *keyEvent,
              void *userData)
{
  ctrlDown_ = keyEvent->ctrlKey;
  return false;
}




namespace OrthancStone
{
  class TestSleep : public IObserver
  {
  private:
    WebAssemblyOracle&  oracle_;

    void Schedule()
    {
      oracle_.Schedule(*this, new OrthancStone::SleepOracleCommand(2000));
    }
    
    void Handle(const SleepOracleCommand::TimeoutMessage& message)
    {
      LOG(INFO) << "TIMEOUT";
      Schedule();
    }
    
  public:
    TestSleep(MessageBroker& broker,
              WebAssemblyOracle& oracle) :
      IObserver(broker),
      oracle_(oracle)
    {
      oracle.RegisterObserverCallback(
        new Callable<TestSleep, SleepOracleCommand::TimeoutMessage>
        (*this, &TestSleep::Handle));

      LOG(INFO) << "STARTING";
      Schedule();
    }
  };

  static TestSleep testSleep(broker_, oracle_);
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

      emscripten_set_wheel_callback("mycanvas1", viewport1_.get(), false, OnMouseWheel);
      emscripten_set_wheel_callback("mycanvas2", viewport2_.get(), false, OnMouseWheel);
      emscripten_set_wheel_callback("mycanvas3", viewport3_.get(), false, OnMouseWheel);

      emscripten_set_keydown_callback("#window", NULL, false, OnKey);
      emscripten_set_keyup_callback("#window", NULL, false, OnKey);
    
      emscripten_request_animation_frame_loop(OnAnimationFrame, NULL);

      loader_->LoadSeries("a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa");
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "Exception during Initialize(): " << e.What();
    }
  }
}
