(function() { // BEGIN LOCAL_SCOPE

function traverse(root, functor, depth) {
    var subs = root.findChildren(/.*/)
    depth++;
    for (var i = 0; i <subs.length; i++) {
        functor(subs[i], depth, i)
        traverse(subs[i], functor, depth)
    }    
}   

function traverseTree(root, functor) {
    traverse(root, functor, 0);    
}   

function printJob(job, depth, index) {
    var tab = "  "
    var depthTab = "";
    for (var d = 0; d < depth; d++) { depthTab += tab }
    print(depthTab + index + " " + job.objectName) 
    var props = Object.keys(job)
    for (var p=0; p < props.length;p++) {
        var prop = props[p]
        print(depthTab + tab + String(props[p]));
    }
}

//traverseTree(Render, printJob);

var qml = Script.resolvePath('inspectEngine.qml');
var window = new OverlayWindow({
    title: 'Inspect Engine',
    source: qml,
    width: 250, 
    height: 300
});
window.closed.connect(function() { Script.stop(); });

}()); // END LOCAL_SCOPE