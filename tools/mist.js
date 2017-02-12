var mist = require("../build/Debug/mist");
var stream = require("stream");
var util = require("util")
var assert = require("assert");

// Writable
function _MistWritable(mistObj) {
    stream.Writable.call(this);
    this._mistObj = mistObj;
}

util.inherits(_MistWritable, stream.Writable);

_MistWritable.prototype._write = function (chunk, encoding, callback) {
    assert(typeof callback === 'function');
    this._mistObj._write(chunk, encoding, callback);
    //console.log("_write");
};

// Readable
function _MistReadable(mistObj)
{
    var self = this;
    stream.Readable.call(this);
    this._mistObj = mistObj;
    mistObj.setOnData(function (buffer) {
        //console.log("OnData", buffer.length);
        if (buffer.length == 0) {
            self.push(null);
        } else {
            self.push(buffer);
        }
    });
}

util.inherits(_MistReadable, stream.Readable);

_MistReadable.prototype._read = function (length) {
};

// Duplex
function _MistDuplex(mistObj)
{
    var self = this;
    stream.Duplex.call(this);
    this._mistObj = mistObj;
    mistObj.setOnData(function (buffer) {
        //console.log("OnData", buffer.length);
        if (buffer.length == 0) {
            self.push(null);
        } else {
            self.push(buffer);
        }
    });
}

util.inherits(_MistDuplex, stream.Duplex);

_MistDuplex.prototype._write = function (chunk, encoding, callback) {
    assert(typeof callback === 'function');
    this._mistObj._write(chunk, encoding, callback);
    //console.log("_write");
};

_MistDuplex.prototype._read = function (length) {
};



mist.ClientRequest.prototype.writeStream = function () {
    if (!this._writeStream)
        this._writeStream = new _MistWritable(this);
    return this._writeStream;
}

mist.ClientResponse.prototype.readStream = function () {
    if (!this._readStream)
        this._readStream = new _MistReadable(this);
    return this._readStream;
}

mist.ServerRequest.prototype.readStream = function () {
    if (!this._readStream)
        this._readStream = new _MistReadable(this);
    return this._readStream;
}

mist.ServerResponse.prototype.writeStream = function () {
    if (!this._writeStream)
        this._writeStream = new _MistWritable(this);
    return this._writeStream;
}

mist.Socket.prototype.stream = function () {
    if (!this._duplexStream)
        this._duplexStream = new _MistDuplex(this);
    return this._duplexStream;
}



module.exports = Object.keys(mist).reduce(
    function (obj, key) { obj[key] = mist[key]; return obj; }, {})
