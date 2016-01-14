var packager = require('electron-packager')
var osType = require('os').type();

var platform = null;
if (osType == "Darwin" || osType == "Linux") {
    platform = osType.toLowerCase();
} else if (osType == "Windows_NT") {
    platform = "win32"
}

var argv = require('yargs').argv;

// check which icon we should use, beta or regular
var iconName = argv.beta ? "console-beta" : "console"

// setup the common options for the packager
var options = {
    dir: __dirname,
    name: "server-console",
    version: "0.35.4",
    overwrite: true,
    prune: true,
    arch: "x64",
    platform: platform,
    icon: "resources/" + iconName
}

const EXEC_NAME = "server-console";
const SHORT_NAME = "Server Console";
const FULL_NAME = "High Fidelity Server Console";

// setup per OS options
if (osType == "Darwin") {
    options["name"] = SHORT_NAME
} else if (osType == "Windows_NT") {
    options["version-string"] = {
        CompanyName: "High Fidelity, Inc.",
        FileDescription: SHORT_NAME,
        ProductName: FULL_NAME,
        OriginalFilename: EXEC_NAME + ".exe"
    }
}

// check if we were passed a custom out directory, pass it along if so
if (argv.out) {
    options.out = argv.out
}

// call the packager to produce the executable
packager(options, function(error, appPath) {
    if (error) {
        console.error("There was an error writing the packaged console: " + error.message);
        process.exit(1);
    } else {
        console.log("Wrote new app to " + appPath);
    }
});
