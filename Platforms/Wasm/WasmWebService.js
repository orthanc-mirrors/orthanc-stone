mergeInto(LibraryManager.library, {
  WasmWebService_ScheduleGetRequest: function(callback, url, headersInJsonString, payload) {
    // Directly use XMLHttpRequest (no jQuery) to retrieve the raw binary data
    // http://www.henryalgus.com/reading-binary-files-using-jquery-ajax/
    var xhr = new XMLHttpRequest();
    var url_ = UTF8ToString(url);
    var headersInJsonString_ = UTF8ToString(headersInJsonString);

    xhr.open('GET', url_, true);
    xhr.responseType = 'arraybuffer';
    var headers = JSON.parse(headersInJsonString_);
    for (var key in headers) {
      xhr.setRequestHeader(key, headers[key]);
    }
    // console.log(xhr); 
    xhr.onreadystatechange = function() {
      if (this.readyState == XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          // TODO - Is "new Uint8Array()" necessary? This copies the
          // answer to the WebAssembly stack, hence necessitating
          // increasing the TOTAL_STACK parameter of Emscripten
          WasmWebService_NotifySuccess(callback, url_, new Uint8Array(this.response),
                                       this.response.byteLength, payload);
        } else {
          WasmWebService_NotifyError(callback, url_, payload);
        }
      }
    }
    
    xhr.send();
  },

  WasmWebService_NewScheduleGetRequest: function(callableSuccess, callableFailure, url, headersInJsonString, payload) {
    // Directly use XMLHttpRequest (no jQuery) to retrieve the raw binary data
    // http://www.henryalgus.com/reading-binary-files-using-jquery-ajax/
    var xhr = new XMLHttpRequest();
    var url_ = UTF8ToString(url);
    var headersInJsonString_ = UTF8ToString(headersInJsonString);

    xhr.open('GET', url_, true);
    xhr.responseType = 'arraybuffer';
    var headers = JSON.parse(headersInJsonString_);
    for (var key in headers) {
      xhr.setRequestHeader(key, headers[key]);
    }
    //console.log(xhr); 
    xhr.onreadystatechange = function() {
      if (this.readyState == XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          // TODO - Is "new Uint8Array()" necessary? This copies the
          // answer to the WebAssembly stack, hence necessitating
          // increasing the TOTAL_STACK parameter of Emscripten
          console.log("WasmWebService success", this.response); 
          WasmWebService_NewNotifySuccess(callableSuccess, url_, new Uint8Array(this.response),
                                       this.response.byteLength, payload);
          console.log("WasmWebService success 2", this.response); 
        } else {
          console.log("WasmWebService failed"); 
          WasmWebService_NewNotifyError(callableFailure, url_, payload);
        }
      }
    }
    
    xhr.send();
  },

  WasmWebService_SchedulePostRequest: function(callback, url, headersInJsonString, body, bodySize, payload) {
    var xhr = new XMLHttpRequest();
    var url_ = UTF8ToString(url);
    var headersInJsonString_ = UTF8ToString(headersInJsonString);
    xhr.open('POST', url_, true);
    xhr.responseType = 'arraybuffer';
    xhr.setRequestHeader('Content-type', 'application/octet-stream');

    var headers = JSON.parse(headersInJsonString_);
    for (var key in headers) {
      xhr.setRequestHeader(key, headers[key]);
    }
    
    xhr.onreadystatechange = function() {
      if (this.readyState == XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          WasmWebService_NotifySuccess(callback, url_, new Uint8Array(this.response),
                                       this.response.byteLength, payload);
        } else {
          WasmWebService_NotifyError(callback, url_, payload);
        }
      }
    }

    xhr.send(new Uint8ClampedArray(HEAPU8.buffer, body, bodySize));
  }
});
