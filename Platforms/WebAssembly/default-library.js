// this file contains the JS method you want to expose to C++ code

mergeInto(LibraryManager.library, {
  ScheduleWebViewportRedrawFromCpp: function(cppViewportHandle) {
    ScheduleWebViewportRedraw(cppViewportHandle);
  },
  CreateWasmViewportFromCpp: function(htmlCanvasId) {
    return CreateWasmViewport(htmlCanvasId);
  }
});
  