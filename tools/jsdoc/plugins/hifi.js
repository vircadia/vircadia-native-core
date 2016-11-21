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
        console.log("Scanning hifi source for jsdoc comments...");

        // directories to scan for jsdoc comments
        var dirList = [
            '../../interface/src',
            '../../interface/src/scripting',
            '../../interface/src/ui/overlays',
            '../../libraries/script-engine/src',
            '../../libraries/networking/src',
            '../../libraries/animation/src',
            '../../libraries/entities/src',
        ];
        var exts = ['.h', '.cpp'];

        const fs = require('fs');
        dirList.forEach(function (dir) {
            var files = fs.readdirSync(dir)
            files.forEach(function (file) {
                var path = dir + "/" + file;
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
