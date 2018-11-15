///<reference path='stone-framework-loader.ts'/>
///<reference path='wasm-viewport.ts'/>

if (!('WebAssembly' in window)) {
  alert('Sorry, your browser does not support WebAssembly :(');
}

declare var StoneFrameworkModule : Stone.Framework;

// global functions
var WasmWebService_NotifyError: Function = null;
var WasmWebService_NotifySuccess: Function = null;
var WasmDoAnimation: Function = null;
var SetStartupParameter: Function = null;
var CreateWasmApplication: Function = null;
var CreateCppViewport: Function = null;
var ReleaseCppViewport: Function = null;
var StartWasmApplication: Function = null;
var SendMessageToStoneApplication: Function = null;


function DoAnimationThread() {
  if (WasmDoAnimation != null) {
    WasmDoAnimation();
  }

  setTimeout(DoAnimationThread, 100);  // Update the viewport content every 100ms if need be
}


function GetUriParameters(): Map<string, string> {
  var parameters = window.location.search.substr(1);

  if (parameters != null &&
    parameters != '') {
    var result = new Map<string, string>();
    var tokens = parameters.split('&');

    for (var i = 0; i < tokens.length; i++) {
      var tmp = tokens[i].split('=');
      if (tmp.length == 2) {
        result[tmp[0]] = decodeURIComponent(tmp[1]);
      }
    }

    return result;
  }
  else {
    return new Map<string, string>();
  }
}

// function UpdateWebApplication(statusUpdateMessage: string) {
//   console.log(statusUpdateMessage);
// }

function _InitializeWasmApplication(orthancBaseUrl: string): void {

  CreateWasmApplication();

  // parse uri and transmit the parameters to the app before initializing it
  let parameters = GetUriParameters();

  for (let key in parameters) {
    if (parameters.hasOwnProperty(key)) {
      SetStartupParameter(key, parameters[key]);
    }
  }

  StartWasmApplication(orthancBaseUrl);

  // trigger a first resize of the canvas that has just been initialized
  Stone.WasmViewport.ResizeAll();

  DoAnimationThread();
}

function InitializeWasmApplication(wasmModuleName: string, orthancBaseUrl: string) {
  
  Stone.Framework.Configure(wasmModuleName);

  // Wait for the Orthanc Framework to be initialized (this initializes
  // the WebAssembly environment) and then, create and initialize the Wasm application
  Stone.Framework.Initialize(true, function () {

    console.log("Connecting C++ methods to JS methods");
    
    SetStartupParameter = StoneFrameworkModule.cwrap('SetStartupParameter', null, ['string', 'string']);
    CreateWasmApplication = StoneFrameworkModule.cwrap('CreateWasmApplication', null, ['number']);
    CreateCppViewport = StoneFrameworkModule.cwrap('CreateCppViewport', 'number', []);
    ReleaseCppViewport = StoneFrameworkModule.cwrap('ReleaseCppViewport', null, ['number']);
    StartWasmApplication = StoneFrameworkModule.cwrap('StartWasmApplication', null, ['string']);

    WasmWebService_NotifySuccess = StoneFrameworkModule.cwrap('WasmWebService_NotifySuccess', null, ['number', 'string', 'array', 'number', 'number']);
    WasmWebService_NotifyError = StoneFrameworkModule.cwrap('WasmWebService_NotifyError', null, ['number', 'string', 'number']);
    WasmDoAnimation = StoneFrameworkModule.cwrap('WasmDoAnimation', null, []);

    SendMessageToStoneApplication = StoneFrameworkModule.cwrap('SendMessageToStoneApplication', 'string', ['string']);

    console.log("Connecting C++ methods to JS methods - done");

    // Prevent scrolling
    document.body.addEventListener('touchmove', function (event) {
      event.preventDefault();
    }, false);

    _InitializeWasmApplication(orthancBaseUrl);
  });
}