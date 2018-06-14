#include "Defaults.h"

#include "WasmWebService.h"
#include <Framework/dev.h>
#include "Framework/Widgets/TestCairoWidget.h"
#include <Framework/Viewport/WidgetViewport.h>
#include <Framework/Widgets/LayerWidget.h>

static unsigned int width_ = 0;
static unsigned int height_ = 0;

/**********************************/

static std::auto_ptr<OrthancStone::BasicWasmApplication> application;
static std::shared_ptr<OrthancStone::WidgetViewport> viewport_;
static OrthancStone::ChangeObserver changeObserver_;
static OrthancStone::StatusBar statusBar_;


static std::list<std::shared_ptr<OrthancStone::WidgetViewport>> viewports_;

#ifdef __cplusplus
extern "C" {
#endif

  using namespace OrthancStone;

  // when WASM needs a C++ viewport
  ViewportHandle EMSCRIPTEN_KEEPALIVE CreateCppViewport() {
    
    std::shared_ptr<OrthancStone::WidgetViewport> viewport(new OrthancStone::WidgetViewport);
    printf("viewport %x\n", viewport.get());

    viewports_.push_back(viewport);

    printf("There are now %d viewports in C++\n", viewports_.size());

    viewport->SetStatusBar(statusBar_);
    viewport->Register(changeObserver_);

    // TODO: remove once we really want to handle multiple viewports
    viewport_ = viewport;
    return viewport.get();
  }

  // when WASM does not need a viewport anymore, it should release it 
  void EMSCRIPTEN_KEEPALIVE ReleaseCppViewport(ViewportHandle viewport) {
    viewports_.remove_if([viewport](const std::shared_ptr<OrthancStone::WidgetViewport>& v) { return v.get() == viewport;});

    printf("There are now %d viewports in C++\n", viewports_.size());
  }


  void EMSCRIPTEN_KEEPALIVE CreateWasmApplication(ViewportHandle viewport) {

    printf("CreateWasmApplication\n");

    application.reset(CreateUserApplication());

    boost::program_options::options_description options;
    application->DeclareStartupOptions(options);
  }

  void EMSCRIPTEN_KEEPALIVE SetStartupParameter(const char* keyc,
                                                  const char* value) {
    application->SetStartupParameter(keyc, value);
  }

  void EMSCRIPTEN_KEEPALIVE StartWasmApplication() {

    printf("StartWasmApplication\n");

    // recreate a command line from uri arguments and parse it
    boost::program_options::variables_map parameters;
    application->GetStartupParameters(parameters);

    BasicWasmApplicationContext& context = dynamic_cast<BasicWasmApplicationContext&>(application->CreateApplicationContext(OrthancStone::WasmWebService::GetInstance(), viewport_));;
    application->Initialize(statusBar_, parameters);

    viewport_->SetSize(width_, height_);
    printf("StartWasmApplication - completed\n");
  }

  // void EMSCRIPTEN_KEEPALIVE ViewportUpdate(const char* _instanceId) {
  //   printf("updating viewport content, Instance = [%s]\n", instanceId.c_str());

  //   layerSource->LoadFrame(instanceId, 0);
  //   printf("frame loaded\n");
  //   instanceWidget->UpdateContent();

  //   printf("update should be done\n");
  // }
  
  // void EMSCRIPTEN_KEEPALIVE ViewportStart()
  // {

  //   viewport_.reset(new OrthancStone::WidgetViewport);
  //   viewport_->SetStatusBar(statusBar_);
  //   viewport_->Register(changeObserver_);
  //   instanceWidget.reset(new OrthancStone::LayerWidget);
  //   layerSource = new OrthancStone::OrthancFrameLayerSource(OrthancStone::WasmWebService::GetInstance());

  //   if (!instanceId.empty())
  //   {
  //     layerSource->LoadFrame(instanceId, 0);
  //   } else {
  //     printf("No instance provided so far\n");
  //   }

  //   instanceWidget->AddLayer(layerSource);

  //   {
  //     OrthancStone::RenderStyle s;
  //     //s.drawGrid_ = true;
  //     s.alpha_ = 1;
  //     s.windowing_ = OrthancStone::ImageWindowing_Bone;
  //     instanceWidget->SetLayerStyle(0, s);
  //   }

  //   viewport_->SetCentralWidget(instanceWidget.release());
  //   viewport_->SetSize(width_, height_);


  // }

  void EMSCRIPTEN_KEEPALIVE NotifyUpdateContent()
  {
    // TODO Only launch the JavaScript timer if "HasUpdateContent()"
    if (viewport_.get() != NULL &&
        viewport_->HasUpdateContent())
    {
      viewport_->UpdateContent();
    }

  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportSetSize(unsigned int width, unsigned int height)
  {
    width_ = width;
    height_ = height;
    
    if (viewport_.get() != NULL)
    {
      viewport_->SetSize(width, height);
    }
  }

  int EMSCRIPTEN_KEEPALIVE ViewportRender(OrthancStone::WidgetViewport* viewport,
                                          unsigned int width,
                                          unsigned int height,
                                          uint8_t* data)
  {
    changeObserver_.Reset();

    //printf("ViewportRender called %dx%d\n", width, height);
    if (width == 0 ||
        height == 0)
    {
      return 1;
    }

    Orthanc::ImageAccessor surface;
    surface.AssignWritable(Orthanc::PixelFormat_BGRA32, width, height, 4 * width, data);

    viewport->Render(surface);

    // Convert from BGRA32 memory layout (only color mode supported by
    // Cairo, which corresponds to CAIRO_FORMAT_ARGB32) to RGBA32 (as
    // expected by HTML5 canvas). This simply amounts to swapping the
    // B and R channels.
    uint8_t* p = data;
    for (unsigned int y = 0; y < height; y++) {
      for (unsigned int x = 0; x < width; x++) {
        uint8_t tmp = p[0];
        p[0] = p[2];
        p[2] = tmp;
        
        p += 4;
      }
    }

    return 1;
  }


  void EMSCRIPTEN_KEEPALIVE ViewportMouseDown(unsigned int rawButton,
                                              int x,
                                              int y,
                                              unsigned int rawModifiers)
  {
    OrthancStone::MouseButton button;
    switch (rawButton)
    {
      case 0:
        button = OrthancStone::MouseButton_Left;
        break;

      case 1:
        button = OrthancStone::MouseButton_Middle;
        break;

      case 2:
        button = OrthancStone::MouseButton_Right;
        break;

      default:
        return;  // Unknown button
    }

    if (viewport_.get() != NULL)
    {
      viewport_->MouseDown(button, x, y, OrthancStone::KeyboardModifiers_None /* TODO */);
    }
  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportMouseWheel(int deltaY,
                                               int x,
                                               int y,
                                               int isControl)
  {
    if (viewport_.get() != NULL &&
        deltaY != 0)
    {
      OrthancStone::MouseWheelDirection direction = (deltaY < 0 ?
                                                     OrthancStone::MouseWheelDirection_Up :
                                                     OrthancStone::MouseWheelDirection_Down);
      OrthancStone::KeyboardModifiers modifiers = OrthancStone::KeyboardModifiers_None;

      if (isControl != 0)
      {
        modifiers = OrthancStone::KeyboardModifiers_Control;
      }

      viewport_->MouseWheel(direction, x, y, modifiers);
    }
  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportMouseMove(int x,
                                              int y)
  {
    if (viewport_.get() != NULL)
    {
      viewport_->MouseMove(x, y);
    }
  }
  
  void EMSCRIPTEN_KEEPALIVE ViewportKeyPressed(const char* key, 
                                               bool isShiftPressed, 
                                               bool isControlPressed,
                                               bool isAltPressed)
                                               
  {
    if (viewport_.get() != NULL)
    {
      OrthancStone::KeyboardModifiers modifiers = OrthancStone::KeyboardModifiers_None;
      if (isShiftPressed) {
        modifiers = static_cast<OrthancStone::KeyboardModifiers>(modifiers + OrthancStone::KeyboardModifiers_Shift);
      }
      if (isControlPressed) {
        modifiers = static_cast<OrthancStone::KeyboardModifiers>(modifiers + OrthancStone::KeyboardModifiers_Control);
      }
      if (isAltPressed) {
        modifiers = static_cast<OrthancStone::KeyboardModifiers>(modifiers + OrthancStone::KeyboardModifiers_Alt);
      }
      printf("key pressed : %c\n", key[0]);
      viewport_->KeyPressed(key[0], modifiers);
    }
  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportMouseUp()
  {
    if (viewport_.get() != NULL)
    {
      viewport_->MouseUp();
    }
  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportMouseEnter()
  {
    if (viewport_.get() != NULL)
    {
      viewport_->MouseEnter();
    }
  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportMouseLeave()
  {
    if (viewport_.get() != NULL)
    {
      viewport_->MouseLeave();
    }
  }


#ifdef __cplusplus
}
#endif
