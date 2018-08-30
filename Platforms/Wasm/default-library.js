// this file contains the JS method you want to expose to C++ code

mergeInto(LibraryManager.library, {
  ScheduleWebViewportRedrawFromCpp: function(cppViewportHandle) {
    ScheduleWebViewportRedraw(cppViewportHandle);
  },
  CreateWasmViewportFromCpp: function(htmlCanvasId) {
    return CreateWasmViewport(htmlCanvasId);
  },
  // each time the StoneApplication updates its status, it may signal it through this method. i.e, to change the status of a button in the web interface
  UpdateStoneApplicationStatusFromCpp: function(statusUpdateMessage) {
    var statusUpdateMessage_ = UTF8ToString(statusUpdateMessage);
    UpdateWebApplication(statusUpdateMessage_);
  }
});
  