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
#include "menu_api.h"  // NOLINT(*)

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

MenuContext::MenuContext(
    MenuExtension* menu_extension,
    const CameoExtension::PostMessageCallback& post_message)
    : CameoExtension::Context(post_message),
      menu_extension_(menu_extension),
      menu_(NULL) {
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

  if (cmd == "AddMenu")
    HandleAddMenu(input);
  else if (cmd == "AddMenuItem")
    HandleAddMenuItem(input);
  else if (cmd == "SetMenuTitle")
    HandleSetMenuTitle(input);
  else if (cmd == "SetMenuItemState")
    HandleSetMenuItemState(input);
  else if (cmd == "SetMenuItemShortcut")
    HandleSetMenuItemShortcut(input);
  else if (cmd == "RemoveMenu")
    HandleRemoveMenu(input);
  else if (cmd == "RemoveMenuItem")
    HandleRemoveMenuItem(input);
  else if (cmd == "Show")
    gtk_widget_show(menu_);
  else
    VLOG(0) << "Unhandled command " << cmd;
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
  gtk_box_pack_start(GTK_BOX(vbox), menu_, FALSE, TRUE, 0);
  gtk_box_reorder_child(GTK_BOX(vbox), menu_, 0);

  // Note: we only show the menu when it's used for the first
  // time. See the message with command "Show".
}

#define GET_STRING(INPUT, NAME)                 \
  std::string NAME;                             \
  INPUT->GetString(#NAME, &NAME)

#define GET_BOOLEAN(INPUT, NAME)                \
  bool NAME;                                    \
  INPUT->GetBoolean(#NAME, &NAME);

static int FindItemPositionInMenu(GtkMenuShell* shell, GtkWidget* item) {
  int position = 0;
  GList* children = gtk_container_get_children(GTK_CONTAINER(shell));
  for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
    if (iter->data == item)
      return position;
    position++;
  }
  return -1;
}

static void InsertItemIntoPosition(GtkMenuShell* shell, GtkWidget* item,
                                   const std::string& position,
                                   GtkWidget* relative_item) {
  if (position == "first") {
    gtk_menu_shell_prepend(shell, item);
    return;
  }

  if (position == "before") {
    int pos = FindItemPositionInMenu(shell, relative_item);
    if (pos != -1) {
      gtk_menu_shell_insert(shell, item, pos);
      return;
    }
  } else if (position == "after") {
    int pos = FindItemPositionInMenu(shell, relative_item);
    if (pos != -1) {
      gtk_menu_shell_insert(shell, item, pos + 1);
      return;
    }
  }

  gtk_menu_shell_append(shell, item);
}

void MenuContext::HandleAddMenu(const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, id);
  GET_STRING(input, title);
  GET_STRING(input, position);
  GET_STRING(input, relative_id);

  if (menus_.find(id) != menus_.end()) {
    LOG(WARNING) << "cameo.menu.addMenu(): ignoring duplicate menu with id ='"
                 << id << "'";
    return;
  }

  GtkWidget* relative_item = NULL;
  if (!relative_id.empty())
    relative_item = FindMenu(relative_id);

  GtkWidget* item = gtk_menu_item_new_with_label(title.c_str());
  gtk_widget_set_name(item, id.c_str());
  InsertItemIntoPosition(GTK_MENU_SHELL(menu_), item, position, relative_item);
  gtk_widget_show(item);

  GtkWidget* submenu = gtk_menu_new();
  gtk_widget_show(submenu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

  menus_[id] = item;
}

void MenuContext::HandleAddMenuItem(const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, id);
  GET_STRING(input, title);
  GET_STRING(input, parent_id);
  GET_STRING(input, key);
  GET_STRING(input, display_str);
  GET_STRING(input, position);
  GET_STRING(input, relative_id);

  GtkWidget* menu = FindMenu(parent_id);
  if (!menu)
    return;

  GtkWidget* item = NULL;

  if (title == "---") {
    item = gtk_separator_menu_item_new();
  } else {
    item = gtk_menu_item_new_with_label(title.c_str());
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(OnActivateMenuItem), this);
  }
  gtk_widget_set_name(item, id.c_str());
  gtk_widget_show(item);

  GtkWidget* relative_item = NULL;
  if (!relative_id.empty())
    relative_item = FindMenuItem(relative_id);

  GtkWidget* submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu));
  InsertItemIntoPosition(
      GTK_MENU_SHELL(submenu), item, position, relative_item);

  menu_items_[id] = item;
}

