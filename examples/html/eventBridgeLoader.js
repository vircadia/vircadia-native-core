
//public slots:
//    void emitWebEvent(const QString& data);
//    void emitScriptEvent(const QString& data);
//
//signals:
//    void webEventReceived(const QString& data);
//    void scriptEventReceived(const QString& data);
//

var EventBridge;
var WebChannel;

openEventBridge = function(callback) {
    WebChannel = new QWebChannel(qt.webChannelTransport, function (channel) {
        EventBridge = WebChannel.objects.eventBridgeWrapper.eventBridge;
        callback(EventBridge);
    });
}
