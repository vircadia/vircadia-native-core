//
//  testWebSocket.js
//  examples
//
//  Created by Thijs Wenker on 8/18/15
//  Copyright 2015 High Fidelity, Inc.
//
//  WebSocket and WebSocketServer Tests
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

Script.include("../../libraries/unitTest.js");

// We set the unit testing timeout to 1000 milliseconds by default. Please increase if the test fails due to a slow connection.
const UNITTEST_TIMEOUT = 1000;
const WEBSOCKET_PING_URL = "ws://echo.websocket.org";
// Please do not register the following domain + gTLD:
const WEBSOCKET_INVALID_URL = "ws://thisisnotavaliddomainname.invalid";
const TEST_MESSAGE = "This is a test message.";

var unitTests = new SequentialUnitTester();

unitTests.addTest("Test default WebSocket values", function(finished) {
    var _this = this;
    var webSocket = new WebSocket(WEBSOCKET_PING_URL);
    
    webSocket.onmessage = this.registerCallbackFunction(function(event) {
        _this.assertEquals(TEST_MESSAGE, event.data, "event.data should be '" + TEST_MESSAGE + "'");
        webSocket.close();
    });
    webSocket.onopen = this.registerCallbackFunction(function(event) {
        _this.assertEquals(webSocket.OPEN, webSocket.readyState, "readyState should be OPEN");
        webSocket.send(TEST_MESSAGE);
    });
    webSocket.onclose = this.registerCallbackFunction(function(event) {
        _this.assertEquals(webSocket.CLOSED, webSocket.readyState, "readyState should be CLOSED");
        _this.done();
    });
    this.assertEquals(webSocket.CONNECTING, webSocket.readyState, "readyState should be CONNECTING");
    this.assertEquals("blob", webSocket.binaryType, "binaryType should be 'blob'");
    this.assertEquals(0, webSocket.bufferedAmount, "bufferedAmount should be 0");
    this.assertEquals("", webSocket.extensions, "extensions should be an empty string by default");
    this.assertEquals("", webSocket.protocol, "protocol should be an empty string by default");
    this.assertEquals(WEBSOCKET_PING_URL, webSocket.url, "url should be '" + WEBSOCKET_PING_URL + "'");
}, UNITTEST_TIMEOUT);

unitTests.addTest("Test WebSocket invalid URL", function(finished) {
    var _this = this;
    var webSocket = new WebSocket(WEBSOCKET_INVALID_URL);
    var hadError = false;
    webSocket.onerror = this.registerCallbackFunction(function() {
        hadError = true;
        _this.done();
    });
    webSocket.onclose = this.registerCallbackFunction(function(event) {
        _this.assertEquals(webSocket.CLOSED, webSocket.readyState, "readyState should be CLOSED");
    });
    this.assertEquals(webSocket.CONNECTING, webSocket.readyState, "readyState should be CONNECTING");
    this.assertEquals(WEBSOCKET_INVALID_URL, webSocket.url, "url should be '" + WEBSOCKET_INVALID_URL + "'");
}, UNITTEST_TIMEOUT);

if (this.WebSocketServer === undefined) {
    print("Skipping WebSocketServer tests.");
} else {
    unitTests.addTest("Test WebSocketServer with three clients", function(finished) {
        var _this = this;
        const NUMBER_OF_CLIENTS = 3;
        var connectedClients = 0;
        var respondedClients = 0;
        var webSocketServer = new WebSocketServer();
        _this.assertEquals(true, webSocketServer.listening, "listening should be true");
        webSocketServer.newConnection.connect(this.registerCallbackFunction(function(newClient) {
            connectedClients++;
            newClient.onmessage = _this.registerCallbackFunction(function(event) {
                var data = JSON.parse(event.data);
                _this.assertEquals(TEST_MESSAGE, data.message, "data.message should be '" + TEST_MESSAGE + "'");
                respondedClients++;
                if (respondedClients === NUMBER_OF_CLIENTS) {
                    webSocketServer.close();
                    _this.assertEquals(false, webSocketServer.listening, "listening should be false");
                    _this.done();
                }
            });
            newClient.send(JSON.stringify({message: TEST_MESSAGE, client: connectedClients}));
        }));
        var newSocket1 = new WebSocket(webSocketServer.url);
        newSocket1.onmessage = this.registerCallbackFunction(function(event) {
            newSocket1.send(event.data);
        });
        var newSocket2 = new WebSocket(webSocketServer.url);
        newSocket2.onmessage = this.registerCallbackFunction(function(event) {
            newSocket2.send(event.data);
        });
        var newSocket3 = new WebSocket(webSocketServer.url);
        newSocket3.onmessage = this.registerCallbackFunction(function(event) {
            newSocket3.send(event.data);
        });
    }, UNITTEST_TIMEOUT);
}

unitTests.run();
