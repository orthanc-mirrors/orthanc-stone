// this file contains the JS method you want to expose to C++ code

mergeInto(LibraryManager.library, {
  // each time the Application updates its status, it may signal it through this method. i.e, to change the status of a button in the web interface
  // It needs to be put in this file so that the emscripten SDK linker knows where to find it.
  UpdateApplicationStatusFromCpp: function(statusUpdateMessage) {
    var statusUpdateMessage_ = UTF8ToString(statusUpdateMessage);
    UpdateWebApplication(statusUpdateMessage_);
  }
});
  