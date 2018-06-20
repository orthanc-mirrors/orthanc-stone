#pragma once

#include <Framework/Viewport/WidgetViewport.h>

#include <emscripten/emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

  // JS methods accessible from C++
  extern OrthancStone::WidgetViewport* CreateWasmViewportFromCpp(const char* htmlCanvasId);

#ifdef __cplusplus
}
#endif

extern void AttachWidgetToWasmViewport(const char* htmlCanvasId, OrthancStone::IWidget* centralWidget);
