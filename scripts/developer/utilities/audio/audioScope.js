var qml = Script.resourcesPath() + '/qml/AudioScope.qml';
var viewdim = Controller.getViewportDimensions();
var window = new OverlayWindow({
    title: 'Audio Scope',
    source: qml,
    width: 1200,
    height: 500
});
//window.setPosition(0.1*viewdim, 0.2*viewdim);
window.closed.connect(function () { Script.stop(); });