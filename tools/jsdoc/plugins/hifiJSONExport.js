exports.handlers = {
    processingComplete: function(e) {
        const pathTools = require('path');
        var rootFolder = pathTools.join(__dirname, '..');
        var doclets = e.doclets.map(doclet => Object.assign({}, doclet));
        const fs = require('fs');
        doclets.map(doclet => {delete doclet.meta; delete doclet.comment});
        fs.writeFile(pathTools.join(rootFolder, "out/hifiJSDoc.json"), JSON.stringify(doclets, null, 4), function(err) {
            if (err) {
                return console.log(err);
            }

            console.log("The Hifi JSDoc JSON was saved!");
        });
    }
};
