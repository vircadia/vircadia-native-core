print("Launching web window");
qmlWindow = new OverlayWindow({
    title: 'Test Qml', 
    source: "qrc:///qml/OverlayWindowTest.qml", 
    height: 240, 
    width: 320, 
    toolWindow: false,
    visible: true
});

Script.setInterval(function() {
    qmlWindow.raise();
}, 2 * 1000);
