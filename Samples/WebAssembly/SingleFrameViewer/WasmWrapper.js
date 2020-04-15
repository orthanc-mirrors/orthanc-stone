
const Stone = function() {
  this._InitializeViewport = undefined;
  this._LoadOrthanc = undefined;
  this._LoadDicomWeb = undefined;
};

Stone.prototype.Setup = function(Module) {
  this._InitializeViewport = Module.cwrap('InitializeViewport', null, [ 'string' ]);
  this._LoadOrthanc = Module.cwrap('LoadOrthanc', null, [ 'string', 'int' ]);
  this._LoadDicomWeb = Module.cwrap('LoadDicomWeb', null, [ 'string', 'string', 'string', 'string', 'int' ]);
};

Stone.prototype.InitializeViewport = function(canvasId) {
  this._InitializeViewport(canvasId);
};

Stone.prototype.LoadOrthanc = function(instance, frame) {
  this._LoadOrthanc(instance, frame);
};

Stone.prototype.LoadDicomWeb = function(server, studyInstanceUid, seriesInstanceUid, sopInstanceUid, frame) {
  this._LoadDicomWeb(server, studyInstanceUid, seriesInstanceUid, sopInstanceUid, frame);
};

var stone = new Stone();

