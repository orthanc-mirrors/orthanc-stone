import Stone = require('./stone-framework-loader');
import StoneViewport = require('./wasm-viewport');
import * as Logger from './logger'

if (!('WebAssembly' in window)) {
  alert('Sorry, your browser does not support WebAssembly :(');
}

//var StoneFrameworkModule : Stone.Framework = (<any>window).StoneFrameworkModule;
//export declare var StoneFrameworkModule : Stone.Framework;

// global functions
var WasmWebService_NotifyError: Function = null;
var WasmWebService_NotifySuccess: Function = null;
var WasmWebService_NotifyCachedSuccess: Function = null;
var WasmDelayedCallExecutor_ExecuteCallback: Function = null;
var WasmDoAnimation: Function = null;
var SetStartupParameter: Function = null;
var CreateWasmApplication: Function = null;
export var CreateCppViewport: Function = null;
var ReleaseCppViewport: Function = null;
var StartWasmApplication: Function = null;
export var SendSerializedMessageToStoneApplication: Function = null;

var auxiliaryParameters : Map<string,string>  = null;

export function SetApplicationParameters(params : Map<string,string>) {
  if (auxiliaryParameters != null) {
    console.warn("wasm-application-runner.SetApplicationParameters: about to overwrite the existing application parameters!")
  }
  auxiliaryParameters = params;
}

function DoAnimationThread() {
  if (WasmDoAnimation != null) {
    WasmDoAnimation();
  }

  // Update the viewport content every 100ms if need be
  setTimeout(DoAnimationThread, 100);  
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
      } else if(tmp.length == 1) {
        // if there is no '=', we treat ot afterwards as a flag-style param
        result[tmp[0]] = "";
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

  // transmit the API-specified parameters to the app before initializing it
  for (let key in auxiliaryParameters) {
    if (auxiliaryParameters.hasOwnProperty(key)) {
      Logger.defaultLogger.debug(
        `About to call SetStartupParameter("${key}","${auxiliaryParameters[key]}")`);
      SetStartupParameter(key, auxiliaryParameters[key]);
    }
  }

  // parse uri and transmit the URI parameters to the app before initializing it
  let parameters = GetUriParameters();

  for (let key in parameters) {
    if (parameters.hasOwnProperty(key)) {
      Logger.defaultLogger.debug(
        `About to call SetStartupParameter("${key}","${parameters[key]}")`);
      SetStartupParameter(key, parameters[key]);
    }
  }

  StartWasmApplication(orthancBaseUrl);

  // trigger a first resize of the canvas that has just been initialized
  StoneViewport.WasmViewport.ResizeAll();

  DoAnimationThread();
}

export function InitializeWasmApplication(wasmModuleName: string, orthancBaseUrl: string) {
  
  Stone.Framework.Configure(wasmModuleName);

  // Wait for the Orthanc Framework to be initialized (this initializes
  // the WebAssembly environment) and then, create and initialize the Wasm application
  Stone.Framework.Initialize(true, function () {

    Logger.defaultLogger.debug("Connecting C++ methods to JS methods");
    
    SetStartupParameter = (<any> window).StoneFrameworkModule.cwrap('SetStartupParameter', null, ['string', 'string']);
    CreateWasmApplication = (<any> window).StoneFrameworkModule.cwrap('CreateWasmApplication', null, ['number']);
    CreateCppViewport = (<any> window).StoneFrameworkModule.cwrap('CreateCppViewport', 'number', []);
    ReleaseCppViewport = (<any> window).StoneFrameworkModule.cwrap('ReleaseCppViewport', null, ['number']);
    StartWasmApplication = (<any> window).StoneFrameworkModule.cwrap('StartWasmApplication', null, ['string']);
    (<any> window).IsTraceLevelEnabled = (<any> window).StoneFrameworkModule.cwrap('WasmIsTraceLevelEnabled', 'boolean', null);
    (<any> window).IsInfoLevelEnabled = (<any> window).StoneFrameworkModule.cwrap('WasmIsInfoLevelEnabled', 'boolean', null);

    (<any> window).WasmWebService_NotifyCachedSuccess = (<any> window).StoneFrameworkModule.cwrap('WasmWebService_NotifyCachedSuccess', null, ['number']);
    (<any> window).WasmWebService_NotifySuccess = (<any> window).StoneFrameworkModule.cwrap('WasmWebService_NotifySuccess', null, ['number', 'string', 'array', 'number', 'number']);
    (<any> window).WasmWebService_NotifyError = (<any> window).StoneFrameworkModule.cwrap('WasmWebService_NotifyError', null, ['number', 'string', 'number']);
    (<any> window).WasmDelayedCallExecutor_ExecuteCallback = (<any> window).StoneFrameworkModule.cwrap('WasmDelayedCallExecutor_ExecuteCallback', null, ['number']);
    // no need to put this into the globals for it's only used in this very module
    WasmDoAnimation = (<any> window).StoneFrameworkModule.cwrap('WasmDoAnimation', null, []);

    SendSerializedMessageToStoneApplication = (<any> window).StoneFrameworkModule.cwrap('SendSerializedMessageToStoneApplication', 'string', ['string']);

    Logger.defaultLogger.debug("Connecting C++ methods to JS methods - done");

    // Prevent scrolling
    document.body.addEventListener('touchmove', function (event) {
      event.preventDefault();
    }, { passive: false}); // must not be passive if calling event.preventDefault, ie to cancel scroll or zoom of the whole interface

    _InitializeWasmApplication(orthancBaseUrl);
  });
}


// exports.InitializeWasmApplication = InitializeWasmApplication;



