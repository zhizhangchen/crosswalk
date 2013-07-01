// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cameo/brackets/brackets_context.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "cameo/brackets/brackets_platform.h"

namespace brackets {

BracketsContext::BracketsContext(
    BracketsExtension* brackets_extension,
    const CameoExtension::PostMessageCallback& post_message)
    : CameoExtension::Context(post_message),
      brackets_extension_(brackets_extension) {}

BracketsContext::~BracketsContext() {}

void BracketsContext::HandleMessage(const std::string& msg) {
  scoped_ptr<base::Value> v(base::JSONReader().ReadToValue(msg));
  base::DictionaryValue* input = static_cast<base::DictionaryValue*>(v.get());
  scoped_ptr<base::DictionaryValue> output(new base::DictionaryValue);

  std::string cmd;
  input->GetString("cmd", &cmd);
  std::string reply_id;
  input->GetString("_reply_id", &reply_id);
  output->SetString("_reply_id", reply_id);

  BracketsExtension::HandlerFunc handler = brackets_extension_->GetHandler(cmd);
  if (!handler) {
    LOG(WARNING) << "Unhandled command " << cmd;
    return;
  }

  handler(input, output.get());

  std::string result;
  base::JSONWriter::Write(output.get(), &result);
  PostMessage(result);
}

}  // namespace brackets
