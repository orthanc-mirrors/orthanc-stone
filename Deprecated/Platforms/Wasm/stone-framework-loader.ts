/**
 * This file contains primitives to interface with WebAssembly and
 * with the Stone framework.
 **/
import * as Logger from './logger'

export declare type InitializationCallback = () => void;

//export declare var StoneFrameworkModule : any;
export var StoneFrameworkModule : any;

//const ASSETS_FOLDER : string = "assets/lib";
//const WASM_FILENAME : string = "orthanc-framework";

export class Framework
{
  private static singleton_ : Framework = null;
  private static wasmModuleName_ : string = null;

  public static Configure(wasmModuleName: string) {
    this.wasmModuleName_ = wasmModuleName;
  }

  private constructor(verbose : boolean) 
  {
    //this.ccall('Initialize', null, [ 'number' ], [ verbose ]);
  }

  
  public ccall( name: string,
                returnType: string,
                argTypes: Array<string>,
                argValues: Array<any>) : any
  {
    return (<any> window).StoneFrameworkModule.ccall(name, returnType, argTypes, argValues);
  }

  
  public cwrap( name: string,
                returnType: string,
                argTypes: Array<string>) : any
  {
    return (<any> window).StoneFrameworkModule.cwrap(name, returnType, argTypes);
  }

  
  public static GetInstance() : Framework
  {
    if (Framework.singleton_ == null) {
      throw new Error('The WebAssembly module is not loaded yet');
    } else {
      return Framework.singleton_;
    }
  }


  public static Initialize( verbose: boolean,
                            callback: InitializationCallback)
  {
    Logger.defaultLogger.debug('Initializing WebAssembly Module');

    (<any> window).errorFromCpp = function(text:any) { Logger.defaultLogger.errorFromCpp(text); };
    (<any> window).warningFromCpp = function(text:any) { Logger.defaultLogger.warningFromCpp(text); };
    (<any> window).infoFromCpp = function(text:any) { Logger.defaultLogger.infoFromCpp(text); };
    (<any> window).debugFromCpp = function(text:any) { Logger.defaultLogger.debugFromCpp(text); };

    // (<any> window).
    (<any> window).StoneFrameworkModule = {
      preRun: [ 
        function() {
          Logger.defaultLogger.debug('Loading the Stone Framework using WebAssembly');
        }
      ],
      postRun: [ 
        function()  {
          // This function is called by ".js" wrapper once the ".wasm"
          // WebAssembly module has been loaded and compiled by the
          // browser
          Logger.defaultLogger.debug('WebAssembly is ready');
          Framework.singleton_ = new Framework(verbose);
          callback();
        }
      ],
      totalDependencies: 0
    };

    // Dynamic loading of the JavaScript wrapper around WebAssembly
    var script = document.createElement('script');
    script.type = 'application/javascript';
    //script.src = "orthanc-stone.js"; // ASSETS_FOLDER + '/' + WASM_FILENAME + '.js';
    script.src = this.wasmModuleName_ + ".js";//  "OrthancStoneSimpleViewer.js"; // ASSETS_FOLDER + '/' + WASM_FILENAME + '.js';
    script.async = true;
    document.head.appendChild(script);
  }
}