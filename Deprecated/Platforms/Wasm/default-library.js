// this file contains the JS method you want to expose to C++ code

mergeInto(LibraryManager.library, {

  ScheduleWebViewportRedrawFromCpp: function(cppViewportHandle) {
    window.ScheduleWebViewportRedraw(cppViewportHandle);
  },

  CreateWasmViewportFromCpp: function(htmlCanvasId) {
    return window.CreateWasmViewport(htmlCanvasId);
  },

  // each time the StoneApplication updates its status, it may signal it 
  // through this method. i.e, to change the status of a button in the web interface
  UpdateStoneApplicationStatusFromCppWithString: function(statusUpdateMessage) {
    var statusUpdateMessage_ = UTF8ToString(statusUpdateMessage);
    window.UpdateWebApplicationWithString(statusUpdateMessage_);
  },

  // same, but with a serialized message
  UpdateStoneApplicationStatusFromCppWithSerializedMessage: function(statusUpdateMessage) {
    var statusUpdateMessage_ = UTF8ToString(statusUpdateMessage);
    window.UpdateWebApplicationWithSerializedMessage(statusUpdateMessage_);
  },

  // These functions are called from C++ (through an extern declaration) 
  // and call the standard logger that, here, routes to the console.

  stone_console_error : function(message) {
    var text = UTF8ToString(message);
    window.errorFromCpp(text);
  },

  stone_console_warning : function(message) {
    var text = UTF8ToString(message);
    window.warningFromCpp(text);
  },

  stone_console_info: function(message) {
    var text = UTF8ToString(message);
    window.infoFromCpp(text);
  },
  
  stone_console_trace : function(message) {
    var text = UTF8ToString(message);
    window.debugFromCpp(text);
  }

});
