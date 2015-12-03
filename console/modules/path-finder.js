var fs = require('fs');

exports.discoveredPath = function (name, preferRelease) {
    var path = "../build/" + name + "/";

    function binaryFromPath(name, path) {
        function platformExtension(name) {
            if (name == "Interface") {
                if (process.platform == "darwin") {
                    return ".app"
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
        var fullPath = path + name + extension;

        try {
            var stats = fs.lstatSync(fullPath);

            if (stats.isFile() || (stats.isDirectory() && extension == ".app")) {
                console.log("Found " + name + " at " + fullPath);
                return fullPath;
            }
        } catch (e) {
            console.warn("Executable with name " + name + " not found at path " + fullPath);
        }

        return null;
    }

    // does the executable exist at this path already?
    // if so assume we're on a platform that doesn't have Debug/Release
    // folders and just use the discovered executable
    var matchingBinary = binaryFromPath(name, path);

    if (matchingBinary == null) {
        if (preferRelease) {
            // check if we can find the executable in a Release folder below this path
            matchingBinary = binaryFromPath(name, path + "Release/");
        } else {
            // check if we can find the executable in a Debug folder below this path
            matchingBinary = binaryFromPath(name, path + "Debug/");
        }
    }

    return matchingBinary;
}
