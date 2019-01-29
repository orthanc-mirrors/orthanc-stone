#include "Defaults.h"

#include "WasmWebService.h"
#include "WasmDelayedCallExecutor.h"
#include <Framework/dev.h>
#include "Framework/Widgets/TestCairoWidget.h"
#include <Framework/Viewport/WidgetViewport.h>
#include <algorithm>
#include "Applications/Wasm/StartupParametersBuilder.h"
#include "Platforms/Wasm/WasmPlatformApplicationAdapter.h"

static unsigned int width_ = 0;
static unsigned int height_ = 0;

/**********************************/

static std::unique_ptr<OrthancStone::IStoneApplication> application;
static std::unique_ptr<OrthancStone::WasmPlatformApplicationAdapter> applicationWasmAdapter = NULL;
static std::unique_ptr<OrthancStone::StoneApplicationContext> context;
static OrthancStone::StartupParametersBuilder startupParametersBuilder;
static OrthancStone::MessageBroker broker;

static OrthancStone::ViewportContentChangedObserver viewportContentChangedObserver_(broker);
static OrthancStone::StatusBar statusBar_;

static std::list<std::shared_ptr<OrthancStone::WidgetViewport>> viewports_;

std::shared_ptr<OrthancStone::WidgetViewport> FindViewportSharedPtr(ViewportHandle viewport) {
  for (const auto& v : viewports_) {
    if (v.get() == viewport) {
      return v;
    }
  }
  assert(false);
  return std::shared_ptr<OrthancStone::WidgetViewport>();
}

