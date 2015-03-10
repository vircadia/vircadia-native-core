//
//  users.js
//  examples
//
//  Created by David Rowe on 9 Mar 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var usersWindow = (function () {

    var API_URL = "https://metaverse.highfidelity.io/api/v1/users?status=online",
        HTTP_GET_TIMEOUT = 60000,  // ms = 1 minute
        usersRequest,
        processUsers,
        usersTimedOut,
        usersTimer,
        UPDATE_TIMEOUT = 1000;  // ms = 1s

    function requestUsers() {
        usersRequest = new XMLHttpRequest();
        usersRequest.open("GET", API_URL, true);
        usersRequest.timeout = HTTP_GET_TIMEOUT * 1000;
        usersRequest.ontimeout = usersTimedOut;
        usersRequest.onreadystatechange = processUsers;
        usersRequest.send();
    }

    processUsers = function () {
        if (usersRequest.readyState === usersRequest.DONE) {
            if (usersRequest.status === 200) {
                // TODO: Process users data
            } else {
                print("Error: Request for users status returned " + usersRequest.status + " " + usersRequest.statusText);
            }

            usersTimer = Script.setTimeout(requestUsers, UPDATE_TIMEOUT);  // Update  while after finished processing.
        }
    };

    usersTimedOut = function () {
        print("Error: Request for users status timed out");
        usersTimer = Script.setTimeout(requestUsers, HTTP_GET_TIMEOUT);  // Try again after a longer delay.
    };

    function setUp() {

        requestUsers();
    }

    function tearDown() {
        Script.clearTimeout(usersTimer);

    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
