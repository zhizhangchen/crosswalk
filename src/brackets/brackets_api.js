// Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
// Copyright (c) 2013 Intel Corporation. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

var brackets = { app: {}, fs: {} };

brackets.fs.NO_ERROR = 0;
brackets.fs.ERR_UNKNOWN = 1;
brackets.fs.ERR_INVALID_PARAMS = 2;
brackets.fs.ERR_NOT_FOUND = 3;
brackets.fs.ERR_CANT_READ = 4;
brackets.fs.ERR_UNSUPPORTED_ENCODING = 5;
brackets.fs.ERR_CANT_WRITE = 6;
brackets.fs.ERR_OUT_OF_SPACE = 7;
brackets.fs.ERR_NOT_FILE = 8;
brackets.fs.ERR_NOT_DIRECTORY = 9;
brackets.fs.ERR_FILE_EXISTS = 10;

brackets._postMessage = (function() {
  brackets._callbacks = [];
  cameo.setMessageListener('brackets', function(msg) {
    var c = brackets._callbacks.shift();
    c(JSON.parse(msg));
  });
  return function(json, callback) {
    brackets._callbacks.push(callback);
    brackets._post_count += 1;
    cameo.postMessage('brackets', JSON.stringify(json));
  };
})();

brackets.fs.readFile = function(path, encoding, callback) {
  var msg = {
    'cmd': 'ReadFile',
    'path': path,
    'encoding': encoding
  };
  brackets._postMessage(msg, function(r) {
    callback(r.error, r.data);
  });
};

brackets.fs.stat = function(path, callback) {
  var msg = {
    'cmd': 'GetFileModificationTime',
    'path': path
  };
  brackets._postMessage(msg, function (r) {
    callback(r.error, {
      isFile: function () {
        return !r.is_dir;
      },
      isDirectory: function () {
        return r.is_dir;
      },
      mtime: new Date(r.modtime * 1000)
    });
  });
};

brackets.fs.readdir = function(path, callback) {
  var msg = {
    'cmd': 'ReadDir',
    'path': path
  };
  brackets._postMessage(msg, function(r) {
    callback(r.error, r.files)
  });
};

brackets.fs.writeFile = function(path, data, encoding, callback) {
  var msg = {
    'cmd': 'WriteFile',
    'path': path,
    'data': data,
    'encoding': encoding
  };
  brackets._postMessage(msg, function(r) {
    callback(r.error);
  });
};

brackets.app.openLiveBrowser = function(url, enableRemoteDebugging, callback) {
  var msg = {
    'cmd': 'OpenLiveBrowser',
    'url': url
  };
  brackets._postMessage(msg, function(r) {
    callback(r.error);
  });
};

brackets.app.openURLInDefaultBrowser = function(callback, url) {
  var msg = {
    'cmd': 'OpenURLInDefaultBrowser',
    'url': url
  };
  brackets._postMessage(msg, function(r) {
    callback(r.error);
  });
};

brackets.fs.rename = function(oldPath, newPath, callback) {
  var msg = {
    'cmd': 'Rename',
    'oldPath': oldPath,
    'newPath': newPath
  };
  brackets._postMessage(msg, function(r) {
    callback(r.error);
  });
};

brackets.app.getApplicationSupportDirectory = function() {
  // FIXME(cmarcelo): Synchronous function. We need to store this
  // value when initializing the plugin.
  return '/tmp/brackets-support';
};

brackets.app.getNodeState = function(callback) {
  callback(true, 0);
};

brackets.fs.showOpenDialog = function(allowMultipleSelection, chooseDirectory, title, initialPath, fileTypes, callback) {
  var msg = {
    'cmd': 'ShowOpenDialog',
    'initialPath': initialPath
  };
  brackets._postMessage(msg, function(r) {
    callback(r.error, r.files);
  });
};

