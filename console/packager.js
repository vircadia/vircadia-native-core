var packager = require('electron-packager')
var osType = require('os').type();

var platform = null;
if (osType == "Darwin" || osType == "Linux") {
    platform = osType.toLowerCase();
} else if (osType == "Windows_NT") {
    platform = "win32"
}

// setup the common options for the packager
var options = {
    dir: __dirname,
    version: "0.35.4",
    overwrite: true,
    prune: true,
    arch: "x64",
    platform: platform
}

// setup per OS options
if (osType == "Darwin") {
    options["name"] = "Server Console"
    options["icon"] = "resources/console.icns"
} else if (osType == "Windows_NT") {
    options["name"] = "server-console"
    options["icon"] = "resources/console.ico"
} else if (osType == "Linux") {
    options["name"] = "server-console"
}

console.log(options);

// call the packager now
packager(options, function(error, appPath){
    console.log("Wrote new app to " + appPath);
});
