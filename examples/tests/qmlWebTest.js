print("Launching web window");

var htmlUrl =  Script.resolvePath("..//html/qmlWebTest.html")
webWindow = new OverlayWebWindow('Test Event Bridge', htmlUrl, 320, 240, false);
webWindow.webEventReceived.connect(function(data) {
    print("JS Side event received: " + data);
});

Script.setInterval(function() {
    var message = [ Math.random(), Math.random() ];
    print("JS Side sending: " + message); 
    webWindow.emitScriptEvent(message);
}, 5 * 1000);

Script.scriptEnding.connect(function(){
    webWindow.close();
    webWindow.deleteLater();
});

