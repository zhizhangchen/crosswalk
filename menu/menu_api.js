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

cameo.setMessageListener('cameo.menu', function(msg) {
  if (cameo.menu.onActivatedMenuItem instanceof Function)
    cameo.menu.onActivatedMenuItem(msg);
});

cameo.menu.setMenuItemState = function(commandid, enabled, checked) {
  var msg = {
    'cmd': 'SetMenuItemState',
    'id': commandid,
    'enabled': enabled,
    'checked': checked
  };
  cameo.menu._postMessage(msg);
}

cameo.menu.addMenu = function(title, id, position, relativeId) {
  var msg = {
    'cmd': 'AddMenu',
    'id': id,
    'title': title,
    'position': position,
    'relative_id': relativeId,
  };
  cameo.menu._postMessage(msg);
}

cameo.menu.addMenuItem = function(parentId, title, id, key, displayStr,
				  position, relativeId) {
  var msg = {
    'cmd': 'AddMenuItem',
    'id': id,
    'title': title,
    'key': key,
    'display_str': displayStr,
    'parent_id': parentId,
    'position': position,
    'relative_id': relativeId,
  };
  cameo.menu._postMessage(msg);
}

cameo.menu.setMenuTitle = function (commandid, title) {
  var msg = {
    'cmd': 'SetMenuTitle',
    'id': commandid,
    'title': title,
  };
  cameo.menu._postMessage(msg);
}

cameo.menu.setMenuItemShortcut = function(commandId, shortcut, displayStr) {
  var msg = {
    'cmd': 'SetMenuItemShortcut',
    'id': commandId,
    'shortcut': shortcut,
    'display_str': displayStr,
  };
  cameo.menu._postMessage(msg);
}

cameo.menu.removeMenu = function(commandId) {
  var msg = {
    'cmd': 'RemoveMenu',
    'id': id,
  };
  cameo.menu._postMessage(msg);
}

cameo.menu.removeMenuItem = function(commandId) {
  var msg = {
    'cmd': 'RemoveMenuItem',
    'id': id,
  };
  cameo.menu._postMessage(msg);
}
