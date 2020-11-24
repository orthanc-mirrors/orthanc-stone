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


var COLORS = [ 'blue', 'red', 'green', 'yellow', 'violet' ];
var SERIES_INSTANCE_UID = '0020,000e';
var STUDY_INSTANCE_UID = '0020,000d';
var STUDY_DESCRIPTION = '0008,1030';
var STUDY_DATE = '0008,0020';

// Registry of the PDF series for which the instance metadata is still waiting
var pendingSeriesPdf_ = {};


function getParameterFromUrl(key) {
  var url = window.location.search.substring(1);
  var args = url.split('&');
  for (var i = 0; i < args.length; i++) {
    var arg = args[i].split('=');
    if (arg[0] == key) {
      return arg[1];
    }
  }
}


Vue.component('viewport', {
  props: [ 'left', 'top', 'width', 'height', 'canvasId', 'active', 'series', 'viewportIndex',
           'showInfo' ],
  template: '#viewport-template',
  data: function () {
    return {
      stone: stone,  // To access global object "stone" from "index.html"
      status: 'waiting',
      currentFrame: 0,
      numberOfFrames: 0,
      quality: '',
      cineControls: false,
      cineIncrement: 0,
      cineFramesPerSecond: 30,
      cineTimeoutId: null,
      cineLoadingFrame: false
    }
  },
  watch: {
    currentFrame: function(newVal, oldVal) {
      /**
       * The "FrameUpdated" event has been received, which indicates
       * that the schedule frame has been displayed: The cine loop can
       * proceed to the next frame (check out "CineCallback()").
       **/
      this.cineLoadingFrame = false;
    },
    series: function(newVal, oldVal) {
      this.status = 'loading';
      this.cineControls = false;
      this.cineMode = '';
      this.cineLoadingFrame = false;
      this.cineRate = 30;   // Default value
      
      if (this.cineTimeoutId !== null) {
        clearTimeout(this.cineTimeoutId);
        this.cineTimeoutId = null;
      }

      var studyInstanceUid = newVal.tags[STUDY_INSTANCE_UID];
      var seriesInstanceUid = newVal.tags[SERIES_INSTANCE_UID];
      stone.SpeedUpFetchSeriesMetadata(studyInstanceUid, seriesInstanceUid);

      if ((newVal.type == stone.ThumbnailType.IMAGE ||
           newVal.type == stone.ThumbnailType.NO_PREVIEW) &&
          newVal.complete) {
        this.status = 'ready';

        var that = this;
        Vue.nextTick(function() {
          stone.LoadSeriesInViewport(that.canvasId, seriesInstanceUid);
        });
      }
      else if (newVal.type == stone.ThumbnailType.PDF) {
        if (newVal.complete) {
          /**
           * Series is complete <=> One already knows about the
           * SOPInstanceUIDs that are available in this series. As a
           * consequence,
           * "OrthancStone::SeriesMetadataLoader::Accessor" will not
           * be empty in "ResourcesLoader::FetchPdf()" in C++ code.
           **/
          stone.FetchPdf(studyInstanceUid, seriesInstanceUid);
        } else {
          /**
           * The SOPInstanceUIDs in this series are not known
           * yet. Schedule an "stone.FetchPdf()" one the series
           * metadata is available.
           **/
          pendingSeriesPdf_[seriesInstanceUid] = true;
        }
      }
      else if (newVal.type == stone.ThumbnailType.VIDEO) {
        this.status = 'video';
        console.warn('Videos are not supported by the Stone Web viewer yet');
      }
    }
  },
  mounted: function() {
    var that = this;

    window.addEventListener('FrameUpdated', function(args) {
      if (args.detail.canvasId == that.canvasId) {
        that.currentFrame = (args.detail.currentFrame + 1);
        that.numberOfFrames = args.detail.numberOfFrames;
        that.quality = args.detail.quality;
      }
    });

    window.addEventListener('SeriesDetailsReady', function(args) {
      if (args.detail.canvasId == that.canvasId) {
        that.cineFramesPerSecond = stone.GetCineRate(that.canvasId);
      }
    });

    window.addEventListener('PdfLoaded', function(args) {
      var studyInstanceUid = args.detail.studyInstanceUid;
      var seriesInstanceUid = args.detail.seriesInstanceUid;
      var pdfPointer = args.detail.pdfPointer;
      var pdfSize = args.detail.pdfSize;

      if ('tags' in that.series &&
          that.series.tags[STUDY_INSTANCE_UID] == studyInstanceUid &&
          that.series.tags[SERIES_INSTANCE_UID] == seriesInstanceUid) {

        that.status = 'pdf';
        var pdf = new Uint8Array(HEAPU8.subarray(pdfPointer, pdfPointer + pdfSize));

        /**
         * It is not possible to bind an "Uint8Array" to a "props"
         * in the "pdf-viewer" component. So we have to directly
         * call the method of a component. But, "$refs are only
         * populated after the component has been rendered", so we
         * wait for the next rendering.
         * https://vuejs.org/v2/guide/components-edge-cases.html#Accessing-Child-Component-Instances-amp-Child-Elements
         **/
        Vue.nextTick(function() {
          that.$refs.pdfViewer.LoadPdf(pdf);
        });
      }
    });
  },
  methods: {
    SeriesDragAccept: function(event) {
      event.preventDefault();
    },
    SeriesDragDrop: function(event) {
      event.preventDefault();

      // The "parseInt()" is because of Microsoft Edge Legacy (*)
      this.$emit('updated-series', parseInt(event.dataTransfer.getData('seriesIndex'), 10));
    },
    MakeActive: function() {
      this.$emit('selected-viewport');
    },
    DecrementFrame: function(isCircular) {
      return stone.DecrementFrame(this.canvasId, isCircular);
    },
    IncrementFrame: function(isCircular) {
      return stone.IncrementFrame(this.canvasId, isCircular);
    },
    CinePlay: function() {
      this.cineControls = true;
      this.cineIncrement = 1;
      this.UpdateCine();
    },
    CinePause: function() {
      if (this.cineIncrement == 0) {
        // Two clicks on the "pause" button will hide the playback control
        this.cineControls = !this.cineControls;
      } else {
        this.cineIncrement = 0;
        this.UpdateCine();
      }
    },
    CineBackward: function() {
      this.cineControls = true;
      this.cineIncrement = -1;
      this.UpdateCine();
    },
    UpdateCine: function() {
      // Cancel the previous cine loop, if any
      if (this.cineTimeoutId !== null) {
        clearTimeout(this.cineTimeoutId);
        this.cineTimeoutId = null;
      }
      
      this.cineLoadingFrame = false;

      if (this.cineIncrement != 0) {
        this.CineCallback();
      }
    },
    CineCallback: function() {
      var reschedule;
      
      if (this.cineLoadingFrame) {
        /**
         * Wait until the frame scheduled by the previous call to
         * "CineCallback()" is actually displayed (i.e. we monitor the
         * "FrameUpdated" event). Otherwise, the background loading
         * process of the DICOM frames in C++ might be behind the
         * advancement of the current frame, which freezes the
         * display.
         **/
        reschedule = true;
      } else {
        this.cineLoadingFrame = true;
        
        if (this.cineIncrement == 1) {
          reschedule = this.DecrementFrame(true /* circular */);
        } else if (this.cineIncrement == -1) {
          reschedule = this.IncrementFrame(true /* circular */);
        } else {
          reschedule = false;  // Increment is zero, this test is just for safety
        }
      }
      
      if (reschedule) {
        this.cineTimeoutId = setTimeout(this.CineCallback, 1000.0 / this.cineFramesPerSecond);
      }     
    }
  }
});


