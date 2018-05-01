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
    beforeParse: function(e) {
        const pathTools = require('path');
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
            '../../libraries/ui/src/ui',
            '../../plugins/oculus/src',
            '../../plugins/openvr/src',
        ];
        var exts = ['.h', '.cpp'];

        const fs = require('fs');
        dirList.forEach(function (dir) {
            var joinedDir = pathTools.join(rootFolder, dir);
            var files = fs.readdirSync(joinedDir)
            files.forEach(function (file) {
                var path = pathTools.join(joinedDir, file);
                if (fs.lstatSync(path).isFile() && endsWith(path, exts)) {
                    var data = fs.readFileSync(path, "utf8");
                    var reg = /(\/\*\*jsdoc(.|[\r\n])*?\*\/)/gm;
                    var matches = data.match(reg);
                    if (matches) {
                        e.source += matches.map(function (s) { return s.replace('/**jsdoc', '/**'); }).join('\n');
                    }
                }
            });
        });
    }
};
