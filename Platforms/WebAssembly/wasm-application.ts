///<reference path='stone-framework-loader.ts'/>
///<reference path='wasm-viewport.ts'/>

if (!('WebAssembly' in window)) {
    alert('Sorry, your browser does not support WebAssembly :(');
}

declare var StoneFrameworkModule : Stone.Framework;

// global functions
var WasmWebService_NotifyError: Function = null;
var WasmWebService_NotifySuccess: Function = null;
var NotifyUpdateContent: Function = null;
var SetStartupParameter: Function = null;
var CreateWasmApplication: Function = null;
var CreateCppViewport: Function = null;
var ReleaseCppViewport: Function = null;
var StartWasmApplication: Function = null;


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

module Stone {

    //  export declare type InitializationCallback = () => void;

    //  export declare var StoneFrameworkModule : any;

    //const ASSETS_FOLDER : string = "assets/lib";
    //const WASM_FILENAME : string = "orthanc-framework";

    export class WasmApplication {

        private viewport_: WasmViewport;
        private canvasId_: string;

        private pimpl_: any; // Private pointer to the underlying WebAssembly C++ object

        public constructor(canvasId: string) {
            this.canvasId_ = canvasId;
            //this.module_ = module;
        }
    }
}


function InitializeWasmApplication(canvasId: string): void {

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

// Wait for the Orthanc Framework to be initialized (this initializes
// the WebAssembly environment) and then, create and initialize the Wasm application
Stone.Framework.Initialize(true, function () {

    console.log("Connecting C++ methods to JS methods");
    
    SetStartupParameter = StoneFrameworkModule.cwrap('SetStartupParameter', null, ['string', 'string']);
    CreateWasmApplication = StoneFrameworkModule.cwrap('CreateWasmApplication', null, ['number']);
    CreateCppViewport = StoneFrameworkModule.cwrap('CreateCppViewport', 'number', []);
    ReleaseCppViewport = StoneFrameworkModule.cwrap('ReleaseCppViewport', null, ['number']);
    StartWasmApplication = StoneFrameworkModule.cwrap('StartWasmApplication', null, []);

    WasmWebService_NotifySuccess = StoneFrameworkModule.cwrap('WasmWebService_NotifySuccess', null, ['number', 'string', 'array', 'number', 'number']);
    WasmWebService_NotifyError = StoneFrameworkModule.cwrap('WasmWebService_NotifyError', null, ['number', 'string', 'number']);
    NotifyUpdateContent = StoneFrameworkModule.cwrap('NotifyUpdateContent', null, []);

    // Prevent scrolling
    document.body.addEventListener('touchmove', function (event) {
        event.preventDefault();
    }, false);


    InitializeWasmApplication("canvas");
});