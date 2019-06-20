//
//  testXMLHttpRequest.js
//  examples
//
//  Created by Ryan Huffman on 5/4/14
//  Copyright 2014 High Fidelity, Inc.
//
//  XMLHttpRequest Tests
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/unitTest.js");

test("Test default request values", function(finished) {
    var req = new XMLHttpRequest();

    this.assertEquals(req.UNSENT, req.readyState, "readyState should be UNSENT");
    this.assertEquals(0, req.status, "status should be `0` by default");
    this.assertEquals("", req.statusText, "statusText should be empty string by default");
    this.assertEquals("", req.getAllResponseHeaders(), "getAllResponseHeaders() should return empty string by default");
    this.assertEquals("", req.response, "response should be empty string by default");
    this.assertEquals("", req.responseText, "responseText should be empty string by default");
    this.assertEquals("", req.responseType, "responseType should be empty string by default");
    this.assertEquals(0, req.timeout, "timeout should be `0` by default");
    this.assertEquals(0, req.errorCode, "there should be no error by default");
});


test("Test readyStates", function() {
    var req = new XMLHttpRequest();
    var state = req.readyState;

    var statesVisited = [true, false, false, false, false]

    req.onreadystatechange = function() {
        statesVisited[req.readyState] = true;
    };

    req.open("GET", "https://gist.githubusercontent.com/huffman/33cc618fec183d1bccd0/raw/test.json", false);
    req.send();
    for (var i = 0; i <= req.DONE; i++) {
        this.assertEquals(true, statesVisited[i], i + " should be set");
    }
    this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
});

test("Test TEXT request", function() {
    var req = new XMLHttpRequest();
    var state = req.readyState;

    req.open("GET", "https://gist.githubusercontent.com/huffman/33cc618fec183d1bccd0/raw/test.json", false);
    req.send();

    this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
    this.assertEquals(200, req.status, "status should be `200`");
    this.assertEquals(0, req.errorCode);
    this.assertEquals("OK", req.statusText, "statusText should be `OK`");
    this.assertNotEquals("", req.getAllResponseHeaders(), "headers should no longer be empty string");
    this.assertNull(req.getResponseHeader('invalidheader'), "invalid header should return `null`");
    this.assertEquals("GitHub.com", req.getResponseHeader('Server'), "Server header should be GitHub.com");
    this.assertEquals('{"id": 1}', req.response);
    this.assertEquals('{"id": 1}', req.responseText);
});

test("Test JSON request", function() {
    var req = new XMLHttpRequest();
    var state = req.readyState;

    req.responseType = "json";
    req.open("GET", "https://gist.githubusercontent.com/huffman/33cc618fec183d1bccd0/raw/test.json", false);
    req.send();

    this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
    this.assertEquals(200, req.status, "status should be `200`");
    this.assertEquals(0, req.errorCode);
    this.assertEquals("OK", req.statusText, "statusText should be `OK`");
    this.assertNotEquals("", req.getAllResponseHeaders(), "headers should no longer be empty string");
    this.assertNull(req.getResponseHeader('invalidheader'), "invalid header should return `null`");
    this.assertEquals("GitHub.com", req.getResponseHeader('Server'), "Server header should be GitHub.com");
    this.assertHasProperty('id', req.response);
    this.assertEquals(1, req.response.id);
    this.assertEquals('{"id": 1}', req.responseText);
});

test("Test Bad URL", function() {
    var req = new XMLHttpRequest();
    var state = req.readyState;

    req.open("POST", "hifi://domain/path", false);
    req.send();

    this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
    this.assertNotEquals(0, req.errorCode);
});

test("Test Bad Method Error", function() {
    var req = new XMLHttpRequest();
    var state = req.readyState;

    req.open("POST", "https://www.google.com", false);

    req.send("randomdata");

    this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
    this.assertEquals(405, req.status);
    this.assertEquals(202, req.errorCode);

    req.open("POST", "https://www.google.com", false)
    req.send();

    this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
    this.assertEquals(405, req.status);
    this.assertEquals(202, req.errorCode);
});

test("Test abort", function() {
    var req = new XMLHttpRequest();
    var state = req.readyState;

    req.open("POST", "https://www.google.com", true)
    req.send();
    req.abort();

    this.assertEquals(0, req.status);
    this.assertEquals(0, req.errorCode);
});

test("Test timeout", function() {
    var req = new XMLHttpRequest();
    var state = req.readyState;
    var timedOut = false;

    req.ontimeout = function() {
        timedOut = true;
    };

    req.open("POST", "https://gist.githubusercontent.com/huffman/33cc618fec183d1bccd0/raw/test.json", false)
    req.timeout = 1;
    req.send();

    this.assertEquals(true, timedOut, "request should have timed out");
    this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
    this.assertEquals(0, req.status, "status should be `0`");
    this.assertEquals(4, req.errorCode, "4 is the timeout error code for QNetworkReply::NetworkError");
});
