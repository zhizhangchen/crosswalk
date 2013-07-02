// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAMEO_MENU_MENU_EXTENSION_H_
#define CAMEO_MENU_MENU_EXTENSION_H_

#include <gtk/gtk.h>
#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "cameo/extensions/browser/cameo_extension.h"
#include "cameo/runtime/browser/runtime.h"
#include "cameo/runtime/browser/runtime_registry.h"
#include "ui/gfx/native_widget_types.h"

namespace base {
class DictionaryValue;
};

using cameo::extensions::CameoExtension;

namespace cameo {

class MenuContext;

class MenuExtension : public CameoExtension,
                      public RuntimeRegistryObserver {
 public:
  explicit MenuExtension(RuntimeRegistry* runtime_registry);
  virtual ~MenuExtension();

  // CameoExtension implementation.
  virtual const char* GetJavaScriptAPI() OVERRIDE;
  virtual Context* CreateContext(
      const PostMessageCallback& post_message) OVERRIDE;

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

  void HandleAddMenu(const base::DictionaryValue* input);
  void HandleAddMenuItem(const base::DictionaryValue* input);
  void HandleSetMenuTitle(const base::DictionaryValue* input);
  void HandleSetMenuItemState(const base::DictionaryValue* input);
  void HandleSetMenuItemShortcut(const base::DictionaryValue* input);
  void HandleRemoveMenu(const base::DictionaryValue* input);
  void HandleRemoveMenuItem(const base::DictionaryValue* input);

  static void OnActivateMenuItem(GtkMenuItem*, gpointer);

  GtkWidget* FindMenu(const std::string& id);
  GtkWidget* FindMenuItem(const std::string& id);
  GtkWidget* ConvertToCheckMenuItem(const std::string& id, GtkWidget* item);
  void ConnectActivateSignal(GtkWidget* item);
  void DisconnectActivateSignal(GtkWidget* item);

  MenuExtension* menu_extension_;
  GtkWidget* menu_;

  typedef std::map<std::string, GtkWidget*> WidgetMap;
  WidgetMap menus_;
  WidgetMap menu_items_;
};

}  // namespace cameo

#endif  // CAMEO_MENU_MENU_EXTENSION_H_
