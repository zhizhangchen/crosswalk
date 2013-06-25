//
// Copyright (c) 2012 Chhatoi Pritam Baral <pritam@pritambaral.com>. All rights reserved.
// Copyright (c) 2013 Intel Corporation. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "cameo/src/brackets/brackets_platform.h"

#include <gtk/gtk.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace brackets {
namespace platform {

ErrorCode ConvertLinuxErrorCode(int errorCode, bool isReading=true)
{
    switch (errorCode) {
      case ENOENT:
        return kNotFoundError;
      case EACCES:
        return isReading ? kCannotReadError : kCannotWriteError;
      case ENOTDIR:
        return kNotDirectoryError;
      default:
        return kUnknownError;
    }
}

ErrorCode GetFileModificationTime(const std::string& path,
                                  time_t& mtime,
                                  bool& is_dir) {
  struct stat buf;
  if (stat(path.c_str(), &buf) == -1)
    // FIXME(jeez): handle kInvalidParametersError return case.
    return ConvertLinuxErrorCode(errno);

  mtime = buf.st_mtime;
  is_dir = S_ISDIR(buf.st_mode);
  return kNoError;
}

ErrorCode ReadDir(const std::string& path,
                  std::vector<std::string>& directoryContents) {
  DIR* dir;
  if ((dir = opendir(path.c_str())) == NULL)
    return kUnknownError;

  std::vector<std::string> files;
  while (struct dirent* dir_entry = readdir(dir)) {
    if (!strcmp(dir_entry->d_name, ".") || !strcmp(dir_entry->d_name,".."))
      continue;
    if (dir_entry->d_type == DT_DIR)
      directoryContents.push_back(dir_entry->d_name);
    else if (dir_entry->d_type == DT_REG)
      files.push_back(dir_entry->d_name);
  }
  closedir(dir);

  directoryContents.insert(directoryContents.end(), files.begin(), files.end());
  return kNoError;
}

ErrorCode ReadFile(const std::string& filename,
                   const std::string& encoding,
                   std::string& contents) {
  if (encoding != "utf8")
    return kUnsupportedEncodingError;

  struct stat buf;
  if (stat(filename.c_str(),&buf) == -1)
    return kUnknownError;

  if (!S_ISREG(buf.st_mode))
    return kUnknownError;


  std::ifstream file(filename.c_str(), std::ios::in);
  if (!file)
    return kCannotReadError;

  file.seekg(0, std::ios::end);
  contents.resize(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(&contents[0], contents.size());
  file.close();

  return kNoError;
}

ErrorCode WriteFile(const std::string& path,
                    const std::string& encoding,
                    std::string& data) {
  if (encoding != "utf8")
    return kUnknownError;

  FILE* file = fopen(path.c_str(), "w");
  if (!file)
    return kUnknownError;

  fwrite(data.c_str(), 1, data.size(), file);
  if (fclose(file) == EOF)
    return kUnknownError;

  return kNoError;
}

ErrorCode OpenLiveBrowser(const std::string& url) {
  pid_t pid = vfork(); // So we can share 'error', replace with fork() call.
  short int error = 0;
  if (pid == 0) {
    execlp("google-chrome", "--allow-file-access-from-files", url.c_str(), "--remote-debugging-port=9222", NULL);
    error = errno;
    _exit(0);
  }

  if (error) {
    std::cout << "Error when trying to launch live browser: " << strerror(error) << "\n";
    return kUnknownError;
  }

  return kNoError;
}

ErrorCode OpenURLInDefaultBrowser(const std::string& url) {
  if (!gtk_show_uri(NULL, url.c_str(), GDK_CURRENT_TIME, NULL))
    return kUnknownError;
  return kNoError;
}

ErrorCode Rename(const std::string& old_name, const std::string& new_name) {
  if (rename(old_name.c_str(), new_name.c_str()) == -1)
    return kUnknownError;
  return kNoError;
}

ErrorCode MakeDir(const std::string& path, int mode) {
  ErrorCode mkdirError = kNoError;
  mode = mode | 0755;

  // FIXME(jeez): add support to recursive mkdir.
  if (mkdir(path.c_str(),mode) == -1)
    mkdirError = ConvertLinuxErrorCode(errno);

  return mkdirError;
}

}  // namespace platform
}  // namespace brackets
