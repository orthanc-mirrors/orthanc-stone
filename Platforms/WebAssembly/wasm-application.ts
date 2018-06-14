///<reference path='stone-framework-loader.ts'/>
///<reference path='wasm-viewport.ts'/>

module Stone {
  
    //  export declare type InitializationCallback = () => void;
      
    //  export declare var StoneFrameworkModule : any;
      
      //const ASSETS_FOLDER : string = "assets/lib";
      //const WASM_FILENAME : string = "orthanc-framework";
      
      export class WasmApplication {
    
        private viewport_ : WasmViewport;
        private canvasId_: string;
    
        private pimpl_ : any; // Private pointer to the underlying WebAssembly C++ object
    
        public constructor(canvasId: string) {
          this.canvasId_ = canvasId;
            //this.module_ = module;
        }
    }
}
    

declare function InitializeWasmApplication(canvasId: string) :void; // still in a js file




// Wait for the Orthanc Framework to be initialized (this initializes
// the WebAssembly environment) and then, create and initialize the Wasm application
Stone.Framework.Initialize(true, function() {
    InitializeWasmApplication("canvas");
});