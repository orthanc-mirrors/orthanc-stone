

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
  viewport = new Stone.WasmViewport(StoneFrameworkModule, 'canvas');
  viewport.Initialize();

  /******************** */
  SetStartupParameter = StoneFrameworkModule.cwrap('SetStartupParameter', null, [ 'string', 'string' ]);
  CreateWasmApplication = StoneFrameworkModule.cwrap('CreateWasmApplication', null, [ ], [ ]);
  StartWasmApplication = StoneFrameworkModule.cwrap('StartWasmApplication', null, [ ], [ ]);
                                 
  /******************** */

  // NotifyGlobalParameter = StoneFrameworkModule.cwrap('NotifyGlobalParameter', null,
  //                                      [ 'string', 'string' ]);
  // ViewportUpdate = StoneFrameworkModule.cwrap('ViewportUpdate', null,
  //                                      [ 'string' ]);
  WasmWebService_NotifySuccess = StoneFrameworkModule.cwrap('WasmWebService_NotifySuccess', null,
                                              [ 'number', 'string', 'array', 'number', 'number' ]);
  WasmWebService_NotifyError = StoneFrameworkModule.cwrap('WasmWebService_NotifyError', null,
                                            [ 'number', 'string', 'number' ]);
  //NotifyRestApiGet = Module.cwrap('NotifyRestApiGet', null, [ 'number', 'array', 'number' ]);
  NotifyUpdateContent = StoneFrameworkModule.cwrap('NotifyUpdateContent', null, [  ]);

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