var app = new Vue({
  el: '#wv',
  data: function() {
    return {
      stone: stone,  // To access global object "stone" from "index.html"
      ready: false,
      leftMode: 'grid',   // Can be 'small', 'grid' or 'full'
      leftVisible: true,
      viewportLayoutButtonsVisible: false,
      mouseActionsVisible: false,
      activeViewport: 0,
      showInfo: true,
      showReferenceLines: true,

      modalWarning: false,
      modalNotDiagnostic: false,
      modalPreferences: false,

      // User preferences (stored in the local storage)
      settingNotDiagnostic: true,
      settingSoftwareRendering: false,

      layoutCountX: 1,
      layoutCountY: 1,
      
      viewport1Width: '100%',
      viewport1Height: '100%',
      viewport1Left: '0%',
      viewport1Top: '0%',
      viewport1Visible: true,
      viewport1Series: {},
      
      viewport2Width: '100%',
      viewport2Height: '100%',
      viewport2Left: '0%',
      viewport2Top: '0%',
      viewport2Visible: false,
      viewport2Series: {},

      viewport3Width: '100%',
      viewport3Height: '100%',
      viewport3Left: '0%',
      viewport3Top: '0%',
      viewport3Visible: false,
      viewport3Series: {},

      viewport4Width: '100%',
      viewport4Height: '100%',
      viewport4Left: '0%',
      viewport4Top: '0%',
      viewport4Visible: false,
      viewport4Series: {},

      showWindowing: false,
      windowingPresets: [],

      series: [],
      studies: [],
      seriesIndex: {}  // Maps "SeriesInstanceUID" to "index in this.series"
    }
  },
  computed: {
    getSelectedStudies() {
      var s = '';
      for (var i = 0; i < this.studies.length; i++) {
        if (this.studies[i].selected) {
          if (s.length > 0)
            s += ', ';
          s += (this.studies[i].tags[STUDY_DESCRIPTION] + ' [' +
                this.studies[i].tags[STUDY_DATE] + ']');
        }
      }
      if (s.length == 0) 
        return '...';
      else
        return s;
    }
  },
  watch: { 
    leftVisible: function(newVal, oldVal) {
      this.FitContent();
    },
    showReferenceLines: function(newVal, oldVal) {
      stone.ShowReferenceLines(newVal ? 1 : 0);
    },
    settingNotDiagnostic: function(newVal, oldVal) {
      localStorage.settingNotDiagnostic = (newVal ? '1' : '0');
    },
    settingSoftwareRendering: function(newVal, oldVal) {
      localStorage.settingSoftwareRendering = (newVal ? '1' : '0');
    }
  },
  methods: {
    FitContent: function() {
      // This function can be used even if WebAssembly is not initialized yet
      if (typeof stone._AllViewportsUpdateSize !== 'undefined') {
        this.$nextTick(function () {
          stone.AllViewportsUpdateSize(true /* fit content */);
        });
      }
    },
    
    GetActiveSeries: function() {
      var s = [];

      if ('tags' in this.viewport1Series)
        s.push(this.viewport1Series.tags[SERIES_INSTANCE_UID]);

      if ('tags' in this.viewport2Series)
        s.push(this.viewport2Series.tags[SERIES_INSTANCE_UID]);

      if ('tags' in this.viewport3Series)
        s.push(this.viewport3Series.tags[SERIES_INSTANCE_UID]);

      if ('tags' in this.viewport4Series)
        s.push(this.viewport4Series.tags[SERIES_INSTANCE_UID]);

      return s;
    },

    GetActiveCanvas: function() {
      if (this.activeViewport == 1) {
        return 'canvas1';
      }
      else if (this.activeViewport == 2) {
        return 'canvas2';
      }
      else if (this.activeViewport == 3) {
        return 'canvas3';
      }
      else if (this.activeViewport == 4) {
        return 'canvas4';
      }
      else {
        return 'canvas1';
      }
    },

    SetResources: function(sourceStudies, sourceSeries) {     
      var indexStudies = {};

      var studies = [];
      var posColor = 0;

      for (var i = 0; i < sourceStudies.length; i++) {
        var studyInstanceUid = sourceStudies[i][STUDY_INSTANCE_UID];
        if (studyInstanceUid !== undefined) {
          if (studyInstanceUid in indexStudies) {
            console.error('Twice the same study: ' + studyInstanceUid);
          } else {
            indexStudies[studyInstanceUid] = studies.length;
            
            studies.push({
              'studyInstanceUid' : studyInstanceUid,
              'series' : [ ],
              'color' : COLORS[posColor],
              'selected' : true,
              'tags' : sourceStudies[i]
            });

            posColor = (posColor + 1) % COLORS.length;
          }
        }
      }

      var series = [];
      var seriesIndex = {};

      for (var i = 0; i < sourceSeries.length; i++) {
        var studyInstanceUid = sourceSeries[i][STUDY_INSTANCE_UID];
        var seriesInstanceUid = sourceSeries[i][SERIES_INSTANCE_UID];
        if (studyInstanceUid !== undefined &&
            seriesInstanceUid !== undefined) {
          if (studyInstanceUid in indexStudies) {
            seriesIndex[seriesInstanceUid] = series.length;
            var study = studies[indexStudies[studyInstanceUid]];
            study.series.push(i);
            series.push({
              'numberOfFrames' : 0,
              'complete' : false,
              'type' : stone.ThumbnailType.LOADING,
              'color': study.color,
              'tags': sourceSeries[i]
            });
          }
        }
      }
      
      this.studies = studies;
      this.series = series;
      this.seriesIndex = seriesIndex;
      this.ready = true;
    },
    
    SeriesDragStart: function(event, seriesIndex) {
      // It is necessary to use ".toString()" for Microsoft Edge Legacy (*)
      event.dataTransfer.setData('seriesIndex', seriesIndex.toString());
    },

    SetViewportSeriesInstanceUid: function(viewportIndex, seriesInstanceUid) {
      if (seriesInstanceUid in this.seriesIndex) {
        this.SetViewportSeries(viewportIndex, this.seriesIndex[seriesInstanceUid]);
      }
    },
    
    SetViewportSeries: function(viewportIndex, seriesIndex) {
      var series = this.series[seriesIndex];
      
      if (viewportIndex == 1) {
        this.viewport1Series = series;
      }
      else if (viewportIndex == 2) {
        this.viewport2Series = series;
      }
      else if (viewportIndex == 3) {
        this.viewport3Series = series;
      }
      else if (viewportIndex == 4) {
        this.viewport4Series = series;
      }
    },
    
    ClickSeries: function(seriesIndex) {
      this.SetViewportSeries(this.activeViewport, seriesIndex);
    },
    
    HideViewport: function(index) {
      if (index == 1) {
        this.viewport1Visible = false;
      }
      else if (index == 2) {
        this.viewport2Visible = false;
      }
      else if (index == 3) {
        this.viewport3Visible = false;
      }
      else if (index == 4) {
        this.viewport4Visible = false;
      }
    },
    
    ShowViewport: function(index, left, top, width, height) {
      if (index == 1) {
        this.viewport1Visible = true;
        this.viewport1Left = left;
        this.viewport1Top = top;
        this.viewport1Width = width;
        this.viewport1Height = height;
      }
      else if (index == 2) {
        this.viewport2Visible = true;
        this.viewport2Left = left;
        this.viewport2Top = top;
        this.viewport2Width = width;
        this.viewport2Height = height;
      }
      else if (index == 3) {
        this.viewport3Visible = true;
        this.viewport3Left = left;
        this.viewport3Top = top;
        this.viewport3Width = width;
        this.viewport3Height = height;
      }
      else if (index == 4) {
        this.viewport4Visible = true;
        this.viewport4Left = left;
        this.viewport4Top = top;
        this.viewport4Width = width;
        this.viewport4Height = height;
      }
    },
    
    SetViewportLayout: function(layout) {
      this.viewportLayoutButtonsVisible = false;
      if (layout == '1x1') {
        this.ShowViewport(1, '0%', '0%', '100%', '100%');
        this.HideViewport(2);
        this.HideViewport(3);
        this.HideViewport(4);
        this.layoutCountX = 1;
        this.layoutCountY = 1;
      }
      else if (layout == '2x2') {
        this.ShowViewport(1, '0%', '0%', '50%', '50%');
        this.ShowViewport(2, '50%', '0%', '50%', '50%');
        this.ShowViewport(3, '0%', '50%', '50%', '50%');
        this.ShowViewport(4, '50%', '50%', '50%', '50%');
        this.layoutCountX = 2;
        this.layoutCountY = 2;
      }
      else if (layout == '2x1') {
        this.ShowViewport(1, '0%', '0%', '50%', '100%');
        this.ShowViewport(2, '50%', '0%', '50%', '100%');
        this.HideViewport(3);
        this.HideViewport(4);
        this.layoutCountX = 2;
        this.layoutCountY = 1;
      }
      else if (layout == '1x2') {
        this.ShowViewport(1, '0%', '0%', '100%', '50%');
        this.ShowViewport(2, '0%', '50%', '100%', '50%');
        this.HideViewport(3);
        this.HideViewport(4);
        this.layoutCountX = 1;
        this.layoutCountY = 2;
      }

      this.FitContent();
    },

    UpdateSeriesThumbnail: function(seriesInstanceUid) {
      if (seriesInstanceUid in this.seriesIndex) {
        var index = this.seriesIndex[seriesInstanceUid];
        var series = this.series[index];

        var type = stone.LoadSeriesThumbnail(seriesInstanceUid);
        series.type = type;

        if (type == stone.ThumbnailType.IMAGE) {
          series.thumbnail = stone.GetStringBuffer();
        }

        // https://fr.vuejs.org/2016/02/06/common-gotchas/#Why-isn%E2%80%99t-the-DOM-updating
        this.$set(this.series, index, series);
      }
    },

    UpdateIsSeriesComplete: function(studyInstanceUid, seriesInstanceUid) {
      if (seriesInstanceUid in this.seriesIndex) {
        var index = this.seriesIndex[seriesInstanceUid];
        var series = this.series[index];

        var oldComplete = series.complete;
        
        series.complete = stone.IsSeriesComplete(seriesInstanceUid);
        
        if (!oldComplete &&
            series.complete)
        {
          series.numberOfFrames = stone.GetSeriesNumberOfFrames(seriesInstanceUid);
          
          if (seriesInstanceUid in pendingSeriesPdf_) {
            stone.FetchPdf(studyInstanceUid, seriesInstanceUid);
            delete pendingSeriesPdf_[seriesInstanceUid];
          }
        }

        // https://fr.vuejs.org/2016/02/06/common-gotchas/#Why-isn%E2%80%99t-the-DOM-updating
        this.$set(this.series, index, series);

        if ('tags' in this.viewport1Series &&
            this.viewport1Series.tags[SERIES_INSTANCE_UID] == seriesInstanceUid) {
          this.$set(this.viewport1Series, series);
        }

        if ('tags' in this.viewport2Series &&
            this.viewport2Series.tags[SERIES_INSTANCE_UID] == seriesInstanceUid) {
          this.$set(this.viewport2Series, series);
        }

        if ('tags' in this.viewport3Series &&
            this.viewport3Series.tags[SERIES_INSTANCE_UID] == seriesInstanceUid) {
          this.$set(this.viewport3Series, series);
        }

        if ('tags' in this.viewport4Series &&
            this.viewport4Series.tags[SERIES_INSTANCE_UID] == seriesInstanceUid) {
          this.$set(this.viewport4Series, series);
        }
      }
    },

    SetWindowing: function(center, width) {
      this.showWindowing = false;
      var canvas = this.GetActiveCanvas();
      if (canvas != '') {
        stone.SetWindowing(canvas, center, width);
      }
    },

    InvertContrast: function() {
      var canvas = this.GetActiveCanvas();
      if (canvas != '') {
        stone.InvertContrast(canvas);
      }
    },

    FlipX: function() {
      var canvas = this.GetActiveCanvas();
      if (canvas != '') {
        stone.FlipX(canvas);
      }
    },

    FlipY: function() {
      var canvas = this.GetActiveCanvas();
      if (canvas != '') {
        stone.FlipY(canvas);
      }
    },

    ApplyPreferences: function() {
      this.modalPreferences = false;

      if ((stone.IsSoftwareRendering() != 0) != this.settingSoftwareRendering) {
        document.location.reload();
      }
    },

    HideAllTooltips: function() {
      $('[data-toggle="tooltip"]').tooltip('hide');
    },

    SetMouseButtonActions: function(left, middle, right) {
      this.mouseActionsVisible = false;
      stone.SetMouseButtonActions(left, middle, right);
    },

    LoadOsiriXAnnotations: function(xml, clearPrevious)
    {
      if (stone.LoadOsiriXAnnotations(xml, clearPrevious)) {
        var seriesInstanceUid = stone.GetStringBuffer();

        this.SetViewportLayout('1x1');
        this.leftVisible = false;
        this.SetViewportSeriesInstanceUid(1, seriesInstanceUid);
          
        stone.FocusFirstOsiriXAnnotation('canvas1');
      }
    },

    ToggleWindowing: function()
    {
      if (this.showWindowing)
      {
        this.showWindowing = false;
      }
      else
      {
        stone.LoadWindowingPresets(this.GetActiveCanvas());
        this.windowingPresets = JSON.parse(stone.GetStringBuffer());

        var p = $('#windowing-popover').last();
        var top = p.offset().top + p.height() + 10;
        $('#windowing-content').css('top', top);
        //$('#windowing-content').css('right', '10');
        //$('#windowing-content').css('left', 'auto');

        this.showWindowing = true;
      }
    }
  },
  
  mounted: function() {
    this.SetViewportLayout('1x1');

    if (localStorage.settingNotDiagnostic) {
      this.settingNotDiagnostic = (localStorage.settingNotDiagnostic == '1');
    }
    
    if (localStorage.settingSoftwareRendering) {
      this.settingSoftwareRendering = (localStorage.settingSoftwareRendering == '1');
    }
    
    this.modalNotDiagnostic = this.settingNotDiagnostic;
  }
});



