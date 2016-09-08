//
//  createGlobalEventBridge.js
//
//  Created by Anthony J. Thibault on 9/7/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Stick a EventBridge object in the global namespace.
var EventBridge;
(function () {
    // the TempEventBridge class queues up emitWebEvent messages and executes them when the real EventBridge is ready.
    // Similarly, it holds all scriptEventReceived callbacks, and hooks them up to the real EventBridge.
    function TempEventBridge() {
        var self = this;
        this._callbacks = [];
        this._messages = [];
        this.scriptEventReceived = {
            connect: function (cb) { self._callbacks.push(cb); }
        };
        this.emitWebEvent = function (message) {
            self._messages.push(message);
        }
    };

    EventBridge = new TempEventBridge();

    var webChannel = new QWebChannel(qt.webChannelTransport, function (channel) {
        // replace the TempEventBridge with the real one.
        var tempEventBridge = EventBridge;
        EventBridge = channel.objects.eventBridgeWrapper.eventBridge;
        tempEventBridge._callbacks.forEach(function (cb) {
            EventBridge.scriptEventReceived.connect(cb);
        });
        tempEventBridge._messages.forEach(function (msg) {
            EventBridge.emitWebEvent(msg);
        });
    });
})();
