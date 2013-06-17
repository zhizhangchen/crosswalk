// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cameo/src/extensions/api/menu_extension.h"

#include <gtk/gtk.h>
#include <string>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"

#include "cameo/src/runtime/browser/cameo_browser_main_parts.h"
#include "cameo/src/runtime/browser/cameo_content_browser_client.h"
#include "cameo/src/runtime/browser/runtime.h"
#include "cameo/src/runtime/browser/ui/native_app_window.h"

using cameo::extensions::CameoExtension;
using cameo::NativeAppWindow;
using cameo::CameoContentBrowserClient;

namespace cameo {
namespace extensions {
namespace menu {

// This will be generated from menu_api.js.
#include "menu_api.h"

ExtensionContext::ExtensionContext(Extension* extension, const int32_t id)
    : extension_(extension)
    , id_(id) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  CameoBrowserMainParts* parts =
            CameoContentBrowserClient::GetBrowserMainParts();
  CHECK(parts != NULL);

  GtkWindow* window = parts->runtime()->window()->GetNativeWindow();
  CHECK(window);

  GtkWidget* vbox = NULL;
  GList* children = gtk_container_get_children(GTK_CONTAINER(window));
  for (GList *iter = children; iter; iter = iter->next) {
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
  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group_);
}

ExtensionContext::~ExtensionContext()
{
  std::map<std::string, GtkWidget *>::iterator iter;
  for (iter = menus_.begin(); iter != menus_.end(); ++iter) {
    // A popup menu has no parent widget
    if (!gtk_widget_get_parent(iter->second))
      gtk_widget_destroy(iter->second);
  }
  gtk_widget_destroy(menu_);
  g_object_unref(accel_group_);
}

GtkWidget* ExtensionContext::FindMenu(const std::string& id) {
  std::map<std::string, GtkWidget*>::iterator iter = menus_.find(id);
  if (iter == menus_.end())
    return NULL;
  return iter->second;
}

GtkWidget* ExtensionContext::FindMenuItem(const std::string& id) {
  std::map<std::string, GtkWidget*>::iterator iter = menu_items_.find(id);
  if (iter == menu_items_.end())
    return NULL;
  return iter->second;
}

void ExtensionContext::OnActivateMenuItemStatic(GtkMenuItem *menu_item, gpointer user_data) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  ExtensionContext* context = reinterpret_cast<ExtensionContext*>(user_data);
  const char* name = gtk_widget_get_name(GTK_WIDGET(menu_item));
  if (!name) {
    VLOG(0) << "widget has no name";
    return;
  }
  context->OnActivateMenuItem(name);
}

void ExtensionContext::OnActivateMenuItem(const char* item_id) {
  scoped_ptr<base::DictionaryValue> output(new base::DictionaryValue);
  std::string msg;

  output->SetString("cmd", "activate");
  output->SetString("item_id", item_id);

  base::JSONWriter::Write(output.get(), &msg)
  extension_->PostMessage(id_, msg);
}

void ExtensionContext::AddMenuItem(const std::string& menu_id, const std::string& id,
    const std::string& label, MenuItemType type, const std::string& group) {
  GtkWidget* menu = FindMenu(menu_id);
  if (!menu)
    return;

  GtkWidget* item;
  if (type == kSeparatorItem) {
    item = gtk_separator_menu_item_new();
  } else if (type == kCheckItem) {
    item = gtk_check_menu_item_new_with_label(label.c_str());
  } else if (type == kRadioItem) {
    std::map<std::string, GSList*>::iterator iter = radio_groups_.find(group);
    GSList* group_list = iter != radio_groups_.end() ? iter->second : NULL;

    item = gtk_radio_menu_item_new_with_label(group_list, label.c_str());
    radio_groups_[group] = g_slist_prepend(group_list, item);
  } else {
    item = gtk_menu_item_new_with_label(label.c_str());
  }

  g_signal_connect(G_OBJECT(item), "activate",
                      G_CALLBACK(OnActivateMenuItemStatic), this);

  gtk_widget_set_name(item, id.c_str());
  gtk_widget_show(item);

  GtkWidget* submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu));
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  menu_items_[id] = item;
}

void ExtensionContext::PopUpMenu(const std::string& id) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  GtkWidget* menu = FindMenu(id);
  if (!menu)
    return;

  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0,
                 gtk_get_current_event_time());
}

void ExtensionContext::DeleteMenu(const std::string& id) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  GtkWidget* menu = FindMenu(id);
  if (!menu)
    return;

  gtk_widget_destroy(menu);
  menus_.erase(id);
}

void ExtensionContext::DeleteMenuItem(const std::string& id) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  GtkWidget* item = FindMenuItem(id);
  if (item)
    gtk_widget_destroy(item);
}

void ExtensionContext::SetMenuItemIsEnabled(const std::string& id, bool setting) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  GtkWidget* item = FindMenuItem(id);
  if (item)
    gtk_widget_set_sensitive(item, setting);
}

void ExtensionContext::SetMenuItemKeyBinding(const std::string& id,
    const std::string& setting) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

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

