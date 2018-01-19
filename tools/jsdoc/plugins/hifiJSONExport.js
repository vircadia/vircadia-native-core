exports.handlers = {
    processingComplete: function(e) {
        var doclets = e.doclets.map(doclet => Object.assign({}, doclet));
        const fs = require('fs');
        doclets.map(doclet => {delete doclet.meta; delete doclet.comment});
        fs.writeFile("out/hifiJSDoc.json", JSON.stringify(doclets, null, 4), function(err) {
            if (err) {
                return console.log(err);
            }

            console.log("The Hifi JSDoc JSON was saved!");
        });
    }
};
