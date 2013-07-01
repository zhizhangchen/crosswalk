// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cameo/brackets/brackets_extension.h"

#include "base/values.h"
#include "cameo/brackets/brackets_context.h"

namespace brackets {

// This will be generated from brackets_api.js.
#include "brackets_api.h"

BracketsExtension::BracketsExtension() : CameoExtension("brackets") {
  InitializeHandlerMap();
}

const char* BracketsExtension::GetJavaScriptAPI() {
  return kGeneratedSource;
}

CameoExtension::Context* BracketsExtension::CreateContext(
    const CameoExtension::PostMessageCallback& post_message) {
  return new BracketsContext(this, post_message);
}

BracketsExtension::HandlerFunc BracketsExtension::GetHandler(
    const std::string& command) {
  HandlerMap::iterator it = handler_map_.find(command);
  if (it == handler_map_.end())
    return NULL;
  return it->second;
}

static int JSErrorFromPlatformError(platform::ErrorCode error_code) {
  // This works because brackets_platform.h is in sync with brackets.fs.* error
  // codes.
  return static_cast<int>(error_code);
}

static void HandleReadFile(const base::DictionaryValue* input,
                           base::DictionaryValue* output) {
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

static void HandleGetFileModificationTime(const base::DictionaryValue* input,
                                          base::DictionaryValue* output) {
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

static void HandleReadDir(const base::DictionaryValue* input,
                          base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);

  std::vector<std::string> result;
  platform::ErrorCode error = platform::ReadDir(path, result);

  output->SetInteger("error", JSErrorFromPlatformError(error));
  base::ListValue* l = new base::ListValue;
  l->AppendStrings(result);
  output->Set("files", l);
}

static void HandleWriteFile(const base::DictionaryValue* input,
                            base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);
  std::string data;
  input->GetString("data", &data);
  std::string encoding;
  input->GetString("encoding", &encoding);

  platform::ErrorCode error = platform::WriteFile(path, encoding, data);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

static void HandleOpenLiveBrowser(const base::DictionaryValue* input,
                                  base::DictionaryValue* output) {
  std::string url;
  input->GetString("url", &url);

  platform::ErrorCode error = platform::OpenLiveBrowser(url);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

static void HandleOpenURLInDefaultBrowser(const base::DictionaryValue* input,
                                          base::DictionaryValue* output) {
  std::string url;
  input->GetString("url", &url);

  platform::ErrorCode error = platform::OpenURLInDefaultBrowser(url);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

static void HandleRename(const base::DictionaryValue* input,
                         base::DictionaryValue* output) {
  std::string old_path;
  input->GetString("oldPath", &old_path);

  std::string new_path;
  input->GetString("newPath", &new_path);

  platform::ErrorCode error = platform::Rename(old_path, new_path);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

static void HandleMakeDir(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);
  int mode;
  input->GetInteger("mode", &mode);

  platform::ErrorCode error = platform::MakeDir(path, mode);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

static void HandleDeleteFileOrDirectory(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);

  platform::ErrorCode error = platform::DeleteFileOrDirectory(path);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

static void HandleMoveFileOrDirectoryToTrash(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);

  platform::ErrorCode error = platform::MoveFileOrDirectoryToTrash(path);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

static void HandleIsNetworkDrive(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);

  bool is_network_drive;
  platform::ErrorCode error = platform::IsNetworkDrive(path, is_network_drive);
  output->SetInteger("error", JSErrorFromPlatformError(error));
  output->SetBoolean("is_networkDrive", is_network_drive);
}

static void HandleShowOSFolder(const base::DictionaryValue* input, base::DictionaryValue* output) {
  std::string path;
  input->GetString("path", &path);

  platform::ErrorCode error = platform::ShowFolderInOSWindow(path);
  output->SetInteger("error", JSErrorFromPlatformError(error));
}

static void HandleGetPendingFilesToOpen(const base::DictionaryValue* input,
                          base::DictionaryValue* output) {
  std::vector<std::string> result;
  platform::ErrorCode error = platform::GetPendingFilesToOpen(result);

  output->SetInteger("error", JSErrorFromPlatformError(error));
  base::ListValue* l = new base::ListValue;
  l->AppendStrings(result);
  output->Set("files", l);
}

static void HandleGetRemoteDebuggingPort(const base::DictionaryValue* input, base::DictionaryValue* output) {
  int port;
  platform::ErrorCode error = platform::GetRemoteDebuggingPort(port);
  output->SetInteger("error", JSErrorFromPlatformError(error));
  output->SetInteger("debugging_port", port);
}

void BracketsExtension::InitializeHandlerMap() {
  handler_map_["ReadFile"] = HandleReadFile;
  handler_map_["GetFileModificationTime"] = HandleGetFileModificationTime;
  handler_map_["ReadDir"] = HandleReadDir;
  handler_map_["WriteFile"] = HandleWriteFile;
  handler_map_["OpenLiveBrowser"] = HandleOpenLiveBrowser;
  handler_map_["OpenURLInDefaultBrowser"] = HandleOpenURLInDefaultBrowser;
  handler_map_["Rename"] = HandleRename;
  handler_map_["MakeDir"] = HandleMakeDir;
  handler_map_["DeleteFileOrDirectory"] = HandleDeleteFileOrDirectory;
  handler_map_["MoveFileOrDirectoryToTrash"] = HandleMoveFileOrDirectoryToTrash;
  handler_map_["IsNetworkDrive"] = HandleIsNetworkDrive;
  handler_map_["ShowOSFolder"] = HandleShowOSFolder;
  handler_map_["GetPendingFilesToOpen"] = HandleGetPendingFilesToOpen;
  handler_map_["GetRemoteDebuggingPort"] = HandleGetRemoteDebuggingPort;
}

}  // namespace brackets
