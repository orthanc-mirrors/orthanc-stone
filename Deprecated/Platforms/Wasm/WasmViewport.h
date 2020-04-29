#pragma once

#include "../../Framework/Deprecated/Viewport/WidgetViewport.h"

#include <emscripten/emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

  // JS methods accessible from C++
  extern Deprecated::WidgetViewport* CreateWasmViewportFromCpp(const char* htmlCanvasId);

#ifdef __cplusplus
}
#endif

extern void AttachWidgetToWasmViewport(const char* htmlCanvasId, Deprecated::IWidget* centralWidget);
