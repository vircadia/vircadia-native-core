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

Script.include("Test.js");

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


var localFile = Window.browse("Find defaultScripts.js file ...", "", "defaultScripts.js (defaultScripts.js)");

if (localFile !== null) {

    localFile = "file:///" + localFile;
    
    test("Test GET local file synchronously", function () {
        var req = new XMLHttpRequest();

        var statesVisited = [true, false, false, false, false]
        req.onreadystatechange = function () {
            statesVisited[req.readyState] = true;
        };

        req.open("GET", localFile, false);
        req.send();

        this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
        this.assertEquals(200, req.status, "status should be `200`");
        this.assertEquals("OK", req.statusText, "statusText should be `OK`");
        this.assertEquals(0, req.errorCode);
        this.assertEquals("", req.getAllResponseHeaders(), "headers should be null");
        this.assertContains("High Fidelity", req.response.substring(0, 100), "expected text not found in response")

        for (var i = 0; i <= req.DONE; i++) {
            this.assertEquals(true, statesVisited[i], i + " should be set");
        }
    });

    test("Test GET nonexistent local file", function () {
        var nonexistentFile = localFile.replace(".js", "NoExist.js");

        var req = new XMLHttpRequest();
        req.open("GET", nonexistentFile, false);
        req.send();

        this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
        this.assertEquals(404, req.status, "status should be `404`");
        this.assertEquals("Not Found", req.statusText, "statusText should be `Not Found`");
        this.assertNotEquals(0, req.errorCode);
    });

    test("Test GET local file already open", function () {
        // Can't open file exclusively in order to test.
    });

    test("Test GET local file with data not implemented", function () {
        var req = new XMLHttpRequest();
        req.open("GET", localFile, true);
        req.send("data");

        this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
        this.assertEquals(501, req.status, "status should be `501`");
        this.assertEquals("Not Implemented", req.statusText, "statusText should be `Not Implemented`");
        this.assertNotEquals(0, req.errorCode);
    });

    test("Test GET local file asynchronously not implemented", function () {
        var req = new XMLHttpRequest();
        req.open("GET", localFile, true);
        req.send();

        this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
        this.assertEquals(501, req.status, "status should be `501`");
        this.assertEquals("Not Implemented", req.statusText, "statusText should be `Not Implemented`");
        this.assertNotEquals(0, req.errorCode);
    });

    test("Test POST local file not implemented", function () {
        var req = new XMLHttpRequest();
        req.open("POST", localFile, false);
        req.send();

        this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
        this.assertEquals(501, req.status, "status should be `501`");
        this.assertEquals("Not Implemented", req.statusText, "statusText should be `Not Implemented`");
        this.assertNotEquals(0, req.errorCode);
    });

    test("Test local file username and password not implemented", function () {
        var req = new XMLHttpRequest();
        req.open("GET", localFile, false, "username", "password");
        req.send();

        this.assertEquals(req.DONE, req.readyState, "readyState should be DONE");
        this.assertEquals(501, req.status, "status should be `501`");
        this.assertEquals("Not Implemented", req.statusText, "statusText should be `Not Implemented`");
        this.assertNotEquals(0, req.errorCode);
    });

} else {
    print("Local file operation not tested");
}
