/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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

#include <Framework/Viewport/WebAssemblyViewport.h>
#include <Framework/Scene2D/OpenGLCompositor.h>
#include <Framework/Scene2D/PanSceneTracker.h>
#include <Framework/Scene2D/RotateSceneTracker.h>
#include <Framework/Scene2D/ZoomSceneTracker.h>
#include <Framework/Scene2DViewport/UndoStack.h>
#include <Framework/Scene2DViewport/ViewportController.h>
#include <Framework/Loaders/OrthancSeriesVolumeProgressiveLoader.h>
#include <Framework/Oracle/SleepOracleCommand.h>
#include <Framework/Oracle/WebAssemblyOracle.h>
#include <Framework/Scene2D/GrayscaleStyleConfigurator.h>
#include <Framework/StoneInitialization.h>
#include <Framework/Volumes/VolumeSceneLayerSource.h>

#include <Core/OrthancException.h>

#include <emscripten/html5.h>
#include <emscripten.h>

#include <boost/make_shared.hpp>


class ViewportManager;

static const unsigned int FONT_SIZE = 32;

boost::shared_ptr<OrthancStone::DicomVolumeImage>                     ct_(new OrthancStone::DicomVolumeImage);
boost::shared_ptr<OrthancStone::OrthancSeriesVolumeProgressiveLoader> loader_;
std::unique_ptr<ViewportManager>                        widget1_;
std::unique_ptr<ViewportManager>                        widget2_;
std::unique_ptr<ViewportManager>                        widget3_;
//OrthancStone::MessageBroker                                         broker_;
//OrthancStone::WebAssemblyOracle                                     oracle_(broker_);
std::unique_ptr<OrthancStone::IFlexiblePointerTracker>                tracker_;
static std::map<std::string, std::string>                             arguments_;
static bool                                                           ctrlDown_ = false;


#if 0

// use the one from WebAssemblyViewport
static OrthancStone::PointerEvent* ConvertMouseEvent(
  const EmscriptenMouseEvent&        source,
  OrthancStone::IViewport& viewport)
{

  std::unique_ptr<OrthancStone::PointerEvent> target(
    new OrthancStone::PointerEvent);

  target->AddPosition(viewport.GetPixelCenterCoordinates(
                        source.targetX, source.targetY));
  target->SetAltModifier(source.altKey);
  target->SetControlModifier(source.ctrlKey);
  target->SetShiftModifier(source.shiftKey);

  return target.release();
}
#endif


EM_BOOL OnMouseEvent(int eventType, 
                     const EmscriptenMouseEvent *mouseEvent, 
                     void *userData)
{
  if (mouseEvent != NULL &&
      userData != NULL)
  {
    boost::shared_ptr<OrthancStone::WebGLViewport>& viewport = 
      *reinterpret_cast<boost::shared_ptr<OrthancStone::WebGLViewport>*>(userData);

    std::unique_ptr<OrthancStone::IViewport::ILock> lock = (*viewport)->Lock();
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();
    
    switch (eventType)
    {
      case EMSCRIPTEN_EVENT_CLICK:
      {
        static unsigned int count = 0;
        char buf[64];
        sprintf(buf, "click %d", count++);

        std::unique_ptr<OrthancStone::TextSceneLayer> layer(new OrthancStone::TextSceneLayer);
        layer->SetText(buf);
        scene.SetLayer(100, layer.release());
        lock->Invalidate();
        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEDOWN:
      {
        boost::shared_ptr<OrthancStone::IFlexiblePointerTracker> t;

        {
          std::unique_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, controller->GetViewport()));

          switch (mouseEvent->button)
          {
            case 0:  // Left button
              emscripten_console_log("Creating RotateSceneTracker");
              t.reset(new OrthancStone::RotateSceneTracker(
                        viewport, *event));
              break;

            case 1:  // Middle button
              emscripten_console_log("Creating PanSceneTracker");
              LOG(INFO) << "Creating PanSceneTracker" ;
              t.reset(new OrthancStone::PanSceneTracker(
                        viewport, *event));
              break;

            case 2:  // Right button
              emscripten_console_log("Creating ZoomSceneTracker");
              t.reset(new OrthancStone::ZoomSceneTracker(
                        viewport, *event, controller->GetViewport().GetCanvasWidth()));
              break;

            default:
              break;
          }
        }

        if (t.get() != NULL)
        {
          tracker_.reset(
            new OrthancStone::ActiveTracker(t, controller->GetViewport().GetCanvasIdentifier()));
          controller->GetViewport().Refresh();
        }

        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEMOVE:
        if (tracker_.get() != NULL)
        {
          std::unique_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, controller->GetViewport()));
          tracker_->PointerMove(*event);
          controller->GetViewport().Refresh();
        }
        break;

      case EMSCRIPTEN_EVENT_MOUSEUP:
        if (tracker_.get() != NULL)
        {
          std::unique_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, controller->GetViewport()));
          tracker_->PointerUp(*event);
          controller->GetViewport().Refresh();
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


