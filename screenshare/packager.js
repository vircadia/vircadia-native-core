var packager = require('electron-packager');
var osType = require('os').type();
var argv = require('yargs').argv;

var platform = null;
if (osType == "Darwin" || osType == "Linux") {
    platform = osType.toLowerCase();
} else if (osType == "Windows_NT") {
    platform = "win32"
}

var NAME = "hifi-screenshare";
var options = {
    dir: __dirname,
    name: NAME,
    version: "0.1.0",
    overwrite: true,
    prune: true,
    arch: "x64",
    platform: platform,
    ignore: "electron-packager|README.md|CMakeLists.txt|packager.js|.gitignore"
};

// setup per OS options
if (osType == "Darwin") {
    options["app-bundle-id"] = "com.highfidelity.hifi-screenshare";
} else if (osType == "Windows_NT") {
    options["version-string"] = {
        CompanyName: "Vircadia",
        FileDescription: "Vircadia Screenshare",
        ProductName: NAME,
        OriginalFilename: NAME + ".exe"
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
