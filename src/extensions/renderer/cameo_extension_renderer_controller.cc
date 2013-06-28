// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cameo/src/extensions/renderer/cameo_extension_renderer_controller.h"

#include "cameo/src/extensions/common/cameo_extension_messages.h"
#include "cameo/src/extensions/renderer/cameo_extension_render_view_handler.h"
#include "content/public/renderer/render_thread.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebScriptSource.h"
#include "v8/include/v8.h"

namespace cameo {
namespace extensions {

class CameoExtensionV8Wrapper : public v8::Extension {
 public:
  CameoExtensionV8Wrapper();

  // v8::Extension implementation.
  virtual v8::Handle<v8::FunctionTemplate> GetNativeFunction(
      v8::Handle<v8::String> name);

 private:
  static v8::Handle<v8::Value> PostMessage(const v8::Arguments& args);
};

// FIXME(cmarcelo): Implement this in C++ instead of JS. Tie it to the
// window object lifetime.
static const char* const kCameoExtensionV8WrapperAPI =
    "var cameo = cameo || {};"
    "cameo.postMessage = function(extension, msg) {"
    "  native function PostMessage();"
    "  PostMessage(extension, msg);"
    "};"
    "cameo._message_listeners = {};"
    "cameo.setMessageListener = function(extension, callback) {"
    "  if (callback === undefined)"
    "    delete cameo._message_listeners[extension];"
    "  else"
    "    cameo._message_listeners[extension] = callback;"
    "};"
    "cameo.onpostmessage = function(extension, msg) {"
    "  var listener = cameo._message_listeners[extension];"
    "  if (listener !== undefined)"
    "    listener(msg);"
    "};"
    "cameo.registerApiGroup = function(group, extension, apis) {"
    "  cameo[extension] = cameo[extension] || {};"
    "  cameo[extension]._callbacks = cameo[extension]._callbacks || {};"
    "  cameo[extension]._next_reply_id = cameo[extension]._next_reply_id || 0;"
    "  cameo.setMessageListener(extension, function(json) {"
    "    var msg = JSON.parse(json);"
    "    var reply_id = msg._reply_id;"
    "    var callback = cameo[extension]._callbacks[reply_id];"
    "    if (callback) {"
    "      delete msg._reply_id;"
    "      delete cameo[extension]._callbacks[reply_id];"
    "      callback(msg);"
    "    } else {"
    "      console.log('Invalid reply_id received from ' + extension + ' extension: ' + reply_id);"
    "    }"
    "  });"
    "  for(var api in apis){"
    "    group[api] = function () {"
    "      var msg = { cmd: this[0] };"
    "      var callback;"
    "      var cbFields=[];"
    "      var i;"
    "      for (i = 0; i < arguments.length; i ++) {"
    "        var arg = arguments[i];"
    "        if (typeof arg === 'function') {"
    "          callback = arg;"
    "          cbFields = this[i + 1];"
    "        }"
    "        else {"
    "          msg[this[i + 1]] = arg;"
    "        }"
    "      }"
    "      var reply_id = cameo[extension]._next_reply_id;"
    "      cameo[extension]._next_reply_id += 1;"
    "      msg._reply_id = reply_id.toString();"
    "      cameo[extension]._callbacks[reply_id] = function(r) {"
    "        var cbArgs = [];"
    "        if  (typeof cbFields === 'function')"
    "          cbFields.call(null, r, callback);"
    "        else {"
    "          for (i = 0; i < cbFields.length; i ++) {"
    "            cbArgs.push(r[cbFields[i]]);"
    "          }"
    "          callback.apply(null, cbArgs);"
    "        }"
    "      };"
    "      cameo.postMessage(extension, JSON.stringify(msg));"
    "    }.bind(apis[api]);"
    "  }"
    "};";

CameoExtensionV8Wrapper::CameoExtensionV8Wrapper()
    : v8::Extension("cameo", kCameoExtensionV8WrapperAPI) {
}

v8::Handle<v8::FunctionTemplate>
CameoExtensionV8Wrapper::GetNativeFunction(v8::Handle<v8::String> name) {
  if (name->Equals(v8::String::New("PostMessage")))
    return v8::FunctionTemplate::New(PostMessage);
  return v8::Handle<v8::FunctionTemplate>();
}

v8::Handle<v8::Value> CameoExtensionV8Wrapper::PostMessage(
    const v8::Arguments& args) {
  if (args.Length() != 2)
    return v8::False();

  std::string extension(*v8::String::Utf8Value(args[0]));
  std::string msg(*v8::String::Utf8Value(args[1]));

  CameoExtensionRenderViewHandler* handler =
      CameoExtensionRenderViewHandler::GetForCurrentContext();
  if (!handler->PostMessageToExtension(extension, msg))
    return v8::False();
  return v8::True();
}

CameoExtensionRendererController::CameoExtensionRendererController() {
  content::RenderThread* thread = content::RenderThread::Get();
  thread->AddObserver(this);
  thread->RegisterExtension(new CameoExtensionV8Wrapper);
}

CameoExtensionRendererController::~CameoExtensionRendererController() {
  content::RenderThread::Get()->RemoveObserver(this);
}

void CameoExtensionRendererController::RenderViewCreated(
    content::RenderView* render_view) {
  // RenderView will own this object.
  new CameoExtensionRenderViewHandler(render_view, this);
}

bool CameoExtensionRendererController::OnControlMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(CameoExtensionRendererController, message)
    IPC_MESSAGE_HANDLER(CameoViewMsg_RegisterExtension, OnRegisterExtension)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void CameoExtensionRendererController::OnRegisterExtension(
    const std::string& extension, const std::string& api) {
  extension_apis_[extension] = api;
}

bool CameoExtensionRendererController::ContainsExtension(
    const std::string& extension) const {
  return extension_apis_.find(extension) != extension_apis_.end();
}

void CameoExtensionRendererController::InstallJavaScriptAPIs(
    WebKit::WebFrame* frame) {
  ExtensionAPIMap::const_iterator it = extension_apis_.begin();
  for (; it != extension_apis_.end(); ++it) {
    const std::string& api_code = it->second;
    if (!api_code.empty())
      frame->executeScript(WebKit::WebScriptSource(
          WebKit::WebString::fromUTF8(api_code)));
  }
}

}  // namespace extensions
}  // namespace cameo
