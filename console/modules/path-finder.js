var fs = require('fs');

exports.discoveredPath = function (name, preferRelease) {
    var path = "../build/" + name + "/";

    function binaryFromPath(name, path) {
        function platformExtension() {
            return process.platform == 'win32' ? ".exe" : ""
        }

        try {
            var fullPath = path + name + platformExtension();

            var stats = fs.lstatSync(path + name + platformExtension())

            if (stats.isFile()) {
                console.log("Found " + name + " at " + fullPath);
                return fullPath;
            }
        } catch (e) {
            console.log("Executable with name " + name + " not found at path " + path);
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
