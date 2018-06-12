#include "BasicWasmApplicationContext.h"

namespace OrthancStone
{
  IWidget& BasicWasmApplicationContext::SetCentralWidget(IWidget* widget)   // Takes ownership
  {
    printf("BasicWasmApplicationContext::SetCentralWidget %x %x\n", centralViewport_.get(), widget);
    assert(centralViewport_.get() != NULL);
    centralViewport_->SetCentralWidget(widget);
    printf("BasicWasmApplicationContext::SetCentralWidget done\n");
    return *widget;
  }

}