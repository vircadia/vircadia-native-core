var fs = require('fs');
var path = require('path');

function endsWith(path, exts) {
    var result = false;
    exts.forEach(function(ext) {
        if (path.endsWith(ext)) {
            result = true;
        }
    });
    return result;
}

function WarningObject(file, type, issues){
    this.file = file;
    this.type = type;    
    this.issues = issues;
}

var warnings = [];

function parse() {
    var rootFolder = __dirname;
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

    dirList.forEach(function (dir) {
        var joinedDir = path.join(rootFolder, dir);
        var files = fs.readdirSync(joinedDir);
        files.forEach(function (file) {
            var pathDir = path.join(joinedDir, file);
            if (fs.lstatSync(pathDir).isFile() && endsWith(pathDir, exts)) {
                // load entire file into a string
                var data = fs.readFileSync(pathDir, "utf8");
                var fileName = path.basename(file);
                var badJSDocWarnings = checkForBadJSDoc(data, fileName);
                if (badJSDocWarnings.length > 0){
                    warnings.push(badJSDocWarnings);
                }
                var badWordsList = checkForBadwordlist(data, fileName);
                if (badWordsList){
                    warnings.push(badWordsList);
                }
                
            }
        });
    });
}

function checkForBadJSDoc(dataToSearch, file){
    var warningList = [];
    var reg = /\/\*\*js.*/g;
    var matches = dataToSearch.match(reg);
    if (matches) {
        // add to source, but strip off c-comment asterisks
        var filtered = matches.filter( item => {
            return item.trim() !== '/**jsdoc';
        });
        if (filtered.length > 0){
            warningList.push(new WarningObject(file, "badJSDOC", filtered));
        }
    }
    return warningList;
}

var badWordList = ["@params", "@return", "@bool"];

function checkForBadwordlist(dataToSearch, file){
    var warningList = [];
    var reg = /(\/\*\*jsdoc(.|[\r\n])*?\*\/)/g;    
    var matches = dataToSearch.match(reg);
    if (matches) {
        var filtered = matches.forEach( item => {
            var split = item.split(" ");
            var filterList = [];
            item.split(" ").forEach( item => {
                badWordList.forEach(searchTerm => {
                    if (item === searchTerm) {
                        filterList.push(searchTerm);
                    }
                })
            })
            if (filterList.length > 0) {
                warningList.push(filterList);
            }
        });
    }
    let flatten = warningList.reduce( (prev, cur) => {
        return [...prev, ...cur];
    },[])
    let unique = [...new Set(flatten)];
    if (warningList.length > 0) {
        return new WarningObject(file, "badWordList", unique);
    }
    
}

parse();
fs.writeFileSync(path.join(__dirname, "warningLog"), warnings.map( item => JSON.stringify(item)).join("\n"));