
//public slots:
//    void emitWebEvent(const QString& data);
//    void emitScriptEvent(const QString& data);
//
//signals:
//    void webEventReceived(const QString& data);
//    void scriptEventReceived(const QString& data);
//

var EventBridge;

EventBridgeConnectionProxy = function(parent) {
    this.parent = parent;
    this.realSignal = this.parent.realBridge.scriptEventReceived
    this.webWindowId = this.parent.webWindow.windowId;
}

EventBridgeConnectionProxy.prototype.connect = function(callback) {
    var that = this;
    this.realSignal.connect(function(id, message) {
        if (id === that.webWindowId) { callback(message); }
    });
}

EventBridgeProxy = function(webWindow) {
    this.webWindow = webWindow;
    this.realBridge = this.webWindow.eventBridge;  
    this.scriptEventReceived = new EventBridgeConnectionProxy(this);
}

EventBridgeProxy.prototype.emitWebEvent = function(data) {
    this.realBridge.emitWebEvent(data);
}

openEventBridge = function(callback) {
    EVENT_BRIDGE_URI = "ws://localhost:51016";
    socket = new WebSocket(this.EVENT_BRIDGE_URI);
    
    socket.onclose = function() { 
        console.error("web channel closed"); 
    };

    socket.onerror = function(error) { 
        console.error("web channel error: " + error); 
    };

    socket.onopen = function() {
        channel = new QWebChannel(socket, function(channel) {
            console.log("Document url is " + document.URL);
            var webWindow = channel.objects[document.URL.toLowerCase()];
            console.log("WebWindow is " + webWindow)
            eventBridgeProxy = new EventBridgeProxy(webWindow);
            EventBridge = eventBridgeProxy; 
            if (callback) {  callback(eventBridgeProxy); }
        });
    }
}

