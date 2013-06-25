// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cameo/src/menu/menu_extension.h"

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace cameo {

// This will be generated from menu_api.js.
#include "menu_api.h"

MenuExtension::MenuExtension(RuntimeRegistry* runtime_registry)
    : CameoExtension("cameo.menu"),
      runtime_registry_(runtime_registry),
      native_window_(NULL) {
  runtime_registry_->AddObserver(this);
}

MenuExtension::~MenuExtension() {
  runtime_registry_->RemoveObserver(this);
}

const char* MenuExtension::GetJavaScriptAPI() {
  return kGeneratedSource;
}

CameoExtension::Context* MenuExtension::CreateContext(
    const PostMessageCallback& post_message) {
  return new MenuContext(this, post_message);
}

void MenuExtension::OnRuntimeAdded(Runtime* runtime) {
  // FIXME(cmarcelo): We only support one runtime!
  if (native_window_)
    return;
  native_window_ = runtime->window()->GetNativeWindow();
}

MenuContext::MenuContext(MenuExtension* menu_extension,
                         const CameoExtension::PostMessageCallback& post_message)
    : CameoExtension::Context(post_message),
      menu_extension_(menu_extension),
      menu_(NULL),
      accel_group_(NULL) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&MenuContext::InitializeMenu, base::Unretained(this)));
}

MenuContext::~MenuContext() {
  WidgetMap::iterator iter;
  for (iter = menus_.begin(); iter != menus_.end(); ++iter) {
    // A popup menu has no parent widget.
    if (!gtk_widget_get_parent(iter->second))
      gtk_widget_destroy(iter->second);
  }
  gtk_widget_destroy(menu_);
  g_object_unref(accel_group_);
}

void MenuContext::HandleMessage(const std::string& msg) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&MenuContext::HandleMessage, base::Unretained(this), msg));
    return;
  }

  scoped_ptr<base::Value> v(base::JSONReader().ReadToValue(msg));
  base::DictionaryValue* input = static_cast<base::DictionaryValue*>(v.get());
  if (!input) {
    LOG(WARNING) << "Ignoring invalid JSON passed to cameo.menu extension.";
    return;
  }

  std::string cmd;
  input->GetString("cmd", &cmd);

  if (cmd == "CreateMenuItems") {
    HandleCreateMenuItems(input);
  } else if (cmd == "DelMenuItem") {
    HandleDelMenuItem(input);
  } else if (cmd == "AddMenuToplevel") {
    HandleAddMenuToplevel(input);
  } else if (cmd == "AddMenuContext") {
    HandleAddMenuContext(input);
  } else if (cmd == "SetMenuItemEnabled") {
    HandleSetMenuItemEnabled(input);
  } else if (cmd == "SetMenuItemKeyBinding") {
    HandleSetMenuItemKeyBinding(input);
  } else if (cmd == "PopUpMenu") {
    HandlePopUpMenu(input);
  } else {
    VLOG(0) << "Unhandled command " << cmd;
    return;
  }
}

void MenuContext::InitializeMenu() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  GtkWidget* vbox = NULL;
  GList* children = gtk_container_get_children(
      GTK_CONTAINER(menu_extension_->native_window_));
  for (GList* iter = children; iter; iter = iter->next) {
    if (GTK_IS_VBOX(iter->data)) {
      vbox = GTK_WIDGET(iter->data);
      break;
    }
  }
  g_list_free(children);
  CHECK(vbox);

  menu_ = gtk_menu_bar_new();
  gtk_widget_show(menu_);
  gtk_box_pack_start(GTK_BOX(vbox), menu_, FALSE, TRUE, 0);
  gtk_box_reorder_child(GTK_BOX(vbox), menu_, 0);

  accel_group_ = gtk_accel_group_new();
  gtk_window_add_accel_group(
      GTK_WINDOW(menu_extension_->native_window_), accel_group_);
}

#define GET_STRING(INPUT, NAME)                 \
  std::string NAME;                             \
  INPUT->GetString(#NAME, &NAME)

static MenuItemType ConvertMenuItemType(const std::string& item_type) {
  if (item_type == "separator")
    return kSeparatorItem;
  if (item_type == "check")
    return kCheckItem;
  if (item_type == "radio")
    return kRadioItem;
  return kRegularItem;
}

void MenuContext::HandleCreateMenuItems(const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, menu_id);

  const base::ListValue* items;
  if (!input->GetList("items", &items))
    return;

  for (unsigned index = 0; index < items->GetSize(); index++) {
    const base::DictionaryValue* item;
    if (!items->GetDictionary(index, &item))
      continue;
    GET_STRING(item, id);
    GET_STRING(item, label);
    GET_STRING(item, type);
    GET_STRING(item, group);

    AddMenuItem(menu_id, id, label, ConvertMenuItemType(type), group);
  }
}

