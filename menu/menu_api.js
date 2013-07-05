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
