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

// NOTE: This contains an implementation that covers exclusively the Brackets
// case, and its probably not adequated for other cases. The goal is to iterate
// on top of this and create a proper Cameo.Menu API for allowing Cameo apps to
// use native menus.

if (cameo === undefined) {
  console.log('Could not inject menu API to cameo namespace');
  throw 'NoCameoFound';
}

cameo.menu = cameo.menu || {}

cameo.addEventListener = (function() {
  cameo._event_listeners = {};
  return function (extension, type,  callback) {
    cameo._event_listeners[extension] = cameo._event_listeners[extension] || {};
    cameo._event_listeners[extension][type] = cameo._event_listeners[extension][type] || [];
    cameo._event_listeners[extension][type].push(callback);
  }
})();

/* Create an API function for a native command.
 *
 * The first argument should be the command name, followed by argument
 * specifications of the API, which are normally argument names of the API,
 * except for a callback argument, in which case an array or a customized
 * function can be used. The array should specifify the argument names of the
 * callback. The customzied function takes the response message and user
 * secified callback as its arguments. It can do some job using the reponse
 * message and then call the user specified callback. All the argument names
 * should match the message fields sent to or received from the extension.
 *
*/

cameo.createCommand = (function() {
  var queue = {};
  cameo._callbacks = {};
  cameo._next_reply_id = 0;
  function poster() {
    // FIXME(cmarcelo): Change extension to accept a list of messages.
    for (extension in queue) {
      var extensionQueue = queue[extension];
      for (i = 0; i < extensionQueue.length; i++) {
        cameo.postMessage(extension, JSON.stringify(extensionQueue[i]));
        if (cameo._event_listeners[extension])
          cameo._event_listeners[extension]['commandExecuted'].forEach(function (listener) {
            listener(extensionQueue[i]);
        });
      }
      extensionQueue.length = 0;
    }
  }
  return function() {
    var apiSpec = arguments;
    var extension = apiSpec[0];
    var cmd = apiSpec[1];
    var cmdNameType = typeof cmd;
    var callbackCount = 0;
    if (cmdNameType !== "string") {
      console.error('Command name should be string, but is ' + cmdNameType);
      return function () {};
    }
    for (var j = 2; j < apiSpec.length; j++) {
      var argNameType = typeof apiSpec[j];
      if (argNameType !== "string" &&
          !(apiSpec[j] instanceof Array) && argNameType !== "function") {
        console.error('The ' + j + ' argument name of ' + cmd
            + ' should be string, array or function, but is ' + argNameType);
        return function () {};
      }
      if (apiSpec[j] instanceof Array || argNameType === "function")
        callbackCount++;

    }
    if (callbackCount > 1) {
        console.error("Too many callback specs for command " + cmd);
        return function () {};
    }
    cameo.setMessageListener(extension, function(json) {
      var msg;
      try {
        msg = JSON.parse(json);
      }catch (e) {
        msg = json;
      }
      var reply_id = msg._reply_id;
      var callback = cameo._callbacks[reply_id];
      if (callback) {
        delete msg._reply_id;
        delete cameo._callbacks[reply_id];
        callback(msg);
      } else if (!reply_id){
        cameo._event_listeners[extension][msg.type || 'default'].forEach(function (listener) {
          listener(msg);
        });
      }
      else
        console.log('Invalid reply_id received from Brackets extension: ' + reply_id);
    });
    return function () {
      var callback;
      var cbFields=[];
      var msg = { cmd: cmd };
      var i;
      for (i = 0; i < arguments.length; i++) {
        var arg = arguments[i];
        if (typeof arg === 'function') {
          callback = arg;
          cbFields = apiSpec[i + 2];
        }
        else {
          msg[apiSpec[i + 2]] = arg;
        }
      }
      if (callbackCount === 1) {
        var reply_id = cameo._next_reply_id;
        cameo._next_reply_id += 1;
        cameo._callbacks[reply_id] = function(r) {
          var cbArgs = [];
          if  (typeof cbFields === 'function')
            cbFields.call(null, r, callback);
          else {
            for (i = 0; i < cbFields.length; i++) {
              cbArgs.push(r[cbFields[i]]);
            }
            callback.apply(null, cbArgs);
          }
        };
        msg._reply_id = reply_id.toString();
      }
      queue[extension] = queue[extension] || [];
      queue[extension].push(msg);
      setTimeout(poster, 0);
    }
  };
})();

cameo.addEventListener('cameo.menu', 'default', function(msg) {
  if (cameo.menu.onActivatedMenuItem instanceof Function)
    cameo.menu.onActivatedMenuItem(msg);
});
cameo.addEventListener('cameo.menu', 'commandExecuted', (function(msg) {
  var menuWasShown = false;
  return function (msg) {
    if (!menuWasShown && msg.cmd !== "Show") {
      cameo.createCommand("cameo.menu", "Show")();
      menuWasShown = true;
    }
  }
})());

cameo.menu._createCommand = function () {
  [].unshift.call(arguments, "cameo.menu");
  return cameo.createCommand.apply(null, arguments);
}

cameo.menu.setMenuItemState = cameo.menu._createCommand("SetMenuItemState", "id", "enabled", "checked");
cameo.menu.addMenu = cameo.menu._createCommand("AddMenu", "title", "id", "position", "relativeId");
cameo.menu.addMenuItem = cameo.menu._createCommand("AddMenuItem", "parent_id", "title", "id", "key", "display_str", "position", "relative_id");
cameo.menu.setMenuTitle = cameo.menu._createCommand("SetMenuTitle", "id", "title");
cameo.menu.setMenuItemShortcut = cameo.menu._createCommand("SetMenuItemShortcut", "id", "shortcut", "display_str");
cameo.menu.removeMenu = cameo.menu._createCommand("RemoveMenu", "id");
cameo.menu.removeMenuItem = cameo.menu._createCommand("RemoveMenuItem", "id");