#ifdef __cplusplus
extern "C" {
#endif

  using namespace OrthancStone;

  // when WASM needs a C++ viewport
  ViewportHandle EMSCRIPTEN_KEEPALIVE CreateCppViewport() {
    
    std::shared_ptr<OrthancStone::WidgetViewport> viewport(new OrthancStone::WidgetViewport(broker));
    printf("viewport %x\n", (int)viewport.get());

    viewports_.push_back(viewport);

    printf("There are now %lu viewports in C++\n", viewports_.size());

    viewport->SetStatusBar(statusBar_);

    viewport->RegisterObserverCallback(
      new Callable<ViewportContentChangedObserver, IViewport::ViewportChangedMessage>
      (viewportContentChangedObserver_, &ViewportContentChangedObserver::OnViewportChanged));

    return viewport.get();
  }

  // when WASM does not need a viewport anymore, it should release it 
  void EMSCRIPTEN_KEEPALIVE ReleaseCppViewport(ViewportHandle viewport) {
    viewports_.remove_if([viewport](const std::shared_ptr<OrthancStone::WidgetViewport>& v) { return v.get() == viewport;});

    printf("There are now %lu viewports in C++\n", viewports_.size());
  }

  void EMSCRIPTEN_KEEPALIVE CreateWasmApplication(ViewportHandle viewport) {

    printf("CreateWasmApplication\n");

    application.reset(CreateUserApplication(broker));
    applicationWasmAdapter.reset(CreateWasmApplicationAdapter(broker, application.get())); 
    WasmWebService::SetBroker(broker);
    WasmDelayedCallExecutor::SetBroker(broker);

    startupParametersBuilder.Clear();
  }

  void EMSCRIPTEN_KEEPALIVE SetStartupParameter(const char* keyc,
                                                  const char* value) {
    startupParametersBuilder.SetStartupParameter(keyc, value);
  }

  void EMSCRIPTEN_KEEPALIVE StartWasmApplication(const char* baseUri) {

    printf("StartWasmApplication\n");

    // recreate a command line from uri arguments and parse it
    boost::program_options::variables_map parameters;
    boost::program_options::options_description options;
    application->DeclareStartupOptions(options);
    startupParametersBuilder.GetStartupParameters(parameters, options);

    context.reset(new OrthancStone::StoneApplicationContext(broker));
    context->SetOrthancBaseUrl(baseUri);
    printf("Base URL to Orthanc API: [%s]\n", baseUri);
    context->SetWebService(OrthancStone::WasmWebService::GetInstance());
    context->SetDelayedCallExecutor(OrthancStone::WasmDelayedCallExecutor::GetInstance());
    application->Initialize(context.get(), statusBar_, parameters);
    application->InitializeWasm();

//    viewport->SetSize(width_, height_);
    printf("StartWasmApplication - completed\n");
  }
  
  void EMSCRIPTEN_KEEPALIVE WasmDoAnimation()
  {
    for (auto viewport : viewports_) {
      // TODO Only launch the JavaScript timer if "HasAnimation()"
      if (viewport->HasAnimation())
      {
        viewport->DoAnimation();
      }

    }

  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportSetSize(ViewportHandle viewport, unsigned int width, unsigned int height)
  {
    width_ = width;
    height_ = height;
    
    viewport->SetSize(width, height);
  }

  int EMSCRIPTEN_KEEPALIVE ViewportRender(ViewportHandle viewport,
                                          unsigned int width,
                                          unsigned int height,
                                          uint8_t* data)
  {
    viewportContentChangedObserver_.Reset();

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


  void EMSCRIPTEN_KEEPALIVE ViewportMouseDown(ViewportHandle viewport,
                                              unsigned int rawButton,
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

    viewport->MouseDown(button, x, y, OrthancStone::KeyboardModifiers_None, std::vector<OrthancStone::Touch>());
  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportMouseWheel(ViewportHandle viewport,
                                               int deltaY,
                                               int x,
                                               int y,
                                               int isControl)
  {
    if (deltaY != 0)
    {
      OrthancStone::MouseWheelDirection direction = (deltaY < 0 ?
                                                     OrthancStone::MouseWheelDirection_Up :
                                                     OrthancStone::MouseWheelDirection_Down);
      OrthancStone::KeyboardModifiers modifiers = OrthancStone::KeyboardModifiers_None;

      if (isControl != 0)
      {
        modifiers = OrthancStone::KeyboardModifiers_Control;
      }

      viewport->MouseWheel(direction, x, y, modifiers);
    }
  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportMouseMove(ViewportHandle viewport,
                                              int x,
                                              int y)
  {
    viewport->MouseMove(x, y, std::vector<OrthancStone::Touch>());
  }

  void GetTouchVector(std::vector<OrthancStone::Touch>& output,
                      int touchCount,
                      float x0,
                      float y0,
                      float x1,
                      float y1,
                      float x2,
                      float y2)
  {
    // TODO: it might be nice to try to pass all the x0,y0 coordinates as arrays but that's not so easy to pass array between JS and C++
    if (touchCount > 0)
    {
      output.push_back(OrthancStone::Touch(x0, y0));
    }
    if (touchCount > 1)
    {
      output.push_back(OrthancStone::Touch(x1, y1));
    }
    if (touchCount > 2)
    {
      output.push_back(OrthancStone::Touch(x2, y2));
    }

  }

  void EMSCRIPTEN_KEEPALIVE ViewportTouchStart(ViewportHandle viewport,
                                              int touchCount,
                                              float x0,
                                              float y0,
                                              float x1,
                                              float y1,
                                              float x2,
                                              float y2)
  {
    printf("touch start with %d touches\n", touchCount);

    std::vector<OrthancStone::Touch> touches;
    GetTouchVector(touches, touchCount, x0, y0, x1, y1, x2, y2);
    viewport->TouchStart(touches);
  }

  void EMSCRIPTEN_KEEPALIVE ViewportTouchMove(ViewportHandle viewport,
                                              int touchCount,
                                              float x0,
                                              float y0,
                                              float x1,
                                              float y1,
                                              float x2,
                                              float y2)
  {
    printf("touch move with %d touches\n", touchCount);

    std::vector<OrthancStone::Touch> touches;
    GetTouchVector(touches, touchCount, x0, y0, x1, y1, x2, y2);
    viewport->TouchMove(touches);
  }

  void EMSCRIPTEN_KEEPALIVE ViewportTouchEnd(ViewportHandle viewport,
                                              int touchCount,
                                              float x0,
                                              float y0,
                                              float x1,
                                              float y1,
                                              float x2,
                                              float y2)
  {
    printf("touch end with %d touches remaining\n", touchCount);

    std::vector<OrthancStone::Touch> touches;
    GetTouchVector(touches, touchCount, x0, y0, x1, y1, x2, y2);
    viewport->TouchEnd(touches);
  }

  void EMSCRIPTEN_KEEPALIVE ViewportKeyPressed(ViewportHandle viewport,
                                               int key,
                                               const char* keyChar, 
                                               bool isShiftPressed, 
                                               bool isControlPressed,
                                               bool isAltPressed)
                                               
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

    char c = 0;
    if (keyChar != NULL && key == OrthancStone::KeyboardKeys_Generic) {
      c = keyChar[0];
    }
    viewport->KeyPressed(static_cast<OrthancStone::KeyboardKeys>(key), c, modifiers);
  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportMouseUp(ViewportHandle viewport)
  {
    viewport->MouseUp();
  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportMouseEnter(ViewportHandle viewport)
  {
    viewport->MouseEnter();
  }
  

  void EMSCRIPTEN_KEEPALIVE ViewportMouseLeave(ViewportHandle viewport)
  {
    viewport->MouseLeave();
  }

  const char* EMSCRIPTEN_KEEPALIVE SendMessageToStoneApplication(const char* message) 
  {
    static std::string output; // we don't want the string to be deallocated when we return to JS code so we always use the same string (this is fine since JS is single-thread)

    printf("SendMessageToStoneApplication (JS -> C++): %s\n", message);

    if (applicationWasmAdapter.get() != NULL) {
      applicationWasmAdapter->HandleMessageFromWeb(output, std::string(message));
      return output.c_str();
    }
    printf("This stone application does not have a Web Adapter\n");
    return NULL;
  }


#ifdef __cplusplus
}
#endif