window.addEventListener('StoneInitialized', function() {
  stone.Setup(Module);
  stone.SetOrthancRoot('..', true);
  stone.SetSoftwareRendering(localStorage.settingSoftwareRendering == '1');
  console.warn('Stone properly initialized');

  var study = getParameterFromUrl('study');
  var series = getParameterFromUrl('series');

  if (study === undefined) {
    alert('No study was provided in the URL!');
  } else {
    if (series === undefined) {
      console.warn('Loading study: ' + study);
      stone.FetchStudy(study);
    } else {
      console.warn('Loading series: ' + series + ' from study ' + study);
      stone.FetchSeries(study, series);
      app.leftMode = 'full';
    }
  }
});


window.addEventListener('ResourcesLoaded', function() {
  console.log('resources loaded');

  var studies = [];
  for (var i = 0; i < stone.GetStudiesCount(); i++) {
    stone.LoadStudyTags(i);
    studies.push(JSON.parse(stone.GetStringBuffer()));
  }

  var series = [];
  for (var i = 0; i < stone.GetSeriesCount(); i++) {
    stone.LoadSeriesTags(i);
    series.push(JSON.parse(stone.GetStringBuffer()));
  }

  app.SetResources(studies, series);

  for (var i = 0; i < app.series.length; i++) {
    var studyInstanceUid = app.series[i].tags[STUDY_INSTANCE_UID];
    var seriesInstanceUid = app.series[i].tags[SERIES_INSTANCE_UID];
    app.UpdateSeriesThumbnail(seriesInstanceUid);
    app.UpdateIsSeriesComplete(studyInstanceUid, seriesInstanceUid);
  }
});


