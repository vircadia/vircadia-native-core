(function() { // BEGIN LOCAL_SCOPE

function task_traverse(root, functor, depth) {
    var subs = root.findChildren(/.*/)
    depth++;
    for (var i = 0; i <subs.length; i++) {
        functor(subs[i], depth, i)
        task_traverse(subs[i], functor, depth)
    }    
}   

function task_traverseTree(root, functor) {
    task_traverse(root, functor, 0);    
} 

function task_jobProps(job) {
    var keys = Object.keys(job)
    var props = [];
    for (var k=0; k < keys.length;p++) {
        var prop = keys[p]
        
    }   
    return props; 
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

task_traverseTree(Render, printJob);

var qml = Script.resolvePath('inspectEngine.qml');
var window = new OverlayWindow({
    title: 'Inspect Engine',
    source: qml,
    width: 250, 
    height: 300
});
window.closed.connect(function() { Script.stop(); });

}()); // END LOCAL_SCOPE