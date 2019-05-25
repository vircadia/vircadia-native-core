//
//  RootHttpRequest.qml
//  qml/hifi
//
//  Create an item of this in the ROOT qml to be able to make http requests.
//  Used by PSFListModel.qml
//
//  Created by Howard Stearns on 5/29/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

Item {
    property var httpCalls: ({});
    property var httpCounter: 0;
    // Public function for initiating an http request.
    // REQUIRES parent to be root to have sendToScript!
    function request(options, callback) {
        httpCalls[httpCounter] = callback;
        var message = {method: 'http.request', params: options, id: httpCounter++, jsonrpc: "2.0"};
        parent.sendToScript(message);
    }
    // REQUIRES that parent/root handle http.response message.method in fromScript, by calling this function.
    function handleHttpResponse(message) {
        var callback = httpCalls[message.id]; // FIXME: as different top level tablet apps gets loaded, the id repeats. We should drop old app callbacks without warning.
        if (!callback) {
            console.warn('No callback for', JSON.stringify(message));
            return;
        }
        delete httpCalls[message.id];
        callback(message.error, message.response);
    }
}
