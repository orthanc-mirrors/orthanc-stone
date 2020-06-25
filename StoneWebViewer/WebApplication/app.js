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
           'quality', 'framesCount', 'currentFrame', 'showInfo' ],
  template: '#viewport-template',
  data: function () {
    return {
      stone: stone,  // To access global object "stone" from "index.html"
      status: 'waiting'
    }
  },
  watch: { 
    series: function(newVal, oldVal) {
      this.status = 'loading';

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
      else if (newVal.type == stone.ThumbnailType.PDF ||
               newVal.type == stone.ThumbnailType.VIDEO) {
        // TODO
      }
    }
  },
  methods: {
    SeriesDragAccept: function(event) {
      event.preventDefault();
    },
    SeriesDragDrop: function(event) {
      event.preventDefault();
      this.$emit('updated-series', event.dataTransfer.getData('seriesIndex'));
    },
    MakeActive: function() {
      this.$emit('selected-viewport');
    },
    DecrementFrame: function() {
      stone.DecrementFrame(this.canvasId);
    },
    IncrementFrame: function() {
      stone.IncrementFrame(this.canvasId);
    }
  }
})


var app = new Vue({
  el: '#wv',
  data: function() {
    return {
      stone: stone,  // To access global object "stone" from "index.html"
      ready: false,
      leftMode: 'grid',   // Can be 'small', 'grid' or 'full'
      leftVisible: true,
      showWarning: false,
      viewportLayoutButtonsVisible: false,
      activeViewport: 0,
      showInfo: true,
      showReferenceLines: true,
      
      viewport1Width: '100%',
      viewport1Height: '100%',
      viewport1Left: '0%',
      viewport1Top: '0%',
      viewport1Visible: true,
      viewport1Series: {},
      viewport1Quality: '',
      viewport1FramesCount: 0,
      viewport1CurrentFrame: 0,
      
      viewport2Width: '100%',
      viewport2Height: '100%',
      viewport2Left: '0%',
      viewport2Top: '0%',
      viewport2Visible: false,
      viewport2Series: {},
      viewport2Quality: '',
      viewport2FramesCount: 0,
      viewport2CurrentFrame: 0,

      viewport3Width: '100%',
      viewport3Height: '100%',
      viewport3Left: '0%',
      viewport3Top: '0%',
      viewport3Visible: false,
      viewport3Series: {},
      viewport3Quality: '',
      viewport3FramesCount: 0,
      viewport3CurrentFrame: 0,

      viewport4Width: '100%',
      viewport4Height: '100%',
      viewport4Left: '0%',
      viewport4Top: '0%',
      viewport4Visible: false,
      viewport4Series: {},
      viewport4Quality: '',
      viewport4FramesCount: 0,
      viewport4CurrentFrame: 0,

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
    }
  },
  methods: {
    FitContent() {
      // This function can be used even if WebAssembly is not initialized yet
      if (typeof stone._AllViewportsUpdateSize !== 'undefined') {
        this.$nextTick(function () {
          stone.AllViewportsUpdateSize(true /* fit content */);
        });
      }
    },
    
    GetActiveSeries() {
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

    GetActiveCanvas() {
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
              //'length' : 4,
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
      event.dataTransfer.setData('seriesIndex', seriesIndex);
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
      }
      else if (layout == '2x2') {
        this.ShowViewport(1, '0%', '0%', '50%', '50%');
        this.ShowViewport(2, '50%', '0%', '50%', '50%');
        this.ShowViewport(3, '0%', '50%', '50%', '50%');
        this.ShowViewport(4, '50%', '50%', '50%', '50%');
      }
      else if (layout == '2x1') {
        this.ShowViewport(1, '0%', '0%', '50%', '100%');
        this.ShowViewport(2, '50%', '0%', '50%', '100%');
        this.HideViewport(3);
        this.HideViewport(4);
      }
      else if (layout == '1x2') {
        this.ShowViewport(1, '0%', '0%', '100%', '50%');
        this.ShowViewport(2, '0%', '50%', '100%', '50%');
        this.HideViewport(3);
        this.HideViewport(4);
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

    UpdateIsSeriesComplete: function(seriesInstanceUid) {
      if (seriesInstanceUid in this.seriesIndex) {
        var index = this.seriesIndex[seriesInstanceUid];
        var series = this.series[index];

        series.complete = stone.IsSeriesComplete(seriesInstanceUid);

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

    SetWindowing(center, width) {
      var canvas = this.GetActiveCanvas();
      if (canvas != '') {
        stone.SetWindowing(canvas, center, width);
      }
    },

    SetDefaultWindowing() {
      var canvas = this.GetActiveCanvas();
      if (canvas != '') {
        stone.SetDefaultWindowing(canvas);
      }
    },

    InvertContrast() {
      var canvas = this.GetActiveCanvas();
      if (canvas != '') {
        stone.InvertContrast(canvas);
      }
    }
  },
  
  mounted: function() {
    this.SetViewportLayout('1x1');
  }
});



window.addEventListener('StoneInitialized', function() {
  stone.Setup(Module);
  stone.SetOrthancRoot('..', true);
  console.warn('Native Stone properly intialized');

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
    var seriesInstanceUid = app.series[i].tags[SERIES_INSTANCE_UID];
    app.UpdateSeriesThumbnail(seriesInstanceUid);
    app.UpdateIsSeriesComplete(seriesInstanceUid);
  }
});


window.addEventListener('ThumbnailLoaded', function(args) {
  //var studyInstanceUid = args.detail.studyInstanceUid;
  var seriesInstanceUid = args.detail.seriesInstanceUid;
  app.UpdateSeriesThumbnail(seriesInstanceUid);
});


window.addEventListener('MetadataLoaded', function(args) {
  //var studyInstanceUid = args.detail.studyInstanceUid;
  var seriesInstanceUid = args.detail.seriesInstanceUid;
  app.UpdateIsSeriesComplete(seriesInstanceUid);
});


window.addEventListener('FrameUpdated', function(args) {
  var canvasId = args.detail.canvasId;
  var framesCount = args.detail.framesCount;
  var currentFrame = (args.detail.currentFrame + 1);
  var quality = args.detail.quality;
  
  if (canvasId == 'canvas1') {
    app.viewport1CurrentFrame = currentFrame;
    app.viewport1FramesCount = framesCount;
    app.viewport1Quality = quality;
  }
  else if (canvasId == 'canvas2') {
    app.viewport2CurrentFrame = currentFrame;
    app.viewport2FramesCount = framesCount;
    app.viewport2Quality = quality;
  }
  else if (canvasId == 'canvas3') {
    app.viewport3CurrentFrame = currentFrame;
    app.viewport3FramesCount = framesCount;
    app.viewport3Quality = quality;
  }
  else if (canvasId == 'canvas4') {
    app.viewport4CurrentFrame = currentFrame;
    app.viewport4FramesCount = framesCount;
    app.viewport4Quality = quality;
  }
});


window.addEventListener('StoneException', function() {
  console.error('Exception catched in Stone');
});






$(document).ready(function() {
  // Enable support for tooltips in Bootstrap
  //$('[data-toggle="tooltip"]').tooltip();

  //app.showWarning = true;


  $('#windowing-popover').popover({
    container: 'body',
    content: $('#windowing-content').html(),
    template: '<div class="popover wvToolbar__windowingPresetConfigPopover" role="tooltip"><div class="arrow"></div><h3 class="popover-title"></h3><div class="popover-content"></div></div>',
    placement: 'auto',
    html: true,
    sanitize: false,
    trigger: 'focus'   // Close on click
  });
  
  
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