void MenuContext::HandleDelMenuItem(const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, id);
  DeleteMenuItem(id);
}

void MenuContext::HandleAddMenuToplevel(const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, id);
  GET_STRING(input, label);
  CreateToplevelMenu(id, label);
}

void MenuContext::HandleAddMenuContext(const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, id);
  CreatePopupMenu(id);
}

void MenuContext::HandleSetMenuItemEnabled(const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, id);

  bool setting;
  input->GetBoolean("setting", &setting);

  SetMenuItemIsEnabled(id, setting);
}

void MenuContext::HandleSetMenuItemKeyBinding(
    const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, id);
  GET_STRING(input, setting);
  SetMenuItemKeyBinding(id, setting);
}

void MenuContext::HandlePopUpMenu(const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, id);
  PopUpMenu(id);
}

GtkWidget* MenuContext::FindMenu(const std::string& id) {
  WidgetMap::iterator iter = menus_.find(id);
  if (iter == menus_.end())
    return NULL;
  return iter->second;
}

GtkWidget* MenuContext::FindMenuItem(const std::string& id) {
  WidgetMap::iterator iter = menu_items_.find(id);
  if (iter == menu_items_.end())
    return NULL;
  return iter->second;
}

void MenuContext::AddMenuItem(
    const std::string& menu_id, const std::string& id, const std::string& label,
    MenuItemType type, const std::string& group) {
  GtkWidget* menu = FindMenu(menu_id);
  if (!menu)
    return;

  GtkWidget* item = NULL;
  switch (type) {
    case kSeparatorItem:
      item = gtk_separator_menu_item_new();
      break;
    case kCheckItem:
      item = gtk_check_menu_item_new_with_label(label.c_str());
      break;
    case kRadioItem: {
      RadioGroupsMap::iterator iter = radio_groups_.find(group);
      GSList* group_list = iter != radio_groups_.end() ? iter->second : NULL;
      item = gtk_radio_menu_item_new_with_label(group_list, label.c_str());
      radio_groups_[group] = g_slist_prepend(group_list, item);
      break;
    }
    case kRegularItem:
      item = gtk_menu_item_new_with_label(label.c_str());
      break;
  }

  g_signal_connect(G_OBJECT(item), "activate",
                   G_CALLBACK(OnActivateMenuItem), this);
  gtk_widget_set_name(item, id.c_str());
  gtk_widget_show(item);

  GtkWidget* submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu));
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  menu_items_[id] = item;
}

void MenuContext::PopUpMenu(const std::string& id) {
  GtkWidget* menu = FindMenu(id);
  if (!menu)
    return;
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0,
                 gtk_get_current_event_time());
}

void MenuContext::DeleteMenu(const std::string& id) {
  GtkWidget* menu = FindMenu(id);
  if (!menu)
    return;
  gtk_widget_destroy(menu);
  menus_.erase(id);
}

void MenuContext::DeleteMenuItem(const std::string& id) {
  GtkWidget* item = FindMenuItem(id);
  if (!item)
    return;
  gtk_widget_destroy(item);
}

void MenuContext::SetMenuItemIsEnabled(const std::string& id, bool setting) {
  GtkWidget* item = FindMenuItem(id);
  if (!item)
    return;
  gtk_widget_set_sensitive(item, setting);
}

void MenuContext::SetMenuItemKeyBinding(
    const std::string& id, const std::string& setting) {
  guint key;
  GdkModifierType mods;
  // GTK's accelerator format seems good enough to be used for platform-agnostic
  // implementation. See gtk_accelerator_parse() documentation for examples,
  // which includes "<Control>a" and "<Shift><Alt>F1".
  gtk_accelerator_parse(setting.c_str(), &key, &mods);
  if (key == 0 && mods == 0)
    return;

  GtkWidget* item = FindMenuItem(id);
  if (!item)
    return;

  gtk_widget_add_accelerator(item, "selected", accel_group_, key,
                             mods, GTK_ACCEL_VISIBLE);
}

void MenuContext::CreateToplevelMenu(const std::string& id,
                                     const std::string& label) {
  GtkWidget* item = gtk_menu_item_new_with_label(label.c_str());
  gtk_widget_set_name(item, id.c_str());
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), item);
  gtk_widget_show(item);

  GtkWidget* submenu = gtk_menu_new();
  gtk_widget_show(submenu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

  menus_[id] = item;
}

void MenuContext::CreatePopupMenu(const std::string& id) {
  GtkWidget* menu = gtk_menu_new();
  menus_[id] = menu;
}

// static
void MenuContext::OnActivateMenuItem(GtkMenuItem *menu_item,
                                     gpointer user_data) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  MenuContext* context = reinterpret_cast<MenuContext*>(user_data);
  const char* name = gtk_widget_get_name(GTK_WIDGET(menu_item));
  if (!name)
    return;
  context->PostMessage(name);
}

}  // namespace cameo
