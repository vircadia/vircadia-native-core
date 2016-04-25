var qml = Script.resolvePath('playaPerformanceTest.qml');
qmlWindow = new OverlayWindow({
    title: 'Test Qml', 
    source: qml, 
    height: 320, 
    width: 640, 
    toolWindow: false,
    visible: true
});


