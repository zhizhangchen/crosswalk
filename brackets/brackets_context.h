// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAMEO_SRC_BRACKETS_BRACKETS_CONTEXT_H_
#define CAMEO_SRC_BRACKETS_BRACKETS_CONTEXT_H_

#include <string>
#include "base/compiler_specific.h"
#include "cameo/brackets/brackets_extension.h"
#include "cameo/brackets/brackets_platform.h"
#include "cameo/extensions/browser/cameo_extension.h"

using cameo::extensions::CameoExtension;

namespace brackets {

class BracketsExtension;

class BracketsContext : public CameoExtension::Context {
 public:
  BracketsContext(BracketsExtension* brackets_extension,
                  const CameoExtension::PostMessageCallback& post_message);
  virtual ~BracketsContext();

  virtual void HandleMessage(const std::string& msg) OVERRIDE;

 private:
  BracketsExtension* brackets_extension_;
};

}  // namespace brackets

#endif  // CAMEO_SRC_BRACKETS_BRACKETS_CONTEXT_H_
