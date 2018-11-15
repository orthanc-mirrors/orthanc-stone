mergeInto(LibraryManager.library, {
  WasmWebService_GetAsync: function(callableSuccess, callableFailure, url, headersInJsonString, payload, timeoutInSeconds) {
    // Directly use XMLHttpRequest (no jQuery) to retrieve the raw binary data
    // http://www.henryalgus.com/reading-binary-files-using-jquery-ajax/
    var xhr = new XMLHttpRequest();
    var url_ = UTF8ToString(url);
    var headersInJsonString_ = UTF8ToString(headersInJsonString);

    xhr.open('GET', url_, true);
    xhr.responseType = 'arraybuffer';
    xhr.timeout = timeoutInSeconds * 1000;
    var headers = JSON.parse(headersInJsonString_);
    for (var key in headers) {
      xhr.setRequestHeader(key, headers[key]);
    }
    //console.log(xhr); 
    xhr.onreadystatechange = function() {
      if (this.readyState == XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          var s = xhr.getAllResponseHeaders();
          var headers = _malloc(s.length + 1);
          writeStringToMemory(s, headers);
          
          // TODO - Is "new Uint8Array()" necessary? This copies the
          // answer to the WebAssembly stack, hence necessitating
          // increasing the TOTAL_STACK parameter of Emscripten
          WasmWebService_NotifySuccess(callableSuccess, url_, new Uint8Array(this.response),
                                       this.response.byteLength, headers, payload);
        } else {
          WasmWebService_NotifyError(callableFailure, url_, payload);
        }
      }
    }
    
    xhr.send();
  },

  WasmWebService_PostAsync: function(callableSuccess, callableFailure, url, headersInJsonString, body, bodySize, payload, timeoutInSeconds) {
    var xhr = new XMLHttpRequest();
    var url_ = UTF8ToString(url);
    var headersInJsonString_ = UTF8ToString(headersInJsonString);
    xhr.open('POST', url_, true);
    xhr.timeout = timeoutInSeconds * 1000;
    xhr.responseType = 'arraybuffer';
    xhr.setRequestHeader('Content-type', 'application/octet-stream');

    var headers = JSON.parse(headersInJsonString_);
    for (var key in headers) {
      xhr.setRequestHeader(key, headers[key]);
    }
    
    xhr.onreadystatechange = function() {
      if (this.readyState == XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          var s = xhr.getAllResponseHeaders();
          var headers = _malloc(s.length + 1);
          writeStringToMemory(s, headers);

          WasmWebService_NotifySuccess(callableSuccess, url_, new Uint8Array(this.response),
                                       this.response.byteLength, headers, payload);
        } else {
          WasmWebService_NotifyError(callableFailure, url_, payload);
        }
      }
    }

    xhr.send(new Uint8ClampedArray(HEAPU8.buffer, body, bodySize));
  },

  WasmWebService_DeleteAsync: function(callableSuccess, callableFailure, url, headersInJsonString, payload, timeoutInSeconds) {
    var xhr = new XMLHttpRequest();
    var url_ = UTF8ToString(url);
    var headersInJsonString_ = UTF8ToString(headersInJsonString);
    xhr.open('DELETE', url_, true);
    xhr.timeout = timeoutInSeconds * 1000;
    xhr.responseType = 'arraybuffer';
    xhr.setRequestHeader('Content-type', 'application/octet-stream');
  
    var headers = JSON.parse(headersInJsonString_);
    for (var key in headers) {
      xhr.setRequestHeader(key, headers[key]);
    }
    
    xhr.onreadystatechange = function() {
      if (this.readyState == XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          var s = xhr.getAllResponseHeaders();
          var headers = _malloc(s.length + 1);
          writeStringToMemory(s, headers);

          WasmWebService_NotifySuccess(callableSuccess, url_, new Uint8Array(this.response),
                                       this.response.byteLength, headers, payload);
        } else {
          WasmWebService_NotifyError(callableFailure, url_, payload);
        }
      }
    }
  
    xhr.send();
  }
  
});
