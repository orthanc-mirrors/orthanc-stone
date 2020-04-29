
// This object wraps the functions exposed by the wasm module

const WasmModuleWrapper = function() {
  this._InitializeViewport = undefined;
  this._LoadFromOrthanc = undefined;
};

WasmModuleWrapper.prototype.Setup = function(Module) {
  this._InitializeViewport = Module.cwrap('InitializeViewport', null, [ 'string' ]);
  this._LoadFromOrthanc = Module.cwrap('LoadFromOrthanc', null, [ 'string', 'int' ]);
};

WasmModuleWrapper.prototype.InitializeViewport = function(canvasId) {
  this._InitializeViewport(canvasId);
};

WasmModuleWrapper.prototype.LoadFromOrthanc = function(instance, frame) {
  this._LoadFromOrthanc(instance, frame);
};

var wasmModuleWrapper = new WasmModuleWrapper();

$(document).ready(function() {

  window.addEventListener('WasmModuleInitialized', function() {
    wasmModuleWrapper.Setup(Module);
    console.warn('Native C++ module initialized');

    wasmModuleWrapper.InitializeViewport('viewport');
  });

  window.addEventListener('StoneException', function() {
    alert('Exception caught in C++ code');
  });    

  var scriptSource;

  if ('WebAssembly' in window) {
    console.warn('Loading WebAssembly');
    scriptSource = 'SingleFrameViewerWasm.js';
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


$('#orthancLoad').click(function() {
  wasmModuleWrapper.LoadFromOrthanc($('#orthancInstance').val(),
                    $('#orthancFrame').val());
});
