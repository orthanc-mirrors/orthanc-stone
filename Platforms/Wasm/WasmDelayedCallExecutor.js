mergeInto(LibraryManager.library, {
  WasmDelayedCallExecutor_Schedule: function(callable, timeoutInMs/*, payload*/) {
    setTimeout(function() {
      WasmDelayedCallExecutor_ExecuteCallback(callable/*, payload*/);
    }, timeoutInMs);
  }
});
