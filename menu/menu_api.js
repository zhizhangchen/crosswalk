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

cameo.menu._createCommand = (function() {
  cameo.menu._callbacks = {};
  cameo.menu._next_reply_id = 0;
  cameo.setMessageListener('cameo.menu', function(msg) {
    if (cameo.menu.onActivatedMenuItem instanceof Function)
      cameo.menu.onActivatedMenuItem(msg);
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
      var msg = { cmd: apiSpec[0] };
      var i;
      for (i = 0; i < arguments.length; i ++) {
        var arg = arguments[i];
        msg[apiSpec[i + 1]] = arg;
      }
      cameo.menu._postMessage(msg);
    }
  };
})();
// We accumulate the messages and posting them only in the next run of the
// mainloop.
cameo.menu._postMessage = (function() {
  var menuWasShown = false;
  var queue = [];
  var poster = function() {
    var i;
    if (!menuWasShown) {
      queue.push(JSON.stringify({'cmd': 'Show'}));
      menuWasShown = true;
    }
    // FIXME(cmarcelo): Change extension to accept a list of messages.
    for (i = 0; i < queue.length; i++)
      cameo.postMessage('cameo.menu', queue[i]);
    queue.length = 0;
  };
  return function(json) {
    if (queue.length == 0)
      setTimeout(poster, 0);
    queue.push(JSON.stringify(json));
  };
})();

cameo.menu.setMenuItemState = cameo.menu._createCommand("SetMenuItemState", "id", "enabled", "checked");
cameo.menu.addMenu = cameo.menu._createCommand("AddMenu", "title", "id", "position", "relativeId");
cameo.menu.addMenuItem = cameo.menu._createCommand("AddMenuItem", "parent_id", "title", "id", "key", "display_str", "position", "relative_id");
cameo.menu.setMenuTitle = cameo.menu._createCommand("SetMenuTitle", "id", "title");
cameo.menu.setMenuItemShortcut = cameo.menu._createCommand("SetMenuItemShortcut", "id", "shortcut", "display_str");
cameo.menu.removeMenu = cameo.menu._createCommand("RemoveMenu", "id");
cameo.menu.removeMenuItem = cameo.menu._createCommand("RemoveMenuItem", "id");
