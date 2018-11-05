#include "WasmViewport.h"

#include <vector>
#include <memory>

std::vector<std::shared_ptr<OrthancStone::WidgetViewport>> wasmViewports;

void AttachWidgetToWasmViewport(const char* htmlCanvasId, OrthancStone::IWidget* centralWidget) {
    std::shared_ptr<OrthancStone::WidgetViewport> viewport(CreateWasmViewportFromCpp(htmlCanvasId));
    viewport->SetCentralWidget(centralWidget);

    wasmViewports.push_back(viewport);
}