///<reference path='stone-framework-loader.ts'/>
///<reference path='wasm-viewport.ts'/>

if (!('WebAssembly' in window)) {
  alert('Sorry, your browser does not support WebAssembly :(');
}

declare var StoneFrameworkModule : Stone.Framework;

// global functions
var WasmWebService_NotifyError: Function = null;
var WasmWebService_NotifySuccess: Function = null;
var WasmWebService_NewNotifyError: Function = null;
var WasmWebService_NewNotifySuccess: Function = null;
var WasmWebService_SetBaseUri: Function = null;
var NotifyUpdateContent: Function = null;
var SetStartupParameter: Function = null;
var CreateWasmApplication: Function = null;
var CreateCppViewport: Function = null;
var ReleaseCppViewport: Function = null;
var StartWasmApplication: Function = null;
var SendMessageToStoneApplication: Function = null;


function UpdateContentThread() {
  if (NotifyUpdateContent != null) {
    NotifyUpdateContent();
  }

  setTimeout(UpdateContentThread, 100);  // Update the viewport content every 100ms if need be
}


function GetUriParameters() {
  var parameters = window.location.search.substr(1);

  if (parameters != null &&
    parameters != '') {
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
  else {
    return {};
  }
}

// function UpdateWebApplication(statusUpdateMessage: string) {
//   console.log(statusUpdateMessage);
// }

function _InitializeWasmApplication(canvasId: string, orthancBaseUrl: string): void {

  /************************************** */
  CreateWasmApplication();
  WasmWebService_SetBaseUri(orthancBaseUrl);


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
    StartWasmApplication = StoneFrameworkModule.cwrap('StartWasmApplication', null, ['number']);

    WasmWebService_NotifySuccess = StoneFrameworkModule.cwrap('WasmWebService_NotifySuccess', null, ['number', 'string', 'array', 'number', 'number']);
    WasmWebService_NotifyError = StoneFrameworkModule.cwrap('WasmWebService_NotifyError', null, ['number', 'string', 'number']);
    WasmWebService_NewNotifySuccess = StoneFrameworkModule.cwrap('WasmWebService_NewNotifySuccess', null, ['number', 'string', 'array', 'number', 'number']);
    WasmWebService_NewNotifyError = StoneFrameworkModule.cwrap('WasmWebService_NewNotifyError', null, ['number', 'string', 'number']);
    WasmWebService_SetBaseUri = StoneFrameworkModule.cwrap('WasmWebService_SetBaseUri', null, ['string']);
    NotifyUpdateContent = StoneFrameworkModule.cwrap('NotifyUpdateContent', null, []);

    SendMessageToStoneApplication = StoneFrameworkModule.cwrap('SendMessageToStoneApplication', 'string', ['string']);

    console.log("Connecting C++ methods to JS methods - done");

    // Prevent scrolling
    document.body.addEventListener('touchmove', function (event) {
      event.preventDefault();
    }, false);

    _InitializeWasmApplication("canvas", orthancBaseUrl);
  });
}