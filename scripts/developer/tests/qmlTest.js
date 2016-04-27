print("Launching web window");
qmlWindow = new OverlayWindow({
    title: 'Test Qml', 
    source: "https://s3.amazonaws.com/DreamingContent/qml/content.qml", 
    height: 240, 
    width: 320, 
    toolWindow: false,
    visible: true
});

Script.setInterval(function() {
    qmlWindow.raise();
}, 2 * 1000);