window.addEventListener('ThumbnailLoaded', function(args) {
  //var studyInstanceUid = args.detail.studyInstanceUid;
  var seriesInstanceUid = args.detail.seriesInstanceUid;
  app.UpdateSeriesThumbnail(seriesInstanceUid);
});


window.addEventListener('MetadataLoaded', function(args) {
  var studyInstanceUid = args.detail.studyInstanceUid;
  var seriesInstanceUid = args.detail.seriesInstanceUid;
  app.UpdateIsSeriesComplete(studyInstanceUid, seriesInstanceUid);
});


window.addEventListener('StoneException', function() {
  console.error('Exception catched in Stone');
});






$(document).ready(function() {
  // Enable support for tooltips in Bootstrap
  $('[data-toggle="tooltip"]').tooltip({
    placement: 'bottom',
    container: 'body',
    trigger: 'hover'
  });

  //app.modalWarning = true;


  var wasmSource = 'StoneWebViewer.js';
  
  // Option 1: Loading script using plain HTML
  
  /*
    var script = document.createElement('script');
    script.src = wasmSource;
    script.type = 'text/javascript';
    document.body.appendChild(script);
  */

  // Option 2: Loading script using AJAX (gives the opportunity to
  // report explicit errors)

  axios.get(wasmSource)
    .then(function (response) {
      var script = document.createElement('script');
      script.innerHTML = response.data;
      script.type = 'text/javascript';
      document.body.appendChild(script);
    })
    .catch(function (error) {
      alert('Cannot load the WebAssembly framework');
    });
});


