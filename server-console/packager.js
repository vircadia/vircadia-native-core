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
var iconName = argv.production ? "console" : "console-beta";

// setup the common options for the packager
var options = {
    dir: __dirname,
    name: "server-console",
    version: "0.37.5",
    overwrite: true,
    prune: true,
    arch: "x64",
    platform: platform,
    icon: "resources/" + iconName,
    ignore: "logs|(S|s)erver(\\s|-)(C|c)onsole-\\S+|(S|s)andbox-\\S+|electron-packager|README.md|CMakeLists.txt|packager.js|.gitignore"
}

const EXEC_NAME = "server-console";
var SHORT_NAME = argv.client_only ? "Console" : "Sandbox";
var FULL_NAME = argv.client_only ? "Vircadia Console" : "Vircadia Sandbox";

// setup per OS options
if (osType == "Darwin") {
    options["app-bundle-id"] = "com.vircadia.server-console" + (argv.production ? "" : "-dev")
    options["name"] = SHORT_NAME
} else if (osType == "Windows_NT") {
    options["version-string"] = {
        CompanyName: "Vircadia",
        FileDescription: FULL_NAME,
        ProductName: FULL_NAME,
        OriginalFilename: EXEC_NAME + ".exe"
    }
}

// check if we were passed a custom out directory, pass it along if so
if (argv.out) {
    options.out = argv.out
}

// call the packager to produce the executable
packager(options)
    .then(appPath => {
        console.log("Wrote new app to " + appPath);
    })
    .catch(error => {
        console.error("There was an error writing the packaged console: " + error.message);
        process.exit(1);
    });
