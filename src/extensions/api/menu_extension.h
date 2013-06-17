// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAMEO_SRC_EXTENSIONS_API_MENU_EXTENSION_H_
#define CAMEO_SRC_EXTENSIONS_API_MENU_EXTENSION_H_

#include <gtk/gtk.h>
#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "cameo/src/extensions/browser/cameo_extension.h"

namespace base {
class DictionaryValue;
};

namespace cameo {
namespace extensions {
namespace menu {

class Extension;

enum MenuItemType {
  kRegularItem,
  kRadioItem,
  kCheckItem,
  kSeparatorItem
};

class ExtensionContext {
 private:
  Extension* extension_;
  const int32_t id_;
  GtkWidget* menu_;
  GtkAccelGroup* accel_group_;
  std::map<std::string, GSList*> radio_groups_;
  std::map<std::string, GtkWidget*> menus_;
  std::map<std::string, GtkWidget*> menu_items_;

  MenuItemType ConvertMenuItemType(const std::string& item_type);

  GtkWidget* FindMenu(const std::string& id);
  GtkWidget* FindMenuItem(const std::string& id);

  void AddMenuItem(const std::string& menu_id, const std::string& id,
       const std::string& label, MenuItemType type,
       const std::string& group);
  void PopUpMenu(const std::string& id);
  void DeleteMenu(const std::string& id);
  void DeleteMenuItem(const std::string& id);
  void SetMenuItemIsEnabled(const std::string& id, bool setting);
  void SetMenuItemKeyBinding(const std::string& id,
       const std::string& setting);
  void CreateToplevelMenu(const std::string& id, const std::string& label);
  void CreatePopupMenu(const std::string& id);

  static void OnActivateMenuItemStatic(GtkMenuItem*, gpointer);
  void OnActivateMenuItem(const char* item_id);

 public:
  explicit ExtensionContext(Extension* extension, const int32_t id);
  ~ExtensionContext();

  void HandleCreateMenuItems(const base::DictionaryValue* input);
  void HandleDelMenuItem(const base::DictionaryValue* input);
  void HandleAddMenuToplevel(const base::DictionaryValue* input);
  void HandleAddMenuContext(const base::DictionaryValue* input);
  void HandleSetMenuItemEnabled(const base::DictionaryValue* input);
  void HandleSetMenuItemKeyBinding(const base::DictionaryValue* input);
  void HandlePopUpMenu(const base::DictionaryValue* input);

};

class Extension : public cameo::extensions::CameoExtension {
 private:
  std::map<const int32_t, ExtensionContext*> contexts_;
  ExtensionContext* GetExtensionContextForId(const int32_t id);

 public:
  explicit Extension(cameo::extensions::CameoExtension::Poster* poster);
  ~Extension();

  virtual const char* GetJavaScriptAPI() OVERRIDE;
  virtual void HandleMessage(const int32_t id, const std::string& msg) OVERRIDE;
};

}  // namespace menu
}  // namespace extensions
}  // namespace cameo

#endif  // CAMEO_SRC_EXTENSIONS_API_MENU_EXTENSION_H_
