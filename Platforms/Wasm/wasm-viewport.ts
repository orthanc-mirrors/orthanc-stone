var isPendingRedraw = false;

function ScheduleWebViewportRedraw(cppViewportHandle: any) : void
{
  if (!isPendingRedraw) {
    isPendingRedraw = true;
    console.log('Scheduling a refresh of the viewport, as its content changed');
    window.requestAnimationFrame(function() {
      isPendingRedraw = false;
      Stone.WasmViewport.GetFromCppViewport(cppViewportHandle).Redraw();
    });
  }
}

declare function UTF8ToString(any): string;

function CreateWasmViewport(htmlCanvasId: string) : any {
  var cppViewportHandle = CreateCppViewport();
  var canvasId = UTF8ToString(htmlCanvasId);
  var webViewport = new Stone.WasmViewport(StoneFrameworkModule, canvasId, cppViewportHandle);  // viewports are stored in a static map in WasmViewport -> won't be deleted
  webViewport.Initialize();

  return cppViewportHandle;
}

module Stone {
  
//  export declare type InitializationCallback = () => void;
  
//  export declare var StoneFrameworkModule : any;
  
  //const ASSETS_FOLDER : string = "assets/lib";
  //const WASM_FILENAME : string = "orthanc-framework";

  export class WasmViewport {

    private static viewportsMapByCppHandle_ : Map<number, WasmViewport> = new Map<number, WasmViewport>(); // key = the C++ handle
    private static viewportsMapByCanvasId_ : Map<string, WasmViewport> = new Map<string, WasmViewport>(); // key = the canvasId

    private module_ : any;
    private canvasId_ : string;
    private htmlCanvas_ : HTMLCanvasElement;
    private context_ : CanvasRenderingContext2D;
    private imageData_ : any = null;
    private renderingBuffer_ : any = null;
    private touchZoom_ : any = false;
    private touchTranslation_ : any = false;

    private ViewportSetSize : Function;
    private ViewportRender : Function;
    private ViewportMouseDown : Function;
    private ViewportMouseMove : Function;
    private ViewportMouseUp : Function;
    private ViewportMouseEnter : Function;
    private ViewportMouseLeave : Function;
    private ViewportMouseWheel : Function;
    private ViewportKeyPressed : Function;

    private pimpl_ : any; // Private pointer to the underlying WebAssembly C++ object

    public constructor(module: any, canvasId: string, cppViewport: any) {
      
      this.pimpl_ = cppViewport;
      WasmViewport.viewportsMapByCppHandle_[this.pimpl_] = this;
      WasmViewport.viewportsMapByCanvasId_[canvasId] = this;

      this.module_ = module;
      this.canvasId_ = canvasId;
      this.htmlCanvas_ = document.getElementById(this.canvasId_) as HTMLCanvasElement;
      if (this.htmlCanvas_ == null) {
        console.log("Can not create WasmViewport, did not find the canvas whose id is '", this.canvasId_, "'");
      }
      this.context_ = this.htmlCanvas_.getContext('2d');

      this.ViewportSetSize = this.module_.cwrap('ViewportSetSize', null, [ 'number', 'number', 'number' ]);
      this.ViewportRender = this.module_.cwrap('ViewportRender', null, [ 'number', 'number', 'number', 'number' ]);
      this.ViewportMouseDown = this.module_.cwrap('ViewportMouseDown', null, [ 'number', 'number', 'number', 'number', 'number' ]);
      this.ViewportMouseMove = this.module_.cwrap('ViewportMouseMove', null, [ 'number', 'number', 'number' ]);
      this.ViewportMouseUp = this.module_.cwrap('ViewportMouseUp', null, [ 'number' ]);
      this.ViewportMouseEnter = this.module_.cwrap('ViewportMouseEnter', null, [ 'number' ]);
      this.ViewportMouseLeave = this.module_.cwrap('ViewportMouseLeave', null, [ 'number' ]);
      this.ViewportMouseWheel = this.module_.cwrap('ViewportMouseWheel', null, [ 'number', 'number', 'number', 'number', 'number' ]);
      this.ViewportKeyPressed = this.module_.cwrap('ViewportKeyPressed', null, [ 'number', 'number', 'string', 'number', 'number' ]);
    }

