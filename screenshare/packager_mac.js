var packager = require('electron-packager');
var options = {
    dir: __dirname,
    name: "hifi-screenshare",
    version: "0.1.0",
    overwrite: true,
    prune: true,
    arch: "x64",
    platform: "darwin",
    CompanyName: "High Fidelity, Inc.",
    FileDescription: "High Fidelity Screenshare",
    OriginalFilename: "hifi-screenshare.app",
    ignore: "electron-packager|README.md|CMakeLists.txt|packager.js|.gitignore"
};

options["app-bundle-id"] = "com.highfidelity.screenshare"; 

// call the packager to produce the executable
packager(options, function(error, appPath) {
    if (error) {
        console.error("There was an error writing the packaged console: " + error.message);
        process.exit(1);
    } else {
        console.log("Wrote new app to " + appPath);
    }
});
