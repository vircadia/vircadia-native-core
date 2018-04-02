//Script.include('../lib/jet/jet.js');




(function() { // BEGIN LOCAL_SCOPE
    //jet.task_traverseTree(Render, printJob);
    var message = "test";
 /*   var functor = job_print_functor(function (line) { message += line + "\n"; });
        
    task_traverseTree(Workload, functor
    );
   */ 
    
    print(message)

var qml = Script.resolvePath('../lib/jet/TaskList.qml');
var window = new OverlayWindow({
    title: 'Inspect Engine',
    source: qml,
    width: 250, 
    height: 300
});

window.sendToQml(message)

window.closed.connect(function () { Script.stop(); });
Script.scriptEnding.connect(function () {
  /*  var geometry = JSON.stringify({
        x: window.position.x,
        y: window.position.y,
        width: window.size.x,
        height: window.size.y
    })

    Settings.setValue(HMD_DEBUG_WINDOW_GEOMETRY_KEY, geometry);*/
    window.close();
})

}()); // END LOCAL_SCOPE