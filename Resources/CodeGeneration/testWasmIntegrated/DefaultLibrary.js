// this file contains the JS method you want to expose to C++ code

mergeInto(LibraryManager.library, {
  // each time the Application updates its status, it may signal it through this method. i.e, to change the status of a button in the web interface
  // It needs to be put in this file so that the emscripten SDK linker knows where to find it.
  SendFreeTextFromCppJS: function(statusUpdateMessage) {
    var statusUpdateMessage_ = UTF8ToString(statusUpdateMessage);
    SendFreeTextFromCpp(statusUpdateMessage_);
  },
  SendMessageFromCppJS: function(statusUpdateMessage) {
    var statusUpdateMessage_ = UTF8ToString(statusUpdateMessage);
    SendMessageFromCpp(statusUpdateMessage_);
  }
});
