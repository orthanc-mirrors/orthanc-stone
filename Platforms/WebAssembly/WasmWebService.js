mergeInto(LibraryManager.library, {
  WasmWebService_ScheduleGetRequest: function(callback, url, payload) {
    // Directly use XMLHttpRequest (no jQuery) to retrieve the raw binary data
    // http://www.henryalgus.com/reading-binary-files-using-jquery-ajax/
    var xhr = new XMLHttpRequest();
    var tmp = UTF8ToString(url);
    xhr.open('GET', tmp, true);
    xhr.responseType = 'arraybuffer';

    xhr.onreadystatechange = function() {
      if (this.readyState == XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          // TODO - Is "new Uint8Array()" necessary? This copies the
          // answer to the WebAssembly stack, hence necessitating
          // increasing the TOTAL_STACK parameter of Emscripten
          WasmWebService_NotifySuccess(callback, tmp, new Uint8Array(this.response),
                                       this.response.byteLength, payload);
        } else {
          WasmWebService_NotifyError(callback, tmp, payload);
        }
      }
    }
    
    xhr.send();
  },

  WasmWebService_SchedulePostRequest: function(callback, url, body, bodySize, payload) {
    var xhr = new XMLHttpRequest();
    var tmp = UTF8ToString(url);
    xhr.open('POST', tmp, true);
    xhr.responseType = 'arraybuffer';
    xhr.setRequestHeader('Content-type', 'application/octet-stream');
    
    xhr.onreadystatechange = function() {
      if (this.readyState == XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          WasmWebService_NotifySuccess(callback, tmp, new Uint8Array(this.response),
                                       this.response.byteLength, payload);
        } else {
          WasmWebService_NotifyError(callback, tmp, payload);
        }
      }
    }

    xhr.send(new Uint8ClampedArray(HEAPU8.buffer, body, bodySize));
  }
});
