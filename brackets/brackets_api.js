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

brackets.debugging_port = 0;

/* Create an API function for a native command.
 *
 * The first argument should be the command name, followed by argument
 * specifications of the API, which are normally argument names of the API,
 * except for a callback arugument, in which case an array or a customized
 * function can be used. The array should specifify the argument names of the
 * callback. The customzied function takes the response message and user
 * secified callback as its arguments. It can do some job using the reponse
 * message and then call the user specified callback. All the argument names
 * should match the message fields sent to or received from the extension.
 *
*/

brackets._createCommand = (function() {
  brackets._callbacks = {};
  brackets._next_reply_id = 0;
  cameo.setMessageListener('brackets', function(json) {
    var msg = JSON.parse(json);
    var reply_id = msg._reply_id;
    var callback = brackets._callbacks[reply_id];
    //console.log('Received message:' + json);
    if (callback) {
      delete msg._reply_id;
      delete brackets._callbacks[reply_id];
      callback(msg);
    } else {
      console.log('Invalid reply_id received from Brackets extension: ' + reply_id);
    }
  });
  return function() {
    var apiSpec = arguments;
    var cmdNameType =  typeof apiSpec[0];
    var callbackCount = 0;
    if (cmdNameType !== "string") {
      console.error('Command name should be string, but is ' + cmdNameType);
      return function () {};
    }
    for (var j = 1; j < apiSpec.length; j ++) {
      var argNameType =  typeof apiSpec[j];
      if (argNameType !== "string" &&
          !(apiSpec[j] instanceof Array) && argNameType !== "function") {
        console.error('The ' + j + ' argument name of ' + apiSpec[0]
            + ' should be string, array or function, but is ' + argNameType);
        return function () {};
      }
      if (apiSpec[j] instanceof Array || argNameType === "function")
        callbackCount ++;

    }
    if (callbackCount > 1) {
        console.error("Too many callback specs for command " + apiSpec[0]);
        return function () {};
    }
    return function () {
      var callback;
      var cbFields=[];
      var msg = { cmd: apiSpec[0] };
      var i;
      for (i = 0; i < arguments.length; i ++) {
        var arg = arguments[i];
        if (typeof arg === 'function') {
          callback = arg;
          cbFields = apiSpec[i + 1];
        }
        else {
          msg[apiSpec[i + 1]] = arg;
        }
      }
      var reply_id = brackets._next_reply_id;
      brackets._next_reply_id += 1;
      brackets._callbacks[reply_id] = function(r) {
        var cbArgs = [];
        if  (typeof cbFields === 'function')
          cbFields.call(null, r, callback);
        else {
          for (i = 0; i < cbFields.length; i ++) {
            cbArgs.push(r[cbFields[i]]);
          }
          callback.apply(null, cbArgs);
        }
      };
      msg._reply_id = reply_id.toString();
      //console.log('Sending message:'+ JSON.stringify(msg));
      cameo.postMessage('brackets', JSON.stringify(msg));
    }
  };
})();

// PROPERTIES
Object.defineProperty(brackets.app, "language", {
  writeable: false,
  get : function() { return (navigator.language).toLowerCase(); },
  enumerable : true,
  configurable : false
});

// SETUP MESSAGES
brackets._createCommand("GetRemoteDebuggingPort",["debugging_port"])(function (debugging_port) {
  brackets.debugging_port = debugging_port;
});

// API
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

brackets.app.showExtensionsFolder = function(appURL, callback) {
  return brackets.app.showOSFolder(brackets.app.getApplicationSupportDirectory() + '/extensions', callback);
};

brackets.app.getRemoteDebuggingPort = function() {
  return brackets.debugging_port;
};

brackets.fs.readFile = brackets._createCommand("ReadFile", "path", "encoding", ["error", "data"]);
brackets.fs.stat = brackets._createCommand("GetFileModificationTime", "path", function(r, callback) {
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
brackets.fs.readdir = brackets._createCommand("ReadDir", "path", ["error", "files"]);
brackets.fs.writeFile = brackets._createCommand("WriteFile", "path", "data", "encoding", ["error"]);
brackets.fs.rename = brackets._createCommand("Rename", "oldPath", "newPath", ["error"]);
brackets.fs.makedir = brackets._createCommand("MakeDir", "path", "mode", ["error"]);
brackets.fs.unlink = brackets._createCommand("DeleteFileOrDirectory", "path", ["error"]);
brackets.fs.moveToTrash = brackets._createCommand("MoveFileOrDirectoryToTrash", "path", ["error"]);
brackets.fs.isNetworkDrive = brackets._createCommand("IsNetworkDrive", "path", ["error"]);
brackets.app.openLiveBrowser = brackets._createCommand("OpenLiveBrowser", "url", "enableRemoteDebugging", ["error"]);
brackets.app.openURLInDefaultBrowser = brackets._createCommand("OpenURLInDefaultBrowser", ["error"], "url");
brackets.app.showOSFolder = brackets._createCommand("ShowOSFolder", "path", ["error"]);
brackets.app.getPendingFilesToOpen = brackets._createCommand("GetPendingFilesToOpen", ["error", "files"]);