    public GetCppViewport() : number {
      return this.pimpl_;
    }

    public static GetFromCppViewport(cppViewportHandle: number) : WasmViewport {
      if (WasmViewport.viewportsMapByCppHandle_[cppViewportHandle] !== undefined) {
        return WasmViewport.viewportsMapByCppHandle_[cppViewportHandle];
      }
      console.log("WasmViewport not found !");
      return undefined;
    }

    public static GetFromCanvasId(canvasId: string) : WasmViewport {
      if (WasmViewport.viewportsMapByCanvasId_[canvasId] !== undefined) {
        return WasmViewport.viewportsMapByCanvasId_[canvasId];
      }
      console.log("WasmViewport not found !");
      return undefined;
    }

    public static ResizeAll() {
      for (let canvasId in WasmViewport.viewportsMapByCanvasId_) {
        WasmViewport.viewportsMapByCanvasId_[canvasId].Resize();
      }
    }

    public Redraw() {
      if (this.imageData_ === null ||
          this.renderingBuffer_ === null ||
          this.ViewportRender(this.pimpl_,
                         this.imageData_.width,
                         this.imageData_.height,
                         this.renderingBuffer_) == 0) {
        console.log('The rendering has failed');
      } else {
        // Create an accessor to the rendering buffer (i.e. create a
        // "window" above the heap of the WASM module), then copy it to
        // the ImageData object
        this.imageData_.data.set(new Uint8ClampedArray(
          this.module_.buffer,
          this.renderingBuffer_,
          this.imageData_.width * this.imageData_.height * 4));
        
        this.context_.putImageData(this.imageData_, 0, 0);
      }
    }
  
    public Resize() {
      if (this.imageData_ != null &&
          (this.imageData_.width != window.innerWidth ||
           this.imageData_.height != window.innerHeight)) {
        this.imageData_ = null;
      }
      
      // width/height can be defined in percent of window width/height through html attributes like data-width-ratio="50" and data-height-ratio="20"
      var widthRatio = Number(this.htmlCanvas_.dataset["widthRatio"]) || 100;
      var heightRatio = Number(this.htmlCanvas_.dataset["heightRatio"]) || 100;

      this.htmlCanvas_.width = window.innerWidth * (widthRatio / 100);  
      this.htmlCanvas_.height = window.innerHeight * (heightRatio / 100);

      console.log("resizing WasmViewport: ", this.htmlCanvas_.width, "x", this.htmlCanvas_.height);

      if (this.imageData_ === null) {
        this.imageData_ = this.context_.getImageData(0, 0, this.htmlCanvas_.width, this.htmlCanvas_.height);
        this.ViewportSetSize(this.pimpl_, this.htmlCanvas_.width, this.htmlCanvas_.height);
  
        if (this.renderingBuffer_ != null) {
          this.module_._free(this.renderingBuffer_);
        }
        
        this.renderingBuffer_ = this.module_._malloc(this.imageData_.width * this.imageData_.height * 4);
      } else {
        this.ViewportSetSize(this.pimpl_, this.htmlCanvas_.width, this.htmlCanvas_.height);
      }
      
      this.Redraw();
    }

