mergeInto(LibraryManager.library, {
  WasmDelayedCallExecutor_Schedule: function(callable, timeoutInMs/*, payload*/) {
    setTimeout(function() {
      window.WasmDelayedCallExecutor_ExecuteCallback(callable/*, payload*/);
    }, timeoutInMs);
  }
});
