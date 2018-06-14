

// Global context used by "library.js"
var viewport = null;
var WasmWebService_NotifyError = null;
var WasmWebService_NotifySuccess = null;
//var NotifyRestApiGet = null;
var NotifyUpdateContent = null;

function UpdateContentThread()
{
  if (NotifyUpdateContent != null) {
    NotifyUpdateContent();
  }
  
  setTimeout(UpdateContentThread, 100);  // Update the viewport content every 100ms if need be
}


function GetUriParameters()
{
  var parameters = window.location.search.substr(1);

  if (parameters != null &&
      parameters != '')
  {
    var result = {};
    var tokens = parameters.split('&');

    for (var i = 0; i < tokens.length; i++) {
      var tmp = tokens[i].split('=');
      if (tmp.length == 2) {
        result[tmp[0]] = decodeURIComponent(tmp[1]);
      }
    }
    
    return result;
  }
  else
  {
    return {};
  }
}



function InitializeWasmApplication(canvasId)
{
  console.log("Initializing wasm-app");

  console.log("Connecting C++ methods to JS methods");
  SetStartupParameter = StoneFrameworkModule.cwrap('SetStartupParameter', null, [ 'string', 'string' ]);
  CreateWasmApplication = StoneFrameworkModule.cwrap('CreateWasmApplication', null, [ 'any' ], [ ]);
  CreateCppViewport = StoneFrameworkModule.cwrap('CreateCppViewport', 'any', [ ], [ ]);
  ReleaseCppViewport = StoneFrameworkModule.cwrap('ReleaseCppViewport', null, [ 'any' ], [ ]);
  StartWasmApplication = StoneFrameworkModule.cwrap('StartWasmApplication', null, [ ], [ ]);
  WasmWebService_NotifySuccess = StoneFrameworkModule.cwrap('WasmWebService_NotifySuccess', null,
                                              [ 'number', 'string', 'array', 'number', 'number' ]);
  WasmWebService_NotifyError = StoneFrameworkModule.cwrap('WasmWebService_NotifyError', null,
                                            [ 'number', 'string', 'number' ]);
  NotifyUpdateContent = StoneFrameworkModule.cwrap('NotifyUpdateContent', null, [  ]);
                                 
  console.log("Creating main viewport");

  viewport = new Stone.WasmViewport(StoneFrameworkModule, 'canvas');
  viewport.Initialize(CreateCppViewport());

  // Prevent scrolling
  document.body.addEventListener('touchmove', function(event) {
    event.preventDefault();
  }, false);
  
  document.getElementById('canvas').onclick = function() {
    viewport.Redraw();
  };


  /************************************** */
  CreateWasmApplication();

  // parse uri and transmit the parameters to the app before initializing it
  var parameters = GetUriParameters();

  for (var key in parameters) {
    if (parameters.hasOwnProperty(key)) {  
      SetStartupParameter(key, parameters[key]);
    }
  }

  StartWasmApplication();
  /************************************** */

  UpdateContentThread();
}

if (!('WebAssembly' in window)) {
  alert('Sorry, your browser does not support WebAssembly :(');
} else {
  
}
