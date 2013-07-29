// Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
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

exports.postMessage = (function() {
  var callbacks = {};
  var next_reply_id = 0;
  extension.setMessageListener(function(json) {
    var msg = JSON.parse(json);
    var reply_id = msg._reply_id;
    var callback = callbacks[reply_id];
    if (callback) {
      delete msg._reply_id;
      delete callbacks[reply_id];
      callback(msg);
    } else {
      console.log('Invalid reply_id received from node extension: ' + reply_id);
    }
  });
  return function(msg, callback) {
    var reply_id = next_reply_id;
    next_reply_id += 1;
    callbacks[reply_id] = callback;
    msg._reply_id = reply_id.toString();
    extension.postMessage(JSON.stringify(msg));
  }
})();
