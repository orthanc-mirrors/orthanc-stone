$('#study').live('pagebeforecreate', function() {
  var b = $('<a>')
      .attr('data-role', 'button')
      .attr('href', '#')
      .attr('data-icon', 'search')
      .attr('data-theme', 'e')
      .text('Stone Web Viewer');

  b.insertBefore($('#study-delete').parent().parent());
  b.click(function() {
    if ($.mobile.pageData) {
      $.ajax({
        url: '../studies/' + $.mobile.pageData.uuid,
        dataType: 'json',
        cache: false,
        success: function(study) {
          var studyInstanceUid = study.MainDicomTags.StudyInstanceUID;
          window.open('../stone-webviewer/index.html?study=' + studyInstanceUid);
        }
      });      
    }
  });
});


$('#series').live('pagebeforecreate', function() {
  var b = $('<a>')
      .attr('data-role', 'button')
      .attr('href', '#')
      .attr('data-icon', 'search')
      .attr('data-theme', 'e')
      .text('Stone Web Viewer');

  b.insertBefore($('#series-delete').parent().parent());
  b.click(function() {
    if ($.mobile.pageData) {
      $.ajax({
        url: '../series/' + $.mobile.pageData.uuid,
        dataType: 'json',
        cache: false,
        success: function(series) {
          $.ajax({
            url: '../studies/' + series.ParentStudy,
            dataType: 'json',
            cache: false,
            success: function(study) {
              var studyInstanceUid = study.MainDicomTags.StudyInstanceUID;
              var seriesInstanceUid = series.MainDicomTags.SeriesInstanceUID;
              window.open('../stone-webviewer/index.html?study=' + studyInstanceUid +
                          '&series=' + seriesInstanceUid);
            }
          });      
        }
      });      
    }
  });
});

$('#series').live('pagebeforecreate', function() {
  var b = $('<a>')
      .attr('data-role', 'button')
      .attr('href', '#')
      .attr('data-icon', 'search')
      .attr('data-theme', 'e')
      .text('Stone MPR RT Viewer');``

  b.insertBefore($('#series-delete').parent().parent());
  b.click(function() {
    if ($.mobile.pageData) {
      $.ajax({
        url: '../series/' + $.mobile.pageData.uuid,
        dataType: 'json',
        cache: false,
        success: function(series) {

          // we consider that the imaging series to display is the 
          // current one.
          // we will look for RTDOSE and RTSTRUCT instances in the 
          // sibling series from the same study. The first one of 
          // each modality will be grabbed.
          let ctSeries = $.mobile.pageData.uuid;

          $.ajax({
            url: '../studies/' + series.ParentStudy,
            dataType: 'json',
            cache: false,
            success: function(study) {
              // Loop on the study series and find the first RTSTRUCT and RTDOSE instances,
              // if any.
              let rtStructInstance = null;
              let rtDoseInstance = null;
              let rtPetInstance = null;
              let seriesRequests = []

              study.Series.forEach( function(studySeriesUuid) {
                let request = $.ajax({
                  url: '../series/' + studySeriesUuid,
                  dataType: 'json',
                  cache: false,
                });
                seriesRequests.push(request);
              });

              $.when.apply($,seriesRequests).then(function() {
                [].forEach.call(arguments, function(response) {
                  siblingSeries = response[0]
                  if (siblingSeries.MainDicomTags.Modality == "RTDOSE") {
                    // we have found an RTDOSE series. Let's grab the first instance
                    if (siblingSeries.Instances.length > 0) {
                      if(rtDoseInstance == null) {
                        rtDoseInstance = siblingSeries.Instances[0];
                      }
                    }
                  }
                  if (siblingSeries.MainDicomTags.Modality == "PT") {
                    // we have found an RTDOSE series. Let's grab the first instance
                    if (siblingSeries.Instances.length > 0) {
                      if(rtPetInstance == null) {
                        rtPetInstance = siblingSeries.Instances[0];
                      }
                    }
                  }
                  if (siblingSeries.MainDicomTags.Modality == "RTSTRUCT") {
                    // we have found an RTDOSE series. Let's grab the first instance
                    if (siblingSeries.Instances.length > 0) {
                      if(rtStructInstance == null) {
                        rtStructInstance = siblingSeries.Instances[0];
                      }
                    }
                  }
                });
                let mprViewerUrl = '../stone-rtviewer/index.html?ctseries=' + ctSeries + 
                '&rtdose=' + rtDoseInstance + 
                '&rtstruct=' + rtStructInstance;
                //console.log("About to open: " + mprViewerUrl);
                window.open(mprViewerUrl);
              });
            }
          });      
        }
      });      
    }
  });
});

