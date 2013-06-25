// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAMEO_SRC_MENU_MENU_EXTENSION_H_
#define CAMEO_SRC_MENU_MENU_EXTENSION_H_

#include <gtk/gtk.h>
#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "cameo/src/extensions/browser/cameo_extension.h"
#include "cameo/src/runtime/browser/runtime.h"
#include "cameo/src/runtime/browser/runtime_registry.h"
#include "ui/gfx/native_widget_types.h"

namespace base {
class DictionaryValue;
};

using cameo::extensions::CameoExtension;

namespace cameo {

class MenuContext;

enum MenuItemType {
  kRegularItem,
  kRadioItem,
  kCheckItem,
  kSeparatorItem
};

class MenuExtension : public CameoExtension,
                      public RuntimeRegistryObserver {
 public:
  explicit MenuExtension(RuntimeRegistry* runtime_registry);
  virtual ~MenuExtension();

  // CameoExtension implementation.
  virtual const char* GetJavaScriptAPI() OVERRIDE;
  virtual Context* CreateContext(const PostMessageCallback& post_message) OVERRIDE;

  // RuntimeRegistryObserver implementation.
  virtual void OnRuntimeAdded(Runtime* runtime) OVERRIDE;
  virtual void OnRuntimeRemoved(Runtime* runtime) OVERRIDE {}
  virtual void OnRuntimeAppIconChanged(Runtime* runtime) OVERRIDE {}

 private:
  friend class MenuContext;

  RuntimeRegistry* runtime_registry_;
  gfx::NativeWindow native_window_;
};

class MenuContext : public CameoExtension::Context {
 public:
  MenuContext(MenuExtension* menu_extension,
              const CameoExtension::PostMessageCallback& post_message);
  virtual ~MenuContext();

  // CameoExtension::Context implementation.
  virtual void HandleMessage(const std::string& msg) OVERRIDE;

 private:
  // FIXME(cmarcelo): Use a weakptrfactory instead of base::Unretained
  // everywhere.

  void InitializeMenu();

  GtkWidget* FindMenu(const std::string& id);
  GtkWidget* FindMenuItem(const std::string& id);

  void AddMenuItem(const std::string& menu_id, const std::string& id,
                   const std::string& label, MenuItemType type,
                   const std::string& group);
  void PopUpMenu(const std::string& id);
  void DeleteMenu(const std::string& id);
  void DeleteMenuItem(const std::string& id);
  void SetMenuItemIsEnabled(const std::string& id, bool setting);
  void SetMenuItemKeyBinding(const std::string& id, const std::string& setting);
  void CreateToplevelMenu(const std::string& id, const std::string& label);
  void CreatePopupMenu(const std::string& id);

  void HandleCreateMenuItems(const base::DictionaryValue* input);
  void HandleDelMenuItem(const base::DictionaryValue* input);
  void HandleAddMenuToplevel(const base::DictionaryValue* input);
  void HandleAddMenuContext(const base::DictionaryValue* input);
  void HandleSetMenuItemEnabled(const base::DictionaryValue* input);
  void HandleSetMenuItemKeyBinding(const base::DictionaryValue* input);
  void HandlePopUpMenu(const base::DictionaryValue* input);

  static void OnActivateMenuItem(GtkMenuItem*, gpointer);

  MenuExtension* menu_extension_;
  GtkWidget* menu_;
  GtkAccelGroup* accel_group_;

  typedef std::map<std::string, GtkWidget*> WidgetMap;
  WidgetMap menus_;
  WidgetMap menu_items_;

  typedef std::map<std::string, GSList*> RadioGroupsMap;
  RadioGroupsMap radio_groups_;
};

}  // namespace cameo

#endif  // CAMEO_SRC_MENU_MENU_EXTENSION_H_
