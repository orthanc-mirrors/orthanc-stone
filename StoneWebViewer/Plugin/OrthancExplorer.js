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
