function beforePrint(event) {
  var body = $('body');
  body.addClass('print');

  // because firefox does not support/executes codes after the cloned document as been rendered
  // https://bugzilla.mozilla.org/show_bug.cgi?format=default&id=1048317
  // we cannot calculate using the good body size for the clone document
  // so we have to hardcode the body width (meaning we can only renders in A4 in firefox);
  var uaParser = new UAParser();
  var isFirefox = (uaParser.getBrowser().name === 'Firefox');
  var isIE = (uaParser.getBrowser().name === 'IE');
  var isEdge = (uaParser.getBrowser().name === 'Edge');
  //console.log('ua parser', uaParser.getBrowser());
  
  if (isFirefox || isIE || isEdge) {
    if (0) {
      // This is Letter
      body.css('width', '8.5in');
      body.css('height', '11in');
    } else {
      // This is A4
      body.css('width', '210mm');
      body.css('height', '296mm');  // If using "297mm", Firefox creates a second blank page
    }
  }

  $('#viewport canvas').each(function(key, canvas) {
    if ($(canvas).is(':visible')) {
      $(canvas).width($(canvas).parent().width());
      $(canvas).height($(canvas).parent().height());
    }
  });

  stone.FitForPrint();
};


function afterPrint() {
  var body = $('body');
  body.removeClass('print');
  body.css('width', '100%');
  body.css('height', '100%');
  $('#viewport canvas').css('width', '100%');
  $('#viewport canvas').css('height', '100%');
  
  stone.FitForPrint();
}


window.addEventListener('beforeprint', function(event) {
  beforePrint(event);
});


var printMedia = window.matchMedia('print');
printMedia.addListener(function(mql) {
  if (mql.matches) {
    // webkit equivalent of onbeforeprint
    beforePrint();
  }
});


window.addEventListener('afterprint', function() {
  afterPrint();
});


/*vm.cancelPrintMode = function() {
  afterPrint();
}
*/
