(function() { // BEGIN LOCAL_SCOPE
var qml = Script.resolvePath('./workloadInspector.qml');
var window = new OverlayWindow({
    title: 'Inspect Engine',
    source: qml,
    width: 400, 
    height: 600
});

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