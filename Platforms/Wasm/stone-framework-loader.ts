module Stone {
    /**
     * This file contains primitives to interface with WebAssembly and
     * with the Stone framework.
     **/
    
    export declare type InitializationCallback = () => void;
    
    export declare var StoneFrameworkModule : any;
    
    //const ASSETS_FOLDER : string = "assets/lib";
    //const WASM_FILENAME : string = "orthanc-framework";
    
    
    export class Framework
    {
      private static singleton_ : Framework = null;
    
      private constructor(verbose : boolean) 
      {
        //this.ccall('Initialize', null, [ 'number' ], [ verbose ]);
      }
    
      
      public ccall(name: string,
                   returnType: string,
                   argTypes: Array<string>,
                   argValues: Array<any>) : any
      {
        return StoneFrameworkModule.ccall(name, returnType, argTypes, argValues);
      }
    
      
      public cwrap(name: string,
                   returnType: string,
                   argTypes: Array<string>) : any
      {
        return StoneFrameworkModule.cwrap(name, returnType, argTypes);
      }
    
      
      public static GetInstance() : Framework
      {
        if (Framework.singleton_ == null) {
          throw new Error('The WebAssembly module is not loaded yet');
        } else {
          return Framework.singleton_;
        }
      }
      
    
      public static Initialize(verbose: boolean,
                               callback: InitializationCallback)
      {
        console.log('Initializing WebAssembly');
    
        (<any> window).StoneFrameworkModule = {
          preRun: [ 
            function() {
              console.log('Loading the Stone Framework using WebAssembly');
            }
          ],
          postRun: [ 
            function()  {
              // This function is called by ".js" wrapper once the ".wasm"
              // WebAssembly module has been loaded and compiled by the
              // browser
              console.log('WebAssembly is ready');
              Framework.singleton_ = new Framework(verbose);
              callback();
            }
          ],
          print: function(text : string) {
            console.log(text);
          },
          printErr: function(text : string) {
            console.error(text);
          },
          totalDependencies: 0
        };
    
        // Dynamic loading of the JavaScript wrapper around WebAssembly
        var script = document.createElement('script');
        script.type = 'application/javascript';
        //script.src = "orthanc-stone.js"; // ASSETS_FOLDER + '/' + WASM_FILENAME + '.js';
        script.src = "OrthancStoneSimpleViewer.js"; // ASSETS_FOLDER + '/' + WASM_FILENAME + '.js';
        script.async = true;
        document.head.appendChild(script);
      }
    }
}