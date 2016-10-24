var fs = require('fs');
var path = require('path');

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


exports.searchPaths = function(name, binaryType, releaseType) {
    var extension = platformExtension(name);
    var devBasePath = "../build/" + name + "/";

    var paths = [];

    if (!releaseType) {
        // check in the developer build tree for binaries
        var typeSpecificPath = (binaryType == "local-release" ? "Release/" : "Debug/")
        paths = [
            devBasePath + name + extension,
            devBasePath + typeSpecificPath + name + extension
        ]
    } else {
        // check directly beside the binary
        paths = [
            path.join(path.dirname(process.execPath), name + extension)
        ];

        if (process.platform == "win32") {
            // check a level back in case we're packaged on windows
            paths.push(path.resolve(path.dirname(process.execPath), '../' +  name + extension))
        }

        // assume we're inside an app bundle on OS X
        if (process.platform == "darwin") {
            var contentPath = ".app/Contents/";
            var contentEndIndex = __dirname.indexOf(contentPath);

            if (contentEndIndex != -1) {
                // this is an app bundle
                var appPath = __dirname.substring(0, contentEndIndex) + ".app";

                // check in Contents/MacOS for the binaries
                var componentsPath = appPath + "/Contents/MacOS/Components.app/Contents/MacOS/";
                paths.push(componentsPath + name + extension);

                // check beside the app bundle for the binaries
                paths.push(path.join(path.dirname(appPath), name + extension));
            }
        }
    }

    return paths;
}

exports.discoveredPath = function (name, binaryType, releaseType) {
    function binaryFromPaths(name, paths) {
        for (var i = 0; i < paths.length; i++) {
            var testPath = paths[i];

            try {
                var stats = fs.lstatSync(testPath);
                var extension = platformExtension(name);

                if (stats.isFile() || (stats.isDirectory() && extension == ".app")) {
                    log.debug("Found " + name + " at " + testPath);
                    return testPath;
                }
            } catch (e) {
                log.debug("Executable with name " + name + " not found at path " + testPath);
            }
        }

        return null;
    }

    // attempt to find a binary at the usual paths, return null if it doesn't exist
    return binaryFromPaths(name, this.searchPaths(name, binaryType, releaseType));
}
