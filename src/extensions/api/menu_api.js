// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

if (cameo === undefined) {
  console.log('Could not inject menu API to cameo namespace');
  throw 'NoCameoFound';
}

cameo.menu = (function() {
  var menus = []
    , separatorCount = 0;

  var BaseMenu = function(id, params) {
    var items = {}
      , itemCreateTimeout = undefined;

    if (params !== undefined) {
      var keys = Object.keys(params);
      for (var i = 0; i < keys.length; i++)
        this[keys[i]] = params[keys[i]];
    }

    // Menu items are usually very numerous and the API being synchronous,
    // this leads to a pretty funny effect while the menus are being created.
    // Thus, creating an item merely creates a wrapper object, and adding
    // items to a top level menu will actually schedule them to be created
    // after a certain timeout (currently, 0.1s), in a batch, to reduce the
    // amount of messages posted to the UI process.
    var ItemCreateTimeoutFunction = function() {
      var all_item_ids = Object.keys(items)
        , unrealized_items = [];

      // Filter out items that were not created already, and create an
      // easily-serializable object describing each menu item.
      for (var i = 0; i < all_item_ids.length; i++) {
        var item = items[all_item_ids[i]];

        if (item.created !== undefined)
          continue;

        var serialized = {
          'menu_id': id,
          'id': item.id
        };
        if (item.label !== undefined)
          serialized.label = item.label;
        if (item.type !== 'item')
          serialized.type = item.type;
        if (item.sub_menu !== undefined)
          serialized.has_submenu = true;
        if (item.group !== undefined)
          serialized.group = item.group;

        unrealized_items.push(serialized);
        item.created = true;
      }

      // This function being called whenever items were not actually created
      // shouldn't actually happen.
      if (unrealized_items.length === 0) {
        console.log('Menu: unrealized item length is zero?');
        return;
      }

      // Send the batch to Cameo...
      cameo.postMessage('cameo.menu', {
        'cmd': 'CreateMenuItems',
        'menu_id': id,
        'items': unrealized_items
      });

      // ...and undefine the timeout.
      itemCreateTimeout = undefined;
    };

    var AddItem = function(item) {
      item.toplevel = this;
      items[item.id] = item;

      if (itemCreateTimeout !== undefined)
        itemCreateTimeout = clearTimeout(itemCreateTimeout);

      itemCreateTimeout = setTimeout(ItemCreateTimeoutFunction, 100);
    };

    var DeleteItem = function(item) {
      if (item.created === undefined) {
        delete items[item.id];
        return;
      }
      if (!items.hasOwnProperty(item.id))
        return;

      cameo.postMessage('cameo.menu', {
        'cmd': 'DelMenuItem',
        'menu_id': id,
        'id': item.id
      });

      delete items[item.id];
      delete item;
    };

    var Delete = function() {
      var item_ids = Object.keys(items);
      for (var i = 0; i < item_ids.length; i++)
        items[item_ids[i]].Delete();
      items = {};

      cameo.postMessage('cameo.menu', {
        'cmd': 'DelMenu',
        'id': id
      });
    };

    return {
      id: id,
      AddItem: AddItem,
      DeleteItem: DeleteItem,
      Delete: Delete
    };
  };

  var Toplevel = function(id, label) {
    cameo.postMessage('cameo.menu', {
      'cmd': 'AddMenuToplevel',
      'id': id,
      'label': label
    });

    var toplevel = new BaseMenu(id, {
      label: label
    });

    menus.push(toplevel);
    return toplevel;
  };

  var Context = function(id) {
    cameo.postMessage('cameo.menu', {
      'cmd': 'AddMenuContext',
      'id': id
    });

    var context_menu = new BaseMenu(id);
    context_menu.Popup = function() {
      cameo.postMessage('cameo.menu', {
        'cmd': 'PopUpMenu',
        'id': id
      });
    };

    menus.push(context_menu);
    return context_menu;
  };

  var BaseItem = function(id, options) {
    options = options || {};
    var callback = options.callback || function() {};
    var type = options.type || 'item';
    var enabled = options.enabled || true;

    var SetEnabled = function(setting) {
      this.enabled = !!setting;
      cameo.postMessage('cameo.menu', {
        'cmd': 'SetMenuItemEnabled',
        'setting': this.enabled,
        'id': this.id
      });
    };

    var Delete = function() {
      if (this.toplevel !== undefined) {
        this.toplevel.DeleteItem(this);
        return;
      }
      if (this.created) {
        cameo.postMessage('cameo.menu', {
          'cmd': 'DelMenuItem',
          'id': this.id
        });
      }
      delete this;
    };

    return {
      id: id,
      callback: callback,
      type: type,
      enabled: enabled,
      SetEnabled: SetEnabled,
      Delete: Delete
    };
  };

  var Item = function(id, label, options) {
    var item = new BaseItem(id, options);

    item.SetKeyBinding = function(setting) {
      this.key_binding = setting;
      cameo.postMessage('cameo.menu', {
        'cmd': 'SetMenuItemKeyBinding',
        'setting': item.key_binding,
        'id': item.id
      });
    };

    item.label = label;
    if (options !== undefined) {
      item.key_binding = options.key_binding || undefined;
      item.sub_menu = options.sub_menu || undefined;
      item.group = options.group || undefined;
    }

    return item;
  };

  var GetNewSeparatorId = function() {
    separatorCount++;
    return 'separator' + separatorCount;
  };

  var Separator = function() {
    return new BaseItem(GetNewSeparatorId(), {
      'type': 'separator'
    });
  };

  var OnMessageReceived = function(message) {
    console.log('Message received: ' + JSON.stringify(message));
  };

  cameo.setMessageListener('cameo.menu', OnMessageReceived);

  return {
    Separator: Separator,
    Item: Item,
    Context: Context,
    Toplevel: Toplevel,
    DeleteAllMenus: function() {
      for (var i = 0; i < menus.length; i++)
        menus[i].Delete();
      menus = [];
    }
  };
})();
