var fs = require('fs');

exports.searchPaths = function(name, preferRelease) {
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
    var basePath = "../build/" + name + "/";

    return [
        basePath + name + extension,
        basePath + (preferRelease ? "Release/" : "Debug/") + name + extension
    ];
}

exports.discoveredPath = function (name, preferRelease) {
    function binaryFromPaths(name, paths) {
        for (var i = 0; i < paths.length; i++) {
            var path = paths[i];

            try {
                var stats = fs.lstatSync(path);

                if (stats.isFile() || (stats.isDirectory() && extension == ".app")) {
                    console.log("Found " + name + " at " + path);
                    return path;
                }
            } catch (e) {
                console.warn("Executable with name " + name + " not found at path " + path);
            }
        }

        return null;
    }

    // attempt to find a binary at the usual paths, return null if it doesn't exist
    return binaryFromPaths(name, this.searchPaths(name, preferRelease));
}
