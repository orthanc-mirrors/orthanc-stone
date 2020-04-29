
// This object wraps the functions exposed by the wasm module

const WasmModuleWrapper = function() {
  this._InitializeViewport = undefined;
};

WasmModuleWrapper.prototype.Setup = function(Module) {
  this._SetArgument = Module.cwrap('SetArgument', null, [ 'string', 'string' ]);
  this._Initialize = Module.cwrap('Initialize', null, [ 'string' ]);
};

WasmModuleWrapper.prototype.SetArgument = function(key, value) {
  this._SetArgument(key, value);
};

WasmModuleWrapper.prototype.Initialize = function(canvasId) {
  this._Initialize(canvasId);
};

var wasmModuleWrapper = new WasmModuleWrapper();

$(document).ready(function() {

  window.addEventListener('WasmModuleInitialized', function() {

    // bind the C++ global functions
    wasmModuleWrapper.Setup(Module);

    console.warn('Native C++ module initialized');

    // Loop over the GET arguments
    var parameters = window.location.search.substr(1);
    if (parameters != null && parameters != '') {
      var tokens = parameters.split('&');
      for (var i = 0; i < tokens.length; i++) {
        var arg = tokens[i].split('=');
        if (arg.length == 2) {
          // Send each GET argument to WebAssembly
          wasmModuleWrapper.SetArgument(arg[0], decodeURIComponent(arg[1]));
        }
      }
    }
    wasmModuleWrapper.Initialize();
  });

  window.addEventListener('StoneException', function() {
    alert('Exception caught in C++ code');
  });    

  var scriptSource;

  if ('WebAssembly' in window) {
    console.warn('Loading WebAssembly');
    scriptSource = 'RtViewerWasm.js';
  } else {
    console.error('Your browser does not support WebAssembly!');
  }

  // Option 1: Loading script using plain HTML
  
  /*
    var script = document.createElement('script');
    script.src = scriptSource;
    script.type = 'text/javascript';
    document.body.appendChild(script);
  */

  // Option 2: Loading script using AJAX (gives the opportunity to
  // report explicit errors)
  
  axios.get(scriptSource)
    .then(function (response) {
      var script = document.createElement('script');
      script.innerHTML = response.data;
      script.type = 'text/javascript';
      document.body.appendChild(script);
    })
    .catch(function (error) {
      alert('Cannot load the WebAssembly framework');
    });
});

// http://localhost:9979/rtviewer/index.html?loglevel=trace&ctseries=CTSERIES&rtdose=RTDOSE&rtstruct=RTSTRUCT

