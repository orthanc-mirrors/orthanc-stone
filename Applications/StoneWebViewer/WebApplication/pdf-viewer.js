/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/




// Loaded via <script> tag, create shortcut to access PDF.js exports.
var pdfjsLib = window['pdfjs-dist/build/pdf'];

// The workerSrc property shall be specified.
pdfjsLib.GlobalWorkerOptions.workerSrc = 'https://cdn.jsdelivr.net/npm/pdfjs-dist@2.5.207/build/pdf.worker.min.js';



var ZOOM_FACTOR = 1.3;
var FIT_MARGIN = 10;

Vue.component('pdf-viewer', {
  props: [ 'prefix', 'pdf' ],  // "pdf" must correspond to a "Uint8Array"
  template: '#pdf-viewer',
  data: function() {
    return {
      container: null,
      canvas: null,
      ctx: null,
      
      scale: 1,
      countPages: 0,
      currentPage: 0,
      pdfDoc: null,
      isRendering: false,
      pageNumPending: null
    }
  },
  watch: {
    pdf: function(newVal, oldVal) {
      this.LoadPdf();
    }
  },
  mounted: function() {
    this.container = document.getElementById(this.prefix + '-container');
    this.canvas = document.getElementById(this.prefix + '-canvas');
    this.ctx = this.canvas.getContext('2d');

    if (this.container === null ||
        this.canvas === null ||
        this.ctx === null) {
      alert('Bad viewer configuration');
    }

    var that = this;
    this.container.addEventListener('wheel', function(event) {
      that.MouseWheel(event);
    });
  },
  methods: {
    NextPage: function() {
      if (this.pdfDoc !== null &&
          this.currentPage < this.pdfDoc.numPages) {
        this.QueueRenderPage(this.currentPage + 1);
      }
    },
    PreviousPage: function() {
      if (this.pdfDoc !== null &&
          this.currentPage > 1) {
        this.QueueRenderPage(this.currentPage - 1);
      }
    },
    FitWidth: function() {
      if (this.pdfDoc !== null) {
        var that = this;
        this.pdfDoc.getPage(this.currentPage).then(function(page) {
          // https://github.com/mozilla/pdf.js/issues/5628
          // https://stackoverflow.com/a/21064102/881731
          // https://stackoverflow.com/a/60008044/881731
          var scrollbarWidth = window.innerWidth - document.body.clientWidth + FIT_MARGIN;
          that.scale = (that.container.offsetWidth - scrollbarWidth) / page.getViewport({ scale: 1.0 }).width;
          that.QueueRenderPage(that.currentPage);
        });
      }
    },
    FitHeight: function() {
      if (this.pdfDoc !== null) {
        var that = this;
        this.pdfDoc.getPage(this.currentPage).then(function(page) {
          // The computation below assumes that "line-height: 0px" CSS
          // on the parent element of the canvas.
          
          // https://github.com/mozilla/pdf.js/issues/5628
          var scrollbarHeight = window.innerHeight - document.body.clientHeight + FIT_MARGIN;
          that.scale = (that.container.offsetHeight - scrollbarHeight) / page.getViewport({ scale: 1.0 }).height;
          //that.scale = that.container.clientHeight / page.getViewport({ scale: 1.0 }).height;
          that.QueueRenderPage(that.currentPage);
        });
      }
    },
    ZoomIn: function() {
      this.scale *= ZOOM_FACTOR;
      this.QueueRenderPage(this.currentPage);  
    },
    ZoomOut: function() {
      this.scale /= ZOOM_FACTOR;
      this.QueueRenderPage(this.currentPage);  
    },
    LoadPdf: function(pdf) {
      var that = this;
      pdfjsLib.getDocument(new Uint8Array(this.pdf)).promise.then(function(pdfDoc_) {
        that.pdfDoc = pdfDoc_;
        that.currentPage = 0;
        that.countPages = pdfDoc_.numPages;
        that.scale = 1;
        that.isRendering = false;
        that.pageNumPending = null;
        
        // Initial/first page rendering
        that.RenderPage(1);
      });
    },
    RenderPage: function(pageNum) {
      var that = this;
      
      if (this.pdfDoc !== null &&
          pageNum >= 1 &&
          pageNum <= this.countPages) {
        this.isRendering = true;
        this.pdfDoc.getPage(pageNum).then(function(page) {
          var viewport = page.getViewport({scale: that.scale});

          that.canvas.height = viewport.height;
          that.canvas.width = viewport.width;
          
          // Horizontal centering of the canvas. This requires CSS
          // "position: relative" on the canvas element.
          if (that.canvas.width < that.container.clientWidth) {
            that.canvas.style.left = Math.floor((that.container.clientWidth - viewport.width) / 2) + 'px';
          } else {
            that.canvas.style.left = '0px';
          }

          // Render PDF page into canvas context
          var renderContext = {
            canvasContext: that.ctx,
            viewport: viewport
          };
          
          var renderTask = page.render(renderContext);

          // Wait for rendering to finish
          renderTask.promise.then(function() {
            that.isRendering = false;
            that.currentPage = pageNum;
            if (that.pageNumPending !== null) {
              // New page rendering is pending
              that.currentPage = that.pageNumPending;
              that.pageNumPending = null;
              that.RenderPage();
            }
          });
        });
      }
    },
    QueueRenderPage: function(pageNum) {
      if (this.isRendering) {
        this.pageNumPending = pageNum;
      } else {
        this.RenderPage(pageNum);
      }
    },
    MouseWheel: function(event) {
      if (event.ctrlKey) {
        if (event.deltaY < 0) {
          this.ZoomIn();
        } else if (event.deltaY > 0) {
          this.ZoomOut();
        }
        
        event.preventDefault();
      }
    }
  }
});
