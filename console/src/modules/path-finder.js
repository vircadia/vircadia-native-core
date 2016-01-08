var fs = require('fs');
var path = require('path');

exports.searchPaths = function(name, binaryType) {
    function platformExtension(name) {
        if (name == "Interface") {
            if (process.platform == "darwin") {
                return ".app/Contents/MacOS/" + name
            } else if (process.platform == "win32") {
                return ".exe"
            } else {
                return ""
            }
        } else {
            return process.platform == "win32" ? ".exe" : ""
        }
    }

    var extension = platformExtension(name);
    var devBasePath = "../build/" + name + "/";

    var paths = [];

    if (binaryType == "local-release" ||  binaryType == "local-debug") {
        // check in the developer build tree for binaries
        paths = [
            devBasePath + name + extension,
            devBasePath + (binaryType == "local-release" ? "Release/" : "Debug/") + name + extension
        ]
    } else {
        // check directly beside the binary
        paths = [
            path.join(path.dirname(process.execPath), name + extension)
        ];

        // check if we're inside an app bundle on OS X
        if (process.platform == "darwin") {
            var contentPath = ".app/Contents/";
            var contentEndIndex = __dirname.indexOf(contentPath);

            if (contentEndIndex != -1) {
                // this is an app bundle, check in Contents/MacOS for the binaries
                var appPath = __dirname.substring(0, contentEndIndex);
                appPath += ".app/Contents/MacOS/Components.app/Contents/MacOS/";

                paths.push(appPath + name + extension);
            }
        }
    }

    return paths;
}

exports.discoveredPath = function (name, binaryType) {
    function binaryFromPaths(name, paths) {
        for (var i = 0; i < paths.length; i++) {
            var testPath = paths[i];

            try {
                var stats = fs.lstatSync(testPath);

                if (stats.isFile() || (stats.isDirectory() && extension == ".app")) {
                    console.log("Found " + name + " at " + testPath);
                    return testPath;
                }
            } catch (e) {
                console.log("Executable with name " + name + " not found at path " + testPath);
            }
        }

        return null;
    }

    // attempt to find a binary at the usual paths, return null if it doesn't exist
    return binaryFromPaths(name, this.searchPaths(name, binaryType));
}
