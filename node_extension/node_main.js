process.on('message', function (module, msg) {
  var module = require("./node_extensions/" + module + "/impl.js");

  module.handleMessage(msg);
})
process.getExtensionSource = function (path) {
  var fs = require('fs');
  var src = "";
  fs.readdirSync(path).forEach(function (dir){
    var extDir = path + "/" + dir;
    if (fs.statSync(extDir).isDirectory()) {
      src += fs.readFileSync(extDir + "/interface.js");
      src += "\n";
    }
  });
  return src;
}
