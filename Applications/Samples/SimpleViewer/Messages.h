#pragma once

#include "Framework/Messages/MessageType.h"

namespace SimpleViewer
{
  enum SimpleViewerMessageType
  {
    SimpleViewerMessageType_First = OrthancStone::MessageType_CustomMessage,
    SimpleViewerMessageType_AppStatusUpdated

  };
}
