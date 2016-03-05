
//public slots:
//    void emitWebEvent(const QString& data);
//    void emitScriptEvent(const QString& data);
//
//signals:
//    void webEventReceived(const QString& data);
//    void scriptEventReceived(const QString& data);
//

var EventBridge;

openEventBridge = function(callback) {
    new QWebChannel(qt.webChannelTransport, function(channel) {
        console.log("uid " + EventBridgeUid);
        EventBridge = channel.objects[EventBridgeUid];
        callback(EventBridge);
    });
}

