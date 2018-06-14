// http://stackoverflow.com/a/28900478/881731

function WebAssemblyViewport(module, canvasId) {
    this.module = module;
    
    // Obtain a reference to the canvas element using its id
    this.htmlCanvas = document.getElementById(canvasId);
  
    // Obtain a graphics context on the canvas element for drawing.
    this.context = htmlCanvas.getContext('2d');
  
    // ImageData structure that can be used to update the content of the canvas
    this.imageData = null;
  
    // Temporary buffer in the WebAssembly heap that is used to render the pixels
    this.renderingBuffer = null;
  
    // Get access to the WebAssembly functions
    this.ViewportSetSize = this.module.cwrap('ViewportSetSize', null, [ 'number', 'number' ]);
    this.ViewportRender = this.module.cwrap('ViewportRender', 'number', [ 'number', 'number', 'number' ]);
    this.ViewportMouseDown = this.module.cwrap('ViewportMouseDown', null, [ 'number', 'number', 'number', 'number' ]);
    this.ViewportMouseMove = this.module.cwrap('ViewportMouseMove', null, [ 'number', 'number' ]);
    this.ViewportMouseUp = this.module.cwrap('ViewportMouseUp', null, [ ]);
    this.ViewportMouseEnter = this.module.cwrap('ViewportMouseEnter', null, []);
    this.ViewportMouseLeave = this.module.cwrap('ViewportMouseLeave', null, []);
    this.ViewportMouseWheel = this.module.cwrap('ViewportMouseWheel', null, [ 'number', 'number', 'number', 'number' ]);
    this.ViewportKeyPressed = this.module.cwrap('ViewportKeyPressed', null, [ 'string', 'number', 'number' ]);
  
    this.Redraw = function() {
      if (this.imageData === null ||
          this.renderingBuffer === null ||
          ViewportRender(this.imageData.width,
                         this.imageData.height,
                         this.renderingBuffer) == 0) {
        console.log('The rendering has failed');
      } else {
        // Create an accessor to the rendering buffer (i.e. create a
        // "window" above the heap of the WASM module), then copy it to
        // the ImageData object
        this.imageData.data.set(new Uint8ClampedArray(
          this.module.buffer,
          this.renderingBuffer,
          this.imageData.width * this.imageData.height * 4));
        
        this.context.putImageData(imageData, 0, 0);
      }
    }
  
    this.Resize = function() {
      if (this.imageData != null &&
          (this.imageData.width != this.window.innerWidth ||
           this.imageData.height != this.window.innerHeight)) {
        this.imageData = null;
      }
      
      this.htmlCanvas.width = window.innerWidth;
      this.htmlCanvas.height = window.innerHeight;
  
      if (this.imageData === null) {
        this.imageData = context.getImageData(0, 0, this.htmlCanvas.width, this.htmlCanvas.height);
        ViewportSetSize(this.htmlCanvas.width, this.htmlCanvas.height);
  
        if (this.renderingBuffer != null) {
          this.module._free(this.renderingBuffer);
        }
        
        renderingBuffer = this.module._malloc(this.imageData.width * this.imageData.height * 4);
      }
      
      this.Redraw();
    }
  
    // Force the rendering of the viewport for the first time
    this.Resize();
  
    // Register an event listener to call the Resize() function 
    // each time the window is resized.
    window.addEventListener('resize', this.Resize, false);
  
    var that = this;
  
    this.htmlCanvas.addEventListener('contextmenu', function(event) {
      // Prevent right click on the canvas
      event.preventDefault();
    }, false);
    
    this.htmlCanvas.addEventListener('mouseleave', function(event) {
      that.ViewportMouseLeave();
    });
    
    this.htmlCanvas.addEventListener('mouseenter', function(event) {
      that.ViewportMouseEnter();
    });
  
    this.htmlCanvas.addEventListener('mousedown', function(event) {
      var x = event.pageX - this.offsetLeft;
      var y = event.pageY - this.offsetTop;
      that.ViewportMouseDown(event.button, x, y, 0 /* TODO */);    
    });
  
    this.htmlCanvas.addEventListener('mousemove', function(event) {
      var x = event.pageX - this.offsetLeft;
      var y = event.pageY - this.offsetTop;
      that.ViewportMouseMove(x, y);
    });
  
    this.htmlCanvas.addEventListener('mouseup', function(event) {
      that.ViewportMouseUp();
    });
  
    window.addEventListener('keydown', function(event) {
      that.ViewportKeyPressed(event.key, event.shiftKey, event.ctrlKey, event.altKey);
    });
  
    this.htmlCanvas.addEventListener('wheel', function(event) {
      var x = event.pageX - this.offsetLeft;
      var y = event.pageY - this.offsetTop;
      that.ViewportMouseWheel(event.deltaY, x, y, event.ctrlKey);
      event.preventDefault();
    });
  
  
    // Touch events
    this.touchTranslation = false;
    this.touchZoom = false;
  
    this.ResetTouch = function() {
      if (this.touchTranslation ||
          this.touchZoom) {
        this.ViewportMouseUp();
      }
  
      this.touchTranslation = false;
      this.touchZoom = false;
    }
  
    this.GetTouchTranslation = function(event) {
      var touch = event.targetTouches[0];
      return [
        touch.pageX,
        touch.pageY
      ];
    }
    
    this.GetTouchZoom = function(event) {
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
    
    this.htmlCanvas.addEventListener('touchstart', function(event) {
      ResetTouch();
    });
  
    this.htmlCanvas.addEventListener('touchend', function(event) {
      ResetTouch();
    });
    
    this.htmlCanvas.addEventListener('touchmove', function(event) {
      if (that.touchTranslation.length == 2) {
        var t = GetTouchTranslation(event);
        that.ViewportMouseMove(t[0], t[1]);
      }
      else if (that.touchZoom.length == 3) {
        var z0 = that.touchZoom;
        var z1 = GetTouchZoom(event);
        that.ViewportMouseMove(z0[0], z0[1] - z0[2] + z1[2]);
      }
      else {
        // Realize the gesture event
        if (event.targetTouches.length == 1) {
          // Exactly one finger inside the canvas => Setup a translation
          that.touchTranslation = GetTouchTranslation(event);
          that.ViewportMouseDown(1 /* middle button */,
                                 that.touchTranslation[0],
                                 that.touchTranslation[1], 0);
        } else if (event.targetTouches.length == 2) {
          // Exactly 2 fingers inside the canvas => Setup a pinch/zoom
          that.touchZoom = GetTouchZoom(event);
          var z0 = that.touchZoom;
          that.ViewportMouseDown(2 /* right button */,
                                 z0[0],
                                 z0[1], 0);
        }        
      }
    });
    
    return this;
  }
  