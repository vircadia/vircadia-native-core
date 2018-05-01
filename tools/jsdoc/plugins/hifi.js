function endsWith(path, exts) {
    var result = false;
    exts.forEach(function(ext) {
        if (path.endsWith(ext)) {
            result = true;
        }
    });
    return result;
}

exports.handlers = {

    // This event is triggered before parsing has even started.
    // We use this event to scan the C++ files for jsdoc comments
    // and reformat them into a form digestable by jsdoc.
    beforeParse: function(e) {
        var pathTools = require('path');
        var rootFolder = pathTools.dirname(e.filename);
        console.log("Scanning hifi source for jsdoc comments...");

        // directories to scan for jsdoc comments
        var dirList = [
            '../../interface/src',
            '../../interface/src/assets',
            '../../interface/src/audio',
            '../../interface/src/avatar',
            '../../interface/src/commerce',
            '../../interface/src/devices',
            '../../interface/src/java',
            '../../interface/src/networking',
            '../../interface/src/ui/',
            '../../interface/src/scripting',
            '../../interface/src/ui/overlays',
            '../../interface/src/raypick',
            '../../libraries/animation/src',
            '../../libraries/audio-client/src',
            '../../libraries/audio/src',
            '../../libraries/avatars/src',
            '../../libraries/avatars-renderer/src/avatars-renderer',
            '../../libraries/controllers/src/controllers/',
            '../../libraries/controllers/src/controllers/impl/',
            '../../libraries/display-plugins/src/display-plugins/',
            '../../libraries/entities/src',
            '../../libraries/graphics-scripting/src/graphics-scripting/',
            '../../libraries/input-plugins/src/input-plugins',
            '../../libraries/model-networking/src/model-networking/',
            '../../libraries/networking/src',
            '../../libraries/octree/src',
            '../../libraries/physics/src',
            '../../libraries/pointers/src',
            '../../libraries/script-engine/src',
            '../../libraries/shared/src',
            '../../libraries/shared/src/shared',
            '../../libraries/trackers/src/trackers',
            '../../libraries/ui/src/ui',
            '../../plugins/oculus/src',
            '../../plugins/openvr/src'
        ];

        // only files with this extension will be searched for jsdoc comments.
        var exts = ['.h', '.cpp'];

        var fs = require('fs');
        dirList.forEach(function (dir) {
            var joinedDir = pathTools.join(rootFolder, dir);
            var files = fs.readdirSync(joinedDir);
            files.forEach(function (file) {
                var path = pathTools.join(joinedDir, file);
                if (fs.lstatSync(path).isFile() && endsWith(path, exts)) {
                    // load entire file into a string
                    var data = fs.readFileSync(path, "utf8");

                    // this regex searches for blocks starting with /**jsdoc and end with */
                    var reg = /(\/\*\*jsdoc(.|[\r\n])*?\*\/)/gm;
                    var matches = data.match(reg);
                    if (matches) {
                        // add to source, but strip off c-comment asterisks
                        e.source += matches.map(function (s) {
                            return s.replace('/**jsdoc', '/**');
                        }).join('\n');
                    }
                }
            });
        });
    },

    // This event is triggered when a new doclet has been created
    // but before it is passed to the template for output
    newDoclet: function (e) {

        // we only care about hifi custom tags on namespace and class doclets
        if (e.doclet.kind === "namespace" || e.doclet.kind === "class") {
            var rows = [];
            if (e.doclet.hifiInterface) {
                rows.push("Interface Scripts");
            }
            if (e.doclet.hifiClientEntity) {
                rows.push("Client Entity Scripts");
            }
            if (e.doclet.hifiServerEntity) {
                rows.push("Server Entity Scripts");
            }
            if (e.doclet.hifiAssignmentClient) {
                rows.push("Assignment Client Scripts");
            }

            // Append an Available In: table at the end of the namespace description.
            if (rows.length > 0) {
                var table = "<br><br><table><tr><th>Available in:</th><td>" + rows.join("</td><td>") + "</td></tr></table>";
                e.doclet.description = (e.doclet.description ? e.doclet.description : "") + table;
            }
        }
    }
};

// Define custom hifi tags here
exports.defineTags = function (dictionary) {

    // @hifi-interface
    dictionary.defineTag("hifi-interface", {
        onTagged: function (doclet, tag) {
            doclet.hifiInterface = true;
        }
    });

    // @hifi-assignment-client
    dictionary.defineTag("hifi-assignment-client", {
        onTagged: function (doclet, tag) {
            doclet.hifiAssignmentClient = true;
        }
    });

    // @hifi-client-entity
    dictionary.defineTag("hifi-client-entity", {
        onTagged: function (doclet, tag) {
            doclet.hifiClientEntity = true;
        }
    });

    // @hifi-server-entity
    dictionary.defineTag("hifi-server-entity", {
        onTagged: function (doclet, tag) {
            doclet.hifiServerEntity = true;
        }
    });
};