void SetupEvents(const std::string& canvas,
                 boost::shared_ptr<OrthancStone::WebGLViewport>& viewport)
{
  emscripten_set_mousedown_callback(canvas.c_str(), &viewport, false, OnMouseEvent);
  emscripten_set_mousemove_callback(canvas.c_str(), &viewport, false, OnMouseEvent);
  emscripten_set_mouseup_callback(canvas.c_str(), &viewport, false, OnMouseEvent);
}

  class ViewportManager : public OrthanStone::ObserverBase<ViewportManager>
  {
  private:
    OrthancStone::WebAssemblyViewport         viewport_;
    std::unique_ptr<VolumeSceneLayerSource>   source_;
    VolumeProjection                          projection_;
    std::vector<CoordinateSystem3D>           planes_;
    size_t                                    currentPlane_;

    void Handle(const DicomVolumeImage::GeometryReadyMessage& message)
    {
      LOG(INFO) << "Geometry is available";

      const VolumeImageGeometry& geometry = message.GetOrigin().GetGeometry();

      const unsigned int depth = geometry.GetProjectionDepth(projection_);

      // select an initial cutting plane halfway through the volume
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
    ViewportManager(const std::string& canvas,
                    VolumeProjection projection) :
      projection_(projection),
      currentPlane_(0)
    {
      viewport_ = OrthancStone::WebGLViewport::Create(canvas);
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
        new Callable<ViewportManager, DicomVolumeImage::GeometryReadyMessage>
        (*this, &ViewportManager::Handle));

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

    size_t GetSlicesCount() const
    {
      return planes_.size();
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


EM_BOOL OnWindowResize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
  try
  {
    if (widget1_.get() != NULL)
    {
      widget1_->UpdateSize();
    }
  
    if (widget2_.get() != NULL)
    {
      widget2_->UpdateSize();
    }
  
    if (widget3_.get() != NULL)
    {
      widget3_->UpdateSize();
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
    if (widget1_.get() != NULL)
    {
      widget1_->Refresh();
    }
  
    if (widget2_.get() != NULL)
    {
      widget2_->Refresh();
    }
  
    if (widget3_.get() != NULL)
    {
      widget3_->Refresh();
    }

    return true;
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Exception in the animation loop, stopping now: " << e.What();
    return false;
  }  
}

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

      OrthancStone::ViewportManager& widget =
        *reinterpret_cast<OrthancStone::ViewportManager*>(userData);
      
      if (ctrlDown_)
      {
        delta *= static_cast<int>(widget.GetSlicesCount() / 10);
      }

      widget.Scroll(delta);
    }
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Exception in the wheel event: " << e.What();
  }
  
  return true;
}


EM_BOOL OnKeyDown(int eventType,
                  const EmscriptenKeyboardEvent *keyEvent,
                  void *userData)
{
  ctrlDown_ = keyEvent->ctrlKey;
  return false;
}


EM_BOOL OnKeyUp(int eventType,
                const EmscriptenKeyboardEvent *keyEvent,
                void *userData)
{
  ctrlDown_ = false;
  return false;
}



#if 0
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

  //static TestSleep testSleep(broker_, oracle_);
}
#endif

static bool GetArgument(std::string& value,
                        const std::string& key)
{
  std::map<std::string, std::string>::const_iterator found = arguments_.find(key);

  if (found == arguments_.end())
  {
    return false;
  }
  else
  {
    value = found->second;
    return true;
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
  void SetArgument(const char* key, const char* value)
  {
    // This is called for each GET argument (cf. "app.js")
    LOG(INFO) << "Received GET argument: [" << key << "] = [" << value << "]";
    arguments_[key] = value;
  }

  EMSCRIPTEN_KEEPALIVE
  void Initialize()
  {
    try
    {
      oracle_.SetOrthancRoot("..");
      
      loader_.reset(new OrthancStone::OrthancSeriesVolumeProgressiveLoader(ct_, oracle_, oracle_));
    
      widget1_.reset(new OrthancStone::ViewportManager("mycanvas1", OrthancStone::VolumeProjection_Axial));
      {
        std::unique_ptr<OrthancStone::GrayscaleStyleConfigurator> style(new OrthancStone::GrayscaleStyleConfigurator);
        style->SetLinearInterpolation(true);
        style->SetWindowing(OrthancStone::ImageWindowing_Bone);
        widget1_->SetSlicer(0, loader_, *loader_, style.release());
      }
      widget1_->UpdateSize();

      widget2_.reset(new OrthancStone::ViewportManager("mycanvas2", OrthancStone::VolumeProjection_Coronal));
      {
        std::unique_ptr<OrthancStone::GrayscaleStyleConfigurator> style(new OrthancStone::GrayscaleStyleConfigurator);
        style->SetLinearInterpolation(true);
        style->SetWindowing(OrthancStone::ImageWindowing_Bone);
        widget2_->SetSlicer(0, loader_, *loader_, style.release());
      }
      widget2_->UpdateSize();

      widget3_.reset(new OrthancStone::ViewportManager("mycanvas3", OrthancStone::VolumeProjection_Sagittal));
      {
        std::unique_ptr<OrthancStone::GrayscaleStyleConfigurator> style(new OrthancStone::GrayscaleStyleConfigurator);
        style->SetLinearInterpolation(true);
        style->SetWindowing(OrthancStone::ImageWindowing_Bone);
        widget3_->SetSlicer(0, loader_, *loader_, style.release());
      }
      widget3_->UpdateSize();

      emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, OnWindowResize); // DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1 !!

      emscripten_set_wheel_callback("#mycanvas1", widget1_.get(), false, OnMouseWheel);
      emscripten_set_wheel_callback("#mycanvas2", widget2_.get(), false, OnMouseWheel);
      emscripten_set_wheel_callback("#mycanvas3", widget3_.get(), false, OnMouseWheel);

      emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, OnKeyDown);
      emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, OnKeyUp);
    
      //emscripten_request_animation_frame_loop(OnAnimationFrame, NULL);

      std::string ct;
      if (GetArgument(ct, "ct"))
      {
        //loader_->LoadSeries("a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa");
        loader_->LoadSeries(ct);
      }
      else
      {
        LOG(ERROR) << "No Orthanc identifier for the CT series was provided";
      }
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "Exception during Initialize(): " << e.What();
    }
  }
}

