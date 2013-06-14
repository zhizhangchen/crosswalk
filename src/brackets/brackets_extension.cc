// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cameo/src/brackets/brackets_extension.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "cameo/src/brackets/brackets_platform.h"
#include <iostream>

using cameo::extensions::CameoExtension;

namespace brackets {

// This will be generated from brackets_api.js.
#include "brackets_api.h"

BracketsExtension::BracketsExtension(CameoExtension::Poster* poster)
    : CameoExtension(poster, "brackets", CameoExtension::HANDLER_THREAD_UI) {
}

const char* BracketsExtension::GetJavaScriptAPI() {
  return kGeneratedSource;
}

int JSErrorFromPlatformError(platform::ErrorCode error_code) {
  // This works because brackets_platform.h is in sync with
  // brackets.fs.* error codes.
  return int(error_code);
}

void HandleReadFile(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);
  std::string encoding;
  input->GetString("encoding", &encoding);

  std::string data;
  platform::ErrorCode error = platform::ReadFile(path, encoding, data);

  output->SetInteger("error", JSErrorFromPlatformError(error));
  if (error == platform::kNoError)
    output->SetString("data", data);
}

void HandleGetFileModificationTime(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);

  time_t mtime;
  bool is_dir;
  platform::ErrorCode error =
      platform::GetFileModificationTime(path, mtime, is_dir);

  output->SetInteger("error", JSErrorFromPlatformError(error));
  if (error == platform::kNoError) {
    output->SetInteger("modtime", mtime);
    output->SetBoolean("is_dir", is_dir);
  }
}

void HandleReadDir(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);

  std::vector<std::string> result;
  platform::ErrorCode error = platform::ReadDir(path, result);

  output->SetInteger("error", JSErrorFromPlatformError(error));
  base::ListValue* l = new base::ListValue;
  l->AppendStrings(result);
  output->Set("files", l);
}

void HandleWriteFile(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);
  std::string data;
  input->GetString("data", &data);
  std::string encoding;
  input->GetString("encoding", &encoding);

  platform::ErrorCode error = platform::WriteFile(path, encoding, data);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

void HandleOpenLiveBrowser(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string url;
  input->GetString("url", &url);

  platform::ErrorCode error = platform::OpenLiveBrowser(url);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

void HandleOpenURLInDefaultBrowser(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string url;
  input->GetString("url", &url);

  platform::ErrorCode error = platform::OpenURLInDefaultBrowser(url);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

void HandleRename(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string old_path;
  input->GetString("oldPath", &old_path);

  std::string new_path;
  input->GetString("newPath", &new_path);

  platform::ErrorCode error = platform::Rename(old_path, new_path);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

void HandleShowOpenDialog(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string initial_path;
  input->GetString("initialPath", &initial_path);

  std::vector<std::string> result;
  platform::ErrorCode error = platform::ShowOpenDialog(initial_path, result);

  output->SetInteger("error", JSErrorFromPlatformError(error));
  base::ListValue* l = new base::ListValue;
  l->AppendStrings(result);
  output->Set("files", l);
}

// FIXME(cmarcelo): Current implementation is not ideal since we are
// still running in the IO-thread (responsible for IPC) and we should
// not assume we can block at will here. It would be good to have an
// example of asynchronously posting the replies.
void BracketsExtension::HandleMessage(const int32_t render_view_id,
                                      const std::string& msg) {
    scoped_ptr<base::Value> v(base::JSONReader().ReadToValue(msg));
    base::DictionaryValue* input = static_cast<base::DictionaryValue*>(v.get());
    scoped_ptr<base::DictionaryValue> output(new base::DictionaryValue);

    std::string cmd;
    input->GetString("cmd", &cmd);

    typedef void (* HandleFunc)(const base::DictionaryValue*, base::DictionaryValue*);
    HandleFunc handler = NULL;
    if (cmd == "ReadFile")
      handler = HandleReadFile;
    else if (cmd == "GetFileModificationTime")
      handler = HandleGetFileModificationTime;
    else if (cmd == "ReadDir")
      handler = HandleReadDir;
    else if (cmd == "WriteFile")
      handler = HandleWriteFile;
    else if (cmd == "OpenLiveBrowser")
      handler = HandleOpenLiveBrowser;
    else if (cmd == "OpenURLInDefaultBrowser")
      handler = HandleOpenURLInDefaultBrowser;
    else if (cmd == "Rename")
      handler = HandleRename;
    else if (cmd == "ShowOpenDialog")
      handler = HandleShowOpenDialog;

    if (!handler) {
      VLOG(0) << "Unhandled command " << cmd;
      return;
    }

    handler(input, output.get());
    std::string result;
    base::JSONWriter::Write(output.get(), &result);
    PostMessage(render_view_id, result);
}

}  // namespace brackets


