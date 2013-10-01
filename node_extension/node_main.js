process.handleMessage = function (msg) {
  var ret;
  var msgObj = JSON.parse(msg, function(key, value){
    if (value.indexOf("__function__") === 0) {
      var _reply_id = value.split(":")[1];
      return function () {
        var resp = { arguments: arguments};
        if (_reply_id)
          resp._reply_id = _reply_id;
        else
          resp.eventType = key;
        return process.sendMessage(resp);
      }
    }
  });
  var apiStack = msgObj.apiStack;
  var module = require("./node_extensions/" + apiStack[0] +  ".js");
  apiStack.shift();
  apiStack.forEach(function (api) {
    module = module[api];
  });
  if (typeof module === "function")  {
    ret = module.apply(null, apiObj.arguments);
  }
  else
    ret = module;
  return JSON.stringify(ret);
}
process.getExtensionSource = function (path) {
  var fs = require('fs');
  function direct (func) {
    return {__direct__: func.toString()};
  }
  function directInit (func) {
    return {__directInit__: func.toString()};
  }
  var api = {};
  fs.readdirSync(path).forEach(function (file){
    var extPath = path + "/" + file;
    if (fs.statSync(extPath).isFile()) {
      api[file.replace(/\.js$/,"")] = require(extPath);
    }
  });
  return "var api_str = '"  + JSON.stringify(api, function (key, value) {
    if (typeof value === "function") {
      return "__function__";
    }
    else if  (value === undefined) {
      return "__undefined__";
    }
    else if  (Number.isNaN(value)) {
      return "__NaN__";
    }
    else if  (value && value.__direct__) {
      return {__obj_ref: value.__obj_ref};
    }
    else return value;
  }) + "'";
}
