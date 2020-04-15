
// This object wraps the functions exposed by the wasm module

const WasmModuleWrapper = function() {
  this._InitializeViewport = undefined;
  this._LoadOrthanc = undefined;
  this._LoadDicomWeb = undefined;
};

WasmModuleWrapper.prototype.Setup = function(Module) {
  this._InitializeViewport = Module.cwrap('InitializeViewport', null, [ 'string' ]);
  this._LoadOrthanc = Module.cwrap('LoadOrthanc', null, [ 'string', 'int' ]);
  this._LoadDicomWeb = Module.cwrap('LoadDicomWeb', null, [ 'string', 'string', 'string', 'string', 'int' ]);
};

WasmModuleWrapper.prototype.InitializeViewport = function(canvasId) {
  this._InitializeViewport(canvasId);
};

WasmModuleWrapper.prototype.LoadOrthanc = function(instance, frame) {
  this._LoadOrthanc(instance, frame);
};

WasmModuleWrapper.prototype.LoadDicomWeb = function(server, studyInstanceUid, seriesInstanceUid, sopInstanceUid, frame) {
  this._LoadDicomWeb(server, studyInstanceUid, seriesInstanceUid, sopInstanceUid, frame);
};

var moduleWrapper = new WasmModuleWrapper();

$(document).ready(function() {

  window.addEventListener('StoneInitialized', function() {
    stone.Setup(Module);
    console.warn('Native Stone properly intialized');

    stone.InitializeViewport('viewport');
  });

  window.addEventListener('StoneException', function() {
    alert('Exception caught in Stone');
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
  stone.LoadOrthanc($('#orthancInstance').val(),
                    $('#orthancFrame').val());
});


$('#dicomWebLoad').click(function() {
  stone.LoadDicomWeb($('#dicomWebServer').val(),
                     $('#dicomWebStudy').val(),
                     $('#dicomWebSeries').val(),
                     $('#dicomWebInstance').val(),
                     $('#dicomWebFrame').val());
});
