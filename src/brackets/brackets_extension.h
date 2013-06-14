// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAMEO_SRC_BRACKETS_BRACKETS_EXTENSION_H_
#define CAMEO_SRC_BRACKETS_BRACKETS_EXTENSION_H_

#include "base/compiler_specific.h"
#include "cameo/src/extensions/browser/cameo_extension.h"

namespace brackets {

class BracketsExtension : public cameo::extensions::CameoExtension {
 public:
  BracketsExtension(cameo::extensions::CameoExtension::Poster* poster);

  // CameoExtension implementation.
  virtual const char* GetJavaScriptAPI() OVERRIDE;
  virtual void HandleMessage(const int32_t render_view_id,
                             const std::string& msg) OVERRIDE;
};

}  // namespace brackets

#endif  // CAMEO_SRC_BRACKETS_BRACKETS_EXTENSION_H_