    public Initialize() {
      
      // Force the rendering of the viewport for the first time
      this.Resize();
    
      var that : WasmViewport = this;
      // Register an event listener to call the Resize() function 
      // each time the window is resized.
      window.addEventListener('resize', function(event) {
        that.Resize();
      }, false);
  
      this.htmlCanvas_.addEventListener('contextmenu', function(event) {
        // Prevent right click on the canvas
        event.preventDefault();
      }, false);
      
      this.htmlCanvas_.addEventListener('mouseleave', function(event) {
        that.ViewportMouseLeave(that.pimpl_);
      });
      
      this.htmlCanvas_.addEventListener('mouseenter', function(event) {
        that.ViewportMouseEnter(that.pimpl_);
      });
    
      this.htmlCanvas_.addEventListener('mousedown', function(event) {
        var x = event.pageX - this.offsetLeft;
        var y = event.pageY - this.offsetTop;
        that.ViewportMouseDown(that.pimpl_, event.button, x, y, 0 /* TODO */);    
      });
    
      this.htmlCanvas_.addEventListener('mousemove', function(event) {
        var x = event.pageX - this.offsetLeft;
        var y = event.pageY - this.offsetTop;
        that.ViewportMouseMove(that.pimpl_, x, y);
      });
    
      this.htmlCanvas_.addEventListener('mouseup', function(event) {
        that.ViewportMouseUp(that.pimpl_);
      });
    
      window.addEventListener('keydown', function(event) {
        var keyChar = event.key;
        var keyCode = event.keyCode
        if (keyChar.length == 1) {
          keyCode = 0; // maps to OrthancStone::KeyboardKeys_Generic
        } else {
          keyChar = null;
        }
//        console.log("key: ", keyCode, keyChar);
        that.ViewportKeyPressed(that.pimpl_, keyCode, keyChar, event.shiftKey, event.ctrlKey, event.altKey);
      });
    
      this.htmlCanvas_.addEventListener('wheel', function(event) {
        var x = event.pageX - this.offsetLeft;
        var y = event.pageY - this.offsetTop;
        that.ViewportMouseWheel(that.pimpl_, event.deltaY, x, y, event.ctrlKey);
        event.preventDefault();
      });

      this.htmlCanvas_.addEventListener('touchstart', function(event) {
        that.ResetTouch();
      });
    
      this.htmlCanvas_.addEventListener('touchend', function(event) {
        that.ResetTouch();
      });
    
      this.htmlCanvas_.addEventListener('touchmove', function(event) {
        if (that.touchTranslation_.length == 2) {
          var t = that.GetTouchTranslation(event);
          that.ViewportMouseMove(that.pimpl_, t[0], t[1]);
        }
        else if (that.touchZoom_.length == 3) {
          var z0 = that.touchZoom_;
          var z1 = that.GetTouchZoom(event);
          that.ViewportMouseMove(that.pimpl_, z0[0], z0[1] - z0[2] + z1[2]);
        }
        else {
          // Realize the gesture event
          if (event.targetTouches.length == 1) {
            // Exactly one finger inside the canvas => Setup a translation
            that.touchTranslation_ = that.GetTouchTranslation(event);
            that.ViewportMouseDown(that.pimpl_, 
                                  1 /* middle button */,
                                  that.touchTranslation_[0],
                                  that.touchTranslation_[1], 0);
          } else if (event.targetTouches.length == 2) {
            // Exactly 2 fingers inside the canvas => Setup a pinch/zoom
            that.touchZoom_ = that.GetTouchZoom(event);
            var z0 = that.touchZoom_;
            that.ViewportMouseDown(that.pimpl_, 
                                  2 /* right button */,
                                  z0[0],
                                  z0[1], 0);
          }        
        }
      });
    }  

  public ResetTouch() {
    if (this.touchTranslation_ ||
        this.touchZoom_) {
      this.ViewportMouseUp(this.pimpl_);
    }

    this.touchTranslation_ = false;
    this.touchZoom_ = false;
  }
  
  public GetTouchTranslation(event) {
    var touch = event.targetTouches[0];
    return [
      touch.pageX,
      touch.pageY
    ];
  }
    
  public GetTouchZoom(event) {
    var touch1 = event.targetTouches[0];
    var touch2 = event.targetTouches[1];
    var dx = (touch1.pageX - touch2.pageX);
    var dy = (touch1.pageY - touch2.pageY);
    var d = Math.sqrt(dx * dx + dy * dy);
    return [
      (touch1.pageX + touch2.pageX) / 2.0,
      (touch1.pageY + touch2.pageY) / 2.0,
      d
    ];
  }
    
}
}
  