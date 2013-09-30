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

/*exports.postMessage = (function() {
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
})();*/
var postMessage = (function() {
  var callbacks = {};
  var next_reply_id = 0;
  var _event_listeners = [];
  extension.setMessageListener(function(json) {
    var msg = JSON.parse(json);
    var reply_id = msg._reply_id;
    var callback = callbacks[reply_id];
    if (reply_id === undefined) {
      //It's an event. Call listeners
      _event_listeners.forEach(function (listener) {
        listener[msg.eventType].apply(null, msg.arguments);
      });
    }
    else if (callback) {
      delete msg._reply_id;
      delete callbacks[reply_id];
      if (msg.exception) {
        throw exception;
      }
      callback.apply(null, msg.arguments);
    } else {
      console.log('Invalid reply_id received from node extension: ' + reply_id);
    }
  });
  return function(apiStack, args) {
    var send_msg = extension.internal.sendSyncMessage;
    var request, resp, respObj;
    Array.prototype.forEach.call(args, function (arg) {
      if (arg) {
        for (var prop in arg) {
          if (typeof arg[prop] === "function") {
            _event_listeners.push(arg);
            break;
          }
        }
        if (typeof arg === "function") {
          var reply_id = next_reply_id;
          next_reply_id += 1;
          arg._reply_id = reply_id.toString();
          callbacks[reply_id] = arg;
          //send_msg = extension.postMessage;
        }
      }
    });

    request = JSON.stringify({apiStack: apiStack, arguments: Array.prototype.slice.call(args, 0)}, function (key, value) {
      if (typeof value === "function") {
        return "__function__:" + (value._reply_id ? value._reply_id : "");
      }
      else if  (value === undefined) {
        return "__undefined__";
      }
      else if  (Number.isNaN(value)) {
        return "__NaN__";
      }
      else if  (value && value.__obj_ref) {
        return {__obj_ref: value.__obj_ref};
      }
      else return value;
    });
    resp = send_msg(request);
    respObj = ((resp === undefined || resp === "" || resp == "undefined") ? undefined : JSON.parse(resp, function(key, value) {
      if (value === "__function__")
        return function () {
          return postMessage(this, key, arguments);
        }
      return value;
    }));
    if (respObj && respObj.exception) {
      throw respObj.exception;
    }
    respObj && respObj.arguments && respObj.arguments.forEach(function (arg, i) {
      jQuery.extend(true, args[i], arg);
    });
    return respObj && respObj.result;
  }
})();
function proxyFunction(parents, obj) {
  for (var prop in obj) {
    var value = obj[prop];
    if (value === "__function__") {
      var apiStack = parents.slice(0).push(prop);
      obj[prop] = function () {
        postMessage(apiStack, arguments);
      }
    }
    else if (typeof value === "object") {
      parents.push(prop);
      proxyFunction(parents, value);
    }
  }
}
var apis = JSON.parse(api_str, function (key, value) {
  if (value === "__undefined__"){
    return undefined;
  }
  else if (value === "__undefined__"){
    return undefined;
  }
  return value;
});
proxyFunction([], apis);
for (var api in apis) {
  window[api] = apis[api];
}
