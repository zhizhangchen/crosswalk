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

brackets.app.NO_ERROR = 0;

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

brackets.app.getApplicationSupportDirectory = function() {
  // FIXME(cmarcelo): Synchronous function. We need to store this
  // value when initializing the plugin.
  return '/tmp/brackets-support';
};

brackets.app.getNodeState = function(callback) {
  callback(true, 0);
};

brackets.app.quit = function() {
  window.close();
};

cameo.menu = cameo.menu || {};
cameo.menu.onActivatedMenuItem = function(item) {
    brackets.shellAPI.executeCommand(item);
};

brackets.app.addMenu = function(title, id, position, relativeId, callback) {
  cameo.menu.addMenu(title, id, position, relativeId);
  callback(brackets.app.NO_ERROR);
};

brackets.app.addMenuItem = function(parentId, title, id, key, displayStr, position, relativeId, callback) {
  cameo.menu.addMenuItem(parentId, title, id, key, displayStr, position, relativeId);
  callback(brackets.app.NO_ERROR);
}

brackets.app.setMenuTitle = function(commandid, title, callback) {
  cameo.menu.setMenuTitle(commandid, title);
  callback(brackets.app.NO_ERROR);
}

brackets.app.setMenuItemState = function(commandid, enabled, checked, callback) {
  cameo.menu.setMenuItemState(commandid, enabled, checked);
  callback(brackets.app.NO_ERROR);
}

brackets.app.setMenuItemShortcut = function(commandid, shortcut, displayStr, callback) {
  cameo.menu.setMenuItemShortcut(commandid, shortcut, displayStr);
  callback(brackets.app.NO_ERROR);
}

// Each API entry contains the command to the native implementation and the
// arguments of the API. For a callback argument of the API, normally an array
// can be used to specify the response data names that the callback function is
// interested in.  Or it can be a customized function, with response message
// and user secified callback as its arguments
cameo.registerApiGroup(brackets.fs, "brackets", {
  readFile: ["ReadFile", "path", "encoding", ["error", "data"]],
  stat: ["GetFileModificationTime", "path", function(r, callback) {
    callback(r.error, {
      isFile: function () {
        return !r.is_dir;
      },
      isDirectory: function () {
        return r.is_dir;
      },
      mtime: new Date(r.modtime * 1000)
    });
  }],
  readdir: ["ReadDir", "path", ["error", "files"]],
  writeFile: ["WriteFile", "path", "data", "encoding", ["error"]],
  rename: ["Rename", "oldPath", "newPath", ["error"]],
  makedir: ["MakeDir", "path", "mode", ["error"]],
  unlink: ["DeleteFileOrDirectory", "path", ["error"]],
  moveToTrash: ["MoveFileOrDirectoryToTrash", "path", ["error"]],
  isNetworkDrive: ["IsNetworkDrive", "path", ["error"]],
});

cameo.registerApiGroup(brackets.app, "brackets", {
  openLiveBrowser: ["OpenLiveBrowser", "url", "enableRemoteDebugging", ["error"]],
  openURLInDefaultBrowser: ["OpenURLInDefaultBrowser", ["error"], "url"]
});
