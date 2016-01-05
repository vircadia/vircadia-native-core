print("Launching web window");
qmlWindow = new OverlayWindow({
    title: 'Test Qml', 
    source: "https://s3.amazonaws.com/DreamingContent/qml/content.qml", 
    height: 240, 
    width: 320, 
    toolWindow: false,
    visible: true
});

//qmlWindow.eventBridge.webEventReceived.connect(function(data) {
//    print("JS Side event received: " + data);
//});
//
//var titles = ["A", "B", "C"];
//var titleIndex = 0;
//
//Script.setInterval(function() {
//    qmlWindow.eventBridge.emitScriptEvent("JS Event sent");
//    var size = qmlWindow.size;
//    var position = qmlWindow.position;
//    print("Window visible: " + qmlWindow.visible)
//    if (qmlWindow.visible) {
//        print("Window size:  " + size.x + "x" + size.y)
//        print("Window pos:   " + position.x + "x" + position.y)
//        qmlWindow.setVisible(false);
//    } else {
//        qmlWindow.setVisible(true);
//        qmlWindow.setTitle(titles[titleIndex]);
//        qmlWindow.setSize(320 + Math.random() * 100, 240 + Math.random() * 100);
//        titleIndex += 1;
//        titleIndex %= titles.length;
//    }
//}, 2 * 1000);
//
//Script.setTimeout(function() {
//    print("Closing script");
//    qmlWindow.close();
//    Script.stop();
//}, 15 * 1000)
