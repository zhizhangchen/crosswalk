// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAMEO_SRC_BRACKETS_BRACKETS_EXTENSION_H_
#define CAMEO_SRC_BRACKETS_BRACKETS_EXTENSION_H_

#include <map>
#include "base/compiler_specific.h"
#include "cameo/extensions/browser/cameo_extension.h"

using cameo::extensions::CameoExtension;

namespace base {
class DictionaryValue;
}

namespace brackets {

class BracketsExtension : public CameoExtension {
 public:
  BracketsExtension();

  // CameoExtension implementation.
  virtual const char* GetJavaScriptAPI() OVERRIDE;
  virtual Context* CreateContext(
      const PostMessageCallback& post_message) OVERRIDE;

  typedef void (* HandlerFunc)(const base::DictionaryValue*,
                               base::DictionaryValue*);
  HandlerFunc GetHandler(const std::string& command);

 private:
  void InitializeHandlerMap();

  typedef std::map<std::string, HandlerFunc> HandlerMap;
  HandlerMap handler_map_;
};

}  // namespace brackets

#endif  // CAMEO_SRC_BRACKETS_BRACKETS_EXTENSION_H_
