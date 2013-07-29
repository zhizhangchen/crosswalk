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
static CXWalkExtensionContext* _context;
static uv_cond_t js_cond;
static uv_mutex_t js_mutex;
static uv_mutex_t message_mutex;
static std::queue<std::string> messages;

static v8::Handle<v8::Value> on_message_received(const v8::Arguments& args) {
  HandleScope scope(node_isolate);

  String::AsciiValue ascii(args[0]);

  xwalk_extension_context_post_message(_context, *ascii);

}
static std::string jsSource;
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
    Local<Value> args[] = { String::New("message"), String::New("brackets"), String::New(message.c_str())};
    MakeCallback(process, "emit", ARRAY_SIZE(args), args);
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


void NodeExtensionContext::HandleMessage(CXWalkExtensionContext* self,
                                             const char* message) {
  _context = self;
  uv_mutex_lock(&message_mutex);
  messages.push(std::string(message));
  uv_mutex_unlock(&message_mutex);
  // uv_async_send may happen only once for several messages, but it
  // is guranteed that the handler will be called at least once AFTER
  // uv_async_send.
  uv_async_send(&async);
  return;
}

void NodeExtensionContext::Destroy(CXWalkExtensionContext* self) {
  delete reinterpret_cast<NodeExtensionContext*>(self);
}

NodeExtensionContext::NodeExtensionContext(NodeExtension *extension)
      : extension_(extension) {
  destroy = &Destroy;
  handle_message = &HandleMessage;
}

void NodeExtension::Shutdown(CXWalkExtension* self) {
  delete reinterpret_cast<NodeExtension*>(self);
}

const char* NodeExtension::GetJavascript(CXWalkExtension*) {
  uv_mutex_lock(&js_mutex);
  while (jsSource.length() == 0)
    uv_cond_wait(&js_cond, &js_mutex);
  jsSource = kGeneratedSource + jsSource;
  uv_mutex_unlock(&js_mutex);
  return jsSource.c_str();
}

CXWalkExtensionContext* NodeExtension::CreateContext(CXWalkExtension* self) {
  NodeExtension* extension = reinterpret_cast<NodeExtension*>(self);
  NodeExtensionContext* context = new NodeExtensionContext(extension);

  return reinterpret_cast<CXWalkExtensionContext*>(context);
}

NodeExtension::NodeExtension() {
  name = "node";
  api_version = 1;
  get_javascript = &GetJavascript;
  shutdown = &Shutdown;
  context_create = &CreateContext;

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
}

extern "C" CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  if (api_version < 1)
    return NULL;
  return new NodeExtension();
}