// "Prevent Bootstrap dropdown from closing on clicks" for the list of
// studies: https://stackoverflow.com/questions/26639346
$('.dropdown-menu').click(function(e) {
  e.stopPropagation();
});


// Disable the selection of text using the mouse
document.onselectstart = new Function ('return false');







//var expectedOrigin = 'http://localhost:8042';
var expectedOrigin = '';   // TODO - INSECURE - CONFIGURATION

window.addEventListener('message', function(e) {
  if ('type' in e.data) {
    if (expectedOrigin != '' &&
        e.origin !== expectedOrigin) {
      alert('Bad origin for the message');
      return;
    }
    
    if (e.data.type == 'show-osirix-annotations') {
      var clear = true;  // Whether to clear previous annotations
      if ('clear' in e.data) {
        clear = e.data.clear;
      }
      
      app.LoadOsiriXAnnotations(e.data.xml, clear);
    } else {
      console.log('Unknown message type: ' + e.data.type);
    }
  }
});


function Test()
{
  var s = [ 'length.xml', 'arrow.xml', 'text.xml', 'angle.xml' ];

  for (var i = 0; i < s.length; i++) {
    axios.get(s[i])
      .then(function (response) {
        //var targetOrigin = 'http://localhost:8000';
        var targetOrigin = '*';  // TODO - INSECURE
        
        window.postMessage({
          'type': 'show-osirix-annotations',
          'xml': response.data,
          'clear': false
        }, targetOrigin);
      });
  }
}
