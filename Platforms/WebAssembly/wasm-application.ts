///<reference path='stone-framework-loader.ts'/>

declare function InitializeWasmApplication() :void; // still in a js file




// Wait for the Orthanc Framework to be initialized (this initializes
// the WebAssembly environment)
Stone.Framework.Initialize(true, function() {
    InitializeWasmApplication();
});