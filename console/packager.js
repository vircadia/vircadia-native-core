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

const EXEC_NAME = "server-console";
const SHORT_NAME = "Server Console";
const FULL_NAME = "High Fidelity Server Console";

// setup per OS options
if (osType == "Darwin") {
    options["name"] = SHORT_NAME
    options["icon"] = "resources/console.icns"
} else if (osType == "Windows_NT") {
    options["name"] = "server-console"
    options["icon"] = "resources/console.ico"
    options["version-string"] = {
        CompanyName: "High Fidelity, Inc.",
        FileDescription: SHORT_NAME,
        ProductName: FULL_NAME,
        OriginalFilename: EXEC_NAME + ".exe"
    }
} else if (osType == "Linux") {
    options["name"] = "server-console"
}

// call the packager to produce the executable
packager(options, function(error, appPath){
    console.log("Wrote new app to " + appPath);
});
