// usage:
//   node avatar-json-to-dot.js /path/to/avatar-animaton.json > out.dot
//
// Then if you have graphviz installed you can run the following command to generate a png.
//   dot -Tpng out.dot > out.png

var fs = require('fs');
var filename = process.argv[2];

function dumpNodes(node) {
    node.children.forEach(function (child) {
        console.log('    ' + node.id + ' -> ' + child.id + ';');
        dumpNodes(child);
    });
}

fs.readFile(filename, 'utf8', function (err, data) {
    if (err) {
        console.log('error opening ' + filename + ', err = ' + err);
    } else {
        var graph = JSON.parse(data);
        console.log('digraph graphname {');
        console.log('    rankdir = "LR";');
        dumpNodes(graph.root);
        console.log('}');
    }
});
