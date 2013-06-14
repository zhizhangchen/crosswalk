// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAMEO_SRC_BRACKETS_BRACKETS_PLATFORM_H_
#define CAMEO_SRC_BRACKETS_BRACKETS_PLATFORM_H_

#include <string>
#include <vector>

namespace brackets {
namespace platform {

// Need to be in sync with the brackets.fs.* error constants.
enum ErrorCode {
  kNoError = 0,
  kUnknownError = 1,
  kInvalidParametersError = 2,
  // kNotFoundError,
  kCannotReadError = 4,
  kUnsupportedEncodingError = 5,
  // kCannotWriteError = 6,
  // kOutOfSpaceError = 7,
  // kNotFileError = 8,
  // kNotDirectoryError = 9,
  // kFileExistsError = 10
};

ErrorCode GetFileModificationTime(const std::string& path, time_t& mtime, bool& is_dir);
ErrorCode ReadDir(const std::string& path, std::vector<std::string>& directoryContents);
ErrorCode ReadFile(const std::string& filename, const std::string& encoding, std::string& contents);
ErrorCode WriteFile(const std::string& path, const std::string& encoding, std::string& data);
ErrorCode OpenLiveBrowser(const std::string& url);
ErrorCode OpenURLInDefaultBrowser(const std::string& url);
ErrorCode Rename(const std::string& old_name, const std::string& new_name);
ErrorCode ShowOpenDialog(const std::string& initial_path, std::vector<std::string>& files);

}  // namespace platform
}  // namespace brackets

#endif  // CAMEO_SRC_BRACKETS_BRACKETS_PLATFORM_H_