void ExtensionContext::CreateToplevelMenu(const std::string& id, const std::string& label) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  GtkWidget* item = gtk_menu_item_new_with_label(label.c_str());
  gtk_widget_set_name(item, id.c_str());
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), item);
  gtk_widget_show(item);

  GtkWidget* submenu = gtk_menu_new();
  gtk_widget_show(submenu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

  menus_[id] = item;
}

void ExtensionContext::CreatePopupMenu(const std::string& id) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  GtkWidget* menu = gtk_menu_new();
  menus_[id] = menu;
}

MenuItemType ExtensionContext::ConvertMenuItemType(const std::string& item_type) {
  if (item_type == "separator")
    return kSeparatorItem;
  if (item_type == "check")
    return kCheckItem;
  if (item_type == "radio")
    return kRadioItem;
  return kRegularItem;
}

void ExtensionContext::HandleCreateMenuItems(const base::DictionaryValue* input) {
  // FIXME: Should we have a AddMenuItems method on NativeAppWindow?
  std::string menu_id;
  input->GetString("menu_id", &menu_id);

  const base::ListValue* items;
  if (!input->GetList("items", &items))
    return;
  for (unsigned index = 0; index < items->GetSize(); index++) {
    const base::DictionaryValue* item;

    if (!items->GetDictionary(index, &item))
      continue;

    std::string id;
    item->GetString("id", &id);

    std::string label;
    item->GetString("label", &label);

    std::string item_type;
    item->GetString("type", &item_type);

    std::string group;
    item->GetString("group", &group);

    AddMenuItem(menu_id, id, label, ConvertMenuItemType(item_type), group);
  }
}

void ExtensionContext::HandleDelMenuItem(const base::DictionaryValue* input) {
  std::string id;
  input->GetString("id", &id);

  DeleteMenuItem(id);
}

void ExtensionContext::HandleAddMenuToplevel(const base::DictionaryValue* input) {
  std::string id;
  input->GetString("id", &id);

  std::string label;
  input->GetString("label", &label);

  CreateToplevelMenu(id, label);
}

void ExtensionContext::HandleAddMenuContext(const base::DictionaryValue* input) {
  std::string id;
  input->GetString("id", &id);

  CreatePopupMenu(id);
}

void ExtensionContext::HandleSetMenuItemEnabled(const base::DictionaryValue* input) {
  std::string id;
  input->GetString("id", &id);

  bool setting;
  input->GetBoolean("setting", &setting);

  SetMenuItemIsEnabled(id, setting);
}

void ExtensionContext::HandleSetMenuItemKeyBinding(const base::DictionaryValue* input) {
  std::string id;
  input->GetString("id", &id);

  std::string setting;
  input->GetString("setting", &setting);

  SetMenuItemKeyBinding(id, setting);
}

void ExtensionContext::HandlePopUpMenu(const base::DictionaryValue* input) {
  std::string id;
  input->GetString("id", &id);

  PopUpMenu(id);
}

Extension::Extension(CameoExtension::Poster* poster)
    : CameoExtension(poster) {
  name_ = "cameo.menu";
}

const char* Extension::GetJavaScriptAPI() {
  return kGeneratedSource;
}

ExtensionContext* Extension::GetExtensionContextForId(const int32_t id) {
  std::map<const int32_t, ExtensionContext*>::iterator iter = contexts_.find(id);
  if (iter == contexts_.end()) {
    ExtensionContext *new_context = new ExtensionContext(this, id);
    contexts_[id] = new_context;
    return new_context;
  }
  return iter->second;
}

void Extension::HandleMessage(const int32_t id, const std::string& msg) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  scoped_ptr<base::Value> v(base::JSONReader().ReadToValue(msg));
  base::DictionaryValue* input = static_cast<base::DictionaryValue*>(v.get());
  if (!input) {
    VLOG(0) << "Invalid JSON passed to HandleMessage";
    return;
  }

  std::string cmd;
  input->GetString("cmd", &cmd);

  ExtensionContext* context = GetExtensionContextForId(id);
  if (cmd == "CreateMenuItems") {
    context->HandleCreateMenuItems(input);
  } else if (cmd == "DelMenuItem") {
    context->HandleDelMenuItem(input);
  } else if (cmd == "AddMenuToplevel") {
    context->HandleAddMenuToplevel(input);
  } else if (cmd == "AddMenuContext") {
    context->HandleAddMenuContext(input);
  } else if (cmd == "SetMenuItemEnabled") {
    context->HandleSetMenuItemEnabled(input);
  } else if (cmd == "SetMenuItemKeyBinding") {
    context->HandleSetMenuItemKeyBinding(input);
  } else if (cmd == "PopUpMenu") {
    context->HandlePopUpMenu(input);
  } else {
    VLOG(0) << "Unhandled command " << cmd;
    return;
  }
}

Extension::~Extension()
{
  std::map<const int32_t, ExtensionContext*>::iterator iter;
  for (iter = contexts_.begin(); iter != contexts_.end(); iter++)
    delete iter->second;
}

}  // namespace menu
}  // namespace extensions
}  // namespace cameo