void MenuContext::HandleSetMenuTitle(const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, id);
  GET_STRING(input, title);
  GtkWidget* item = FindMenu(id);
  if (!item)
    item = FindMenuItem(id);
  if (!item) {
    LOG(WARNING) << "cameo.menu.setMenuTitle(): can't find menu with id='"
                 << id << "'";
    return;
  }
  gtk_menu_item_set_label(GTK_MENU_ITEM(item), title.c_str());
}

void MenuContext::HandleSetMenuItemState(const base::DictionaryValue* input) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  GET_STRING(input, id);
  GET_BOOLEAN(input, enabled);
  GET_BOOLEAN(input, checked);
  GtkWidget* item = FindMenuItem(id);
  if (!item) {
    LOG(WARNING)
        << "cameo.menu.setMenuItemState(): can't find menu item with id='"
        << id << "'";
    return;
  }

  // First time item is mark as checked, we convert it to CheckMenuItem.
  if (checked && !GTK_IS_CHECK_MENU_ITEM(item))
    item = ConvertToCheckMenuItem(id, item);

  if (GTK_IS_CHECK_MENU_ITEM(item)) {
    g_signal_handlers_disconnect_by_data(G_OBJECT(item), this);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), checked);
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(OnActivateMenuItem), this);
  }
  gtk_widget_set_sensitive(item, enabled);
}

void MenuContext::HandleSetMenuItemShortcut(
    const base::DictionaryValue* input) {
  GET_STRING(input, id);
  GET_STRING(input, shortcut);
  GET_STRING(input, display_str);
  VLOG(0) << "NOT IMPLEMENTED: Set shortcut"
          << " id='" << id << "'"
          << " shortcut='" << shortcut << "'"
          << " display_str='" << display_str << "'";
}

void MenuContext::HandleRemoveMenu(const base::DictionaryValue* input) {
  GET_STRING(input, id);
  GtkWidget* menu = FindMenu(id);
  if (!menu) {
    LOG(WARNING) << "cameo.menu.removeMenu(): "
                 << "can't remove non-existent menu with id ='" << id << "'";
    return;
  }
  gtk_widget_destroy(menu);
  menus_.erase(id);
}

void MenuContext::HandleRemoveMenuItem(const base::DictionaryValue* input) {
  GET_STRING(input, id);
  GtkWidget* item = FindMenuItem(id);
  if (!item) {
    LOG(WARNING) << "cameo.menu.removeMenuItem(): "
                 << "can't remove non-existent menu item with id ='"
                 << id << "'";
    return;
  }
  gtk_widget_destroy(item);
  menu_items_.erase(id);
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

GtkWidget* MenuContext::ConvertToCheckMenuItem(const std::string& id,
                                               GtkWidget* item) {
  GtkWidget* menu = gtk_widget_get_parent(item);
  int pos = FindItemPositionInMenu(GTK_MENU_SHELL(menu), item);

  GtkWidget* new_item = gtk_check_menu_item_new_with_label(
      gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
  gtk_widget_set_name(new_item, id.c_str());
  // FIXME(cmarcelo): Copy accel key to item when we have one.
  ConnectActivateSignal(new_item);

  gtk_widget_destroy(item);
  gtk_menu_shell_insert(GTK_MENU_SHELL(menu), new_item, pos);
  gtk_widget_show(new_item);

  menu_items_[id] = new_item;
  return new_item;
}

void MenuContext::ConnectActivateSignal(GtkWidget* item) {
  g_signal_connect(G_OBJECT(item), "activate",
                   G_CALLBACK(OnActivateMenuItem), this);
}

void MenuContext::DisconnectActivateSignal(GtkWidget* item) {
  g_signal_handlers_disconnect_by_data(G_OBJECT(item), this);
}

}  // namespace cameo
