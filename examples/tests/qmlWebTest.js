print("Launching web window");

var htmlUrl =  Script.resolvePath("..//html/qmlWebTest.html")
webWindow = new OverlayWebWindow('Test Event Bridge', htmlUrl, 320, 240, false);
print("JS Side window: " + webWindow);
print("JS Side bridge: " + webWindow.eventBridge);
webWindow.eventBridge.webEventReceived.connect(function(data) {
    print("JS Side event received: " + data);
});

var titles = ["A", "B", "C"];
var titleIndex = 0;

Script.setInterval(function() {
    webWindow.eventBridge.emitScriptEvent("JS Event sent");
    var size = webWindow.size;
    var position = webWindow.position;
    print("Window url: " + webWindow.url)
    print("Window visible: " + webWindow.visible)
    print("Window size:  " + size.x + "x" + size.y)
    print("Window pos:   " + position.x + "x" + position.y)
    webWindow.setVisible(!webWindow.visible);
    webWindow.setTitle(titles[titleIndex]);
    webWindow.setSize(320 + Math.random() * 100, 240 + Math.random() * 100);
    titleIndex += 1;
    titleIndex %= titles.length;
}, 2 * 1000);

Script.setTimeout(function() {
    print("Closing script");
    webWindow.close();
    Script.stop();
}, 15 * 1000)
