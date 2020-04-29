/**
 * This is a generic bootstrap code that is shared by all the Stone
 * sample applications.
 **/

// Check support for WebAssembly
if (!('WebAssembly' in window)) {
  alert('Sorry, your browser does not support WebAssembly :(');
} else {

  // Wait for the module to be loaded (the event "WebAssemblyLoaded"
  // must be emitted by the "main" function)
  window.addEventListener('WebAssemblyLoaded', function() {

    // Loop over the GET arguments
    var parameters = window.location.search.substr(1);
    if (parameters != null && parameters != '') {
      var tokens = parameters.split('&');
      for (var i = 0; i < tokens.length; i++) {
        var arg = tokens[i].split('=');
        if (arg.length == 2) {

          // Send each GET argument to WebAssembly
          Module.ccall('SetArgument', null, [ 'string', 'string' ],
                       [ arg[0], decodeURIComponent(arg[1]) ]);
        }
      }
    }

    // Inform the WebAssembly module that it can start
    Module.ccall('Initialize', null, null, null);
  });
}
