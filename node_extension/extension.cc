// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>
#include <map>
#include <string>

#include "node_api.h"
#include "extension.h"

#include <node.h>
#include <node_internals.h>
#include "node_main.h"
#include <queue>
using namespace v8;
using namespace node;

static uv_idle_t idle;
static uv_async_t async;
static XW_Instance _instance;
static uv_cond_t js_cond;
static uv_mutex_t js_mutex;
static uv_mutex_t message_mutex;
static std::queue<std::string> messages;
static std::string jsSource;
static std::string rtValue;
static bool returned;

static v8::Handle<v8::Value> on_message_received(const v8::Arguments& args) {
  HandleScope scope(node_isolate);

  String::AsciiValue ascii(args[0]);

  g_messaging_interface->PostMessage(_instance, *ascii);

}
static void node_idle_handler(uv_idle_t* handle, int status) {
  node_isolate = Isolate::GetCurrent();
  NODE_SET_METHOD(process, "sendMessage", on_message_received);
  Local<Value> args[] = {String::New("node_extensions/")};
  String::AsciiValue ascii(MakeCallback(process, "getExtensionSource", ARRAY_SIZE(args), args));
  uv_mutex_lock(&js_mutex);
  jsSource = *ascii;
  uv_mutex_unlock(&js_mutex);
  uv_cond_broadcast(&js_cond);

  uv_idle_stop(&idle);
}

static void node_post_messages(uv_async_t *handle, int status /*UNUSED*/) {
    const char* message = (char*)handle->data;
  uv_mutex_lock(&message_mutex);
  while (!messages.empty()) {
    std::string message = messages.front();
    Local<Value> args[] = { String::New(message.c_str())};
    String::AsciiValue ascii(MakeCallback(process, "handleMessage", ARRAY_SIZE(args), args));
    uv_mutex_lock(&rt_mutex);
    rtValue = *ascii;
    returned = true;
    uv_mutex_unlock(&rt_mutex);
    messages.pop();
  }
  uv_mutex_unlock(&message_mutex);
}
static void start_node(void*) {
  uv_idle_init(uv_default_loop(), &idle);
  uv_idle_start(&idle, node_idle_handler);
  uv_async_init(uv_default_loop(), &async, node_post_messages);
  size_t size = strlen(kNodeMainSource) + 10;
  char* args = (char*)malloc(size);
  memset(args, 0, size);
  memcpy(args, "node\0-e\0", 8);
  memcpy(args + 8, kNodeMainSource, strlen(kNodeMainSource));
  char* argv[3];
  argv[0] = args;
  argv[1] = args + 5;
  argv[2] = args + 8;
  int argc = 3;
  // We have to call SetFatalErrorHandler first or node will crash
  V8::SetFatalErrorHandler(NULL);
  node::Start(3, argv);
  free(args);
}

namespace {
XW_Extension g_extension = 0;

const struct XW_CoreInterface_1* g_core_interface;
const struct XW_MessagingInterface_1* g_messaging_interface;
const XW_Internal_SyncMessagingInterface* g_sync_messaging_interface = NULL;

struct NodeExtension* g_node;
}

static void NodeHandleMessage(XW_Instance instance, const char* message) {
  _instance = instance;
  uv_mutex_lock(&message_mutex);
  messages.push(std::string(message));
  uv_mutex_unlock(&message_mutex);
  // uv_async_send may happen only once for several messages, but it
  // is guranteed that the handler will be called at least once AFTER
  // uv_async_send.
  uv_async_send(&async);
  return;
}

static void NodeHandleSyncMessage(XW_Instance instance, const char* message) {
  uv_mutex_lock(&rt_mutex);
  returned = false;
  NodeHandleMessage(self, message);
  while (!returned);
    uv_cond_wait(&rt_cond, &rt_mutex);
  g_sync_messaging_interface->SetSyncReply(instance, rtValue.c_str());
  uv_mutex_unlock(&rt_mutex);
}

static void NodeShutdown(XW_Extension extension) {
  assert(extension == g_extension);

  g_extension = 0;
}

extern "C" int32_t XW_Initialize(XW_Extension extension, XW_GetInterface get_interface) {
  static uv_thread_t node_id;
  uv_cond_init(&js_cond);
  uv_mutex_init(&js_mutex);
  uv_mutex_init(&message_mutex);
  if (node_id == 0) {
    uv_thread_create(&node_id, start_node, NULL);
  }
  else {
    printf("%s\n", "Warning: node thread already created!");
  }

  uv_mutex_lock(&js_mutex);
  while (jsSource.length() == 0)
    uv_cond_wait(&js_cond, &js_mutex);
  jsSource += kGeneratedSource ;
  uv_mutex_unlock(&js_mutex);

  assert(g_extension == 0);

  g_extension = extension;

  g_core_interface = reinterpret_cast<const XW_CoreInterface *>(
      get_interface(XW_CORE_INTERFACE));
  g_core_interface->SetExtensionName(g_extension, "node");
  g_core_interface->SetJavaScriptAPI(g_extension, jsSource.c_str());
  g_core_interface->RegisterShutdownCallback(g_extension, NodeShutdown);

  // No need to add callbacks for Instance creation and destruction because
  // there's no information that we need to keep on them, so we don't need to
  // add them for now. Note, that in the future, we may have to.

  g_messaging_interface = reinterpret_cast<const XW_MessagingInterface *>(
      get_interface(XW_MESSAGING_INTERFACE));
  g_messaging_interface->Register(g_extension, NodeHandleMessage);

  g_sync_messaging_interface =
      reinterpret_cast<const XW_Internal_SyncMessagingInterface*>(
          get_interface(XW_INTERNAL_SYNC_MESSAGING_INTERFACE));
  if (!g_sync_messaging_interface) {
    std::cerr <<
        "Can't initialize extension: error getting Messaging interface.\n";
    return XW_ERROR;
  }
  g_sync_messaging_interface->Register(extension, NodeHandleSyncMessage);

  g_node = new NodeExtension();

  return XW_OK;
}
