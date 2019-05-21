#include "WasmViewport.h"

#include <vector>
#include <memory>

std::vector<std::shared_ptr<Deprecated::WidgetViewport>> wasmViewports;

void AttachWidgetToWasmViewport(const char* htmlCanvasId, Deprecated::IWidget* centralWidget) {
    std::shared_ptr<Deprecated::WidgetViewport> viewport(CreateWasmViewportFromCpp(htmlCanvasId));
    viewport->SetCentralWidget(centralWidget);

    wasmViewports.push_back(viewport);
}
