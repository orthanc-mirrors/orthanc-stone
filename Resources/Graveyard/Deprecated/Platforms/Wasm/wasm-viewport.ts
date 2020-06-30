import * as wasmApplicationRunner from './wasm-application-runner'
import * as Logger from './logger'

var isPendingRedraw = false;

function ScheduleWebViewportRedraw(cppViewportHandle: any) : void
{
  if (!isPendingRedraw) {
    isPendingRedraw = true;
    Logger.defaultLogger.debug('Scheduling a refresh of the viewport, as its content changed');
    window.requestAnimationFrame(function() {
      isPendingRedraw = false;
      let viewport = WasmViewport.GetFromCppViewport(cppViewportHandle);
      if (viewport) {
        viewport.Redraw();
      }
    });
  }
}

(<any>window).ScheduleWebViewportRedraw = ScheduleWebViewportRedraw;

declare function UTF8ToString(v: any): string;

function CreateWasmViewport(htmlCanvasId: string) : any {
  var cppViewportHandle = wasmApplicationRunner.CreateCppViewport();
  var canvasId = UTF8ToString(htmlCanvasId);
  var webViewport = new WasmViewport((<any> window).StoneFrameworkModule, canvasId, cppViewportHandle);  // viewports are stored in a static map in WasmViewport -> won't be deleted
  webViewport.Initialize();

  return cppViewportHandle;
}
 
(<any>window).CreateWasmViewport = CreateWasmViewport;

export class WasmViewport {

    private static viewportsMapByCppHandle_ : Map<number, WasmViewport> = new Map<number, WasmViewport>(); // key = the C++ handle
    private static viewportsMapByCanvasId_ : Map<string, WasmViewport> = new Map<string, WasmViewport>(); // key = the canvasId

    private module_ : any;
    private canvasId_ : string;
    private htmlCanvas_ : HTMLCanvasElement;
    private context_ : CanvasRenderingContext2D | null;
    private imageData_ : any = null;
    private renderingBuffer_ : any = null;
    
    private touchGestureInProgress_: boolean = false;
    private touchCount_: number = 0;
    private touchGestureLastCoordinates_: [number, number][] = []; // last x,y coordinates of each touch
    
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
    private ViewportTouchStart : Function;
    private ViewportTouchMove : Function;
    private ViewportTouchEnd : Function;

    private pimpl_ : any; // Private pointer to the underlying WebAssembly C++ object

    public constructor(module: any, canvasId: string, cppViewport: any) {
      
      this.pimpl_ = cppViewport;
      WasmViewport.viewportsMapByCppHandle_[this.pimpl_] = this;
      WasmViewport.viewportsMapByCanvasId_[canvasId] = this;

      this.module_ = module;
      this.canvasId_ = canvasId;
      this.htmlCanvas_ = document.getElementById(this.canvasId_) as HTMLCanvasElement;
      if (this.htmlCanvas_ == null) {
        Logger.defaultLogger.error("Can not create WasmViewport, did not find the canvas whose id is '", this.canvasId_, "'");
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
      this.ViewportTouchStart = this.module_.cwrap('ViewportTouchStart', null, [ 'number', 'number', 'number', 'number', 'number', 'number', 'number' ]);
      this.ViewportTouchMove = this.module_.cwrap('ViewportTouchMove', null, [ 'number', 'number', 'number', 'number', 'number', 'number', 'number' ]);
      this.ViewportTouchEnd = this.module_.cwrap('ViewportTouchEnd', null, [ 'number', 'number', 'number', 'number', 'number', 'number', 'number' ]);
    }

    public GetCppViewport() : number {
      return this.pimpl_;
    }

    public static GetFromCppViewport(cppViewportHandle: number) : WasmViewport | null {
      if (WasmViewport.viewportsMapByCppHandle_[cppViewportHandle] !== undefined) {
        return WasmViewport.viewportsMapByCppHandle_[cppViewportHandle];
      }
      Logger.defaultLogger.error("WasmViewport not found !");
      return null;
    }

    public static GetFromCanvasId(canvasId: string) : WasmViewport | null {
      if (WasmViewport.viewportsMapByCanvasId_[canvasId] !== undefined) {
        return WasmViewport.viewportsMapByCanvasId_[canvasId];
      }
      Logger.defaultLogger.error("WasmViewport not found !");
      return null;
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
        Logger.defaultLogger.error('The rendering has failed');
      } else {
        // Create an accessor to the rendering buffer (i.e. create a
        // "window" above the heap of the WASM module), then copy it to
        // the ImageData object
        this.imageData_.data.set(new Uint8ClampedArray(
          this.module_.HEAPU8.buffer,
          this.renderingBuffer_,
          this.imageData_.width * this.imageData_.height * 4));
        
        if (this.context_) {
          this.context_.putImageData(this.imageData_, 0, 0);
        }
      }
    }
  
