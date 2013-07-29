var fs = require('fs');
var util = require('util');
var brackets = brackets || {};
var path = require('path');
brackets.fs = {
  NO_ERROR: 0,
  ERR_UNKNOWN: 1,
  ERR_INVALID_PARAMS: 2,
  ERR_NOT_FOUND: 3,
  ERR_CANT_READ: 4,
  ERR_UNSUPPORTED_ENCODING: 5,
  ERR_CANT_WRITE: 6,
  ERR_OUT_OF_SPACE: 7,
  ERR_NOT_FILE: 8,
  ERR_NOT_DIRECTORY: 9,
  ERR_FILE_EXISTS: 10,
};
exports.handleMessage = function (msg) {
  var api = JSON.parse(msg);
  function sendResponse(res) {
    res._reply_id = api._reply_id;
    if (res.error !== undefined) {
      if (!res.error) {
        res.error = brackets.fs.NO_ERROR;
      }
      else if (res.error.code === "ENOENT") {
        res.error = brackets.fs.ERR_NOT_FOUND;
      }
    }
    process.sendMessage(JSON.stringify(res));
  }
  function rename (oldPath, newPath) {
    var err = brackets.fs.NO_ERROR;
    try {
        fs.renameSync(oldPath, newPath)
    } catch (e) {
      err = brackets.fs.ERR_NOT_FOUND;
    }
    sendResponse({
        error: err
    });
  }
  function spawn(app, args, errorCallback) {
    var error = null, app;
    if (typeof args === "string")
      args = [args];
    app = require('child_process').spawn(app, args);
    app.on('error', function (e) {
      console.log("error", e);
      error = e;
      if (errorCallback)
        errorCallback(e);
    });
    setTimeout(function () {
      if (!(error && errorCallback)) {
        sendResponse({error: error});
      }
    }, 100);
    return app;
  }
  function open(uri) {
    var opener = "xdg-open";
    switch (require('os').platform) {
      case "wind32":
        opener = "start";
        break;
      case "darwin":
        opener = "open";
        break;
    }
    spawn(opener, uri);
  }
  try {
  switch (api.cmd){
    case "ReadFile":
      fs.readFile(api.path, {encoding: api.encoding}, function (err, data) {
        sendResponse({
          error: err,
          data: data
        });
      })
      break;
    case "Rename":
      rename(api.oldPath, api.newPath);
      break;
    case 'GetRemoteDebuggingPort':
      sendResponse({
        debugging_port: 31000
      });
      break;
    case 'WriteFile':
      sendResponse({
        error: 0,
        data: fs.writeFileSync(api.path,  api.data, api.encoding)
      });
      break;
    case 'GetFileModificationTime':
      fs.stat(api.path, function (err, stats) {
        sendResponse({
          error: err,
          is_dir: err? undefined:stats.isDirectory(),
          modtime: err? undefined:stats.mtime.getTime()
        });
      });
      break;
    case 'ReadDir':
      fs.readdir(api.path, function (err, files) {
        sendResponse({
          error: err,
          files: files
        });
      });
      break;
    case 'MakeDir':
      if (fs.existsSync(trashDir)) {
        sendResponse({
          error: brackets.fs.ERR_FILE_EXISTS,
        });
        return;
      }
      fs.mkdir(api.path, api.mode, function (err) {
        sendResponse({
          error: err,
        });
      });
      break;
    case 'DeleteFileOrDirectory':
      fs.stat(api.path, function (err, stats) {
        var func = fs.unlink;
        if (stats.isDirectory())
          func = fs.rmdir;
        func(api.path, function (err) {
          sendResponse({error: err});
        })
      })
      break;
    case 'IsNetworkDrive':
      sendResponse({error: 0, is_network_drive: false });
      break;
    case 'MoveFileOrDirectoryToTrash':
      var trashDir = "/tmp/brackets";
      if (!fs.existsSync(trashDir))
        fs.mkdirSync(trashDir);
      rename(api.path, trashDir + path.basename(api.path));
      break;
    case 'GetPendingFilesToOpen':
      sendResponse( {error: 0, files: [] });
      break;
    case 'OpenLiveBrowser':
      var args = ["--allow-file-access-from-files", api.url,
                  "--remote-debugging-port=9222"];
      spawn("google-chrome", args, function () {
        spawn("chromium-browser", args);
      });
      break;
    case 'OpenURLInDefaultBrowser':
      open(api.url);
      break;
    case 'ShowOSFolder':
      fs.stat(api.path, function (err, stats) {
        var dir = api.path;
        if (stats.isFile())
          dir = path.dirname(api.path);
        open(dir);
      });
      break;
    default:
      console.error("Warning: Command not supported: " + api.cmd);
      break;
  }
  }catch (e) {
    sendResponse({error: brackets.fs.ERR_UNKNOWN});
    console.log(e.stack);
  }
}