    public Resize() {
      if (this.imageData_ != null &&
          (this.imageData_.width != window.innerWidth ||
           this.imageData_.height != window.innerHeight)) {
        this.imageData_ = null;
      }
      
      // width/height is defined by the parent width/height
      if (this.htmlCanvas_.parentElement) {
        this.htmlCanvas_.width = this.htmlCanvas_.parentElement.offsetWidth;  
        this.htmlCanvas_.height = this.htmlCanvas_.parentElement.offsetHeight;  

        Logger.defaultLogger.debug("resizing WasmViewport: ", this.htmlCanvas_.width, "x", this.htmlCanvas_.height);

        if (this.imageData_ === null && this.context_) {
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

       that.ViewportMouseDown(that.pimpl_, event.button, x, y, 0 /* TODO detect modifier keys*/);    
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
        var keyChar: string | null = event.key;
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
      }, {passive: false}); // must not be passive if calling event.preventDefault, ie to cancel scroll or zoom of the whole interface

      this.htmlCanvas_.addEventListener('touchstart', function(event: TouchEvent) {
        // don't propagate events to the whole body (this could zoom the entire page instead of zooming the viewport)
        event.preventDefault();
        event.stopPropagation();

        // TODO: find a way to pass the coordinates as an array between JS and C++
        var x0 = 0;
        var y0 = 0;
        var x1 = 0;
        var y1 = 0;
        var x2 = 0;
        var y2 = 0;
        if (event.targetTouches.length > 0) {
          x0 = event.targetTouches[0].pageX;
          y0 = event.targetTouches[0].pageY;
        }
        if (event.targetTouches.length > 1) {
          x1 = event.targetTouches[1].pageX;
          y1 = event.targetTouches[1].pageY;
        }
        if (event.targetTouches.length > 2) {
          x2 = event.targetTouches[2].pageX;
          y2 = event.targetTouches[2].pageY;
        }

        that.ViewportTouchStart(that.pimpl_, event.targetTouches.length, x0, y0, x1, y1, x2, y2);
      }, {passive: false}); // must not be passive if calling event.preventDefault, ie to cancel scroll or zoom of the whole interface
    
      this.htmlCanvas_.addEventListener('touchend', function(event) {
        // don't propagate events to the whole body (this could zoom the entire page instead of zooming the viewport)
        event.preventDefault();
        event.stopPropagation();

        // TODO: find a way to pass the coordinates as an array between JS and C++
        var x0 = 0;
        var y0 = 0;
        var x1 = 0;
        var y1 = 0;
        var x2 = 0;
        var y2 = 0;
        if (event.targetTouches.length > 0) {
          x0 = event.targetTouches[0].pageX;
          y0 = event.targetTouches[0].pageY;
        }
        if (event.targetTouches.length > 1) {
          x1 = event.targetTouches[1].pageX;
          y1 = event.targetTouches[1].pageY;
        }
        if (event.targetTouches.length > 2) {
          x2 = event.targetTouches[2].pageX;
          y2 = event.targetTouches[2].pageY;
        }

        that.ViewportTouchEnd(that.pimpl_, event.targetTouches.length, x0, y0, x1, y1, x2, y2);
      });
    
      this.htmlCanvas_.addEventListener('touchmove', function(event: TouchEvent) {

        // don't propagate events to the whole body (this could zoom the entire page instead of zooming the viewport)
        event.preventDefault();
        event.stopPropagation();


        // TODO: find a way to pass the coordinates as an array between JS and C++
        var x0 = 0;
        var y0 = 0;
        var x1 = 0;
        var y1 = 0;
        var x2 = 0;
        var y2 = 0;
        if (event.targetTouches.length > 0) {
          x0 = event.targetTouches[0].pageX;
          y0 = event.targetTouches[0].pageY;
        }
        if (event.targetTouches.length > 1) {
          x1 = event.targetTouches[1].pageX;
          y1 = event.targetTouches[1].pageY;
        }
        if (event.targetTouches.length > 2) {
          x2 = event.targetTouches[2].pageX;
          y2 = event.targetTouches[2].pageY;
        }

        that.ViewportTouchMove(that.pimpl_, event.targetTouches.length, x0, y0, x1, y1, x2, y2);
        return;

      }, {passive: false}); // must not be passive if calling event.preventDefault, ie to cancel scroll or zoom of the whole interface
    }  

  public ResetTouch() {
    if (this.touchTranslation_ ||
        this.touchZoom_) {
      this.ViewportMouseUp(this.pimpl_);
    }

    this.touchTranslation_ = false;
    this.touchZoom_ = false;
  }
  
  public GetTouchTranslation(event: any) {
    var touch = event.targetTouches[0];
    return [
      touch.pageX,
      touch.pageY
    ];
  }
    
  public GetTouchZoom(event: any) {
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
