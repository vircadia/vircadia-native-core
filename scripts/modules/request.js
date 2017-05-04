"use strict";

//  request.js
//
//  Created by Cisco Fresquet on 04/24/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global module */
// @module request
//
// This module contains the `request` module implementation

// ===========================================================================================
module.exports = {

    // ------------------------------------------------------------------
    request: function (options, callback) { // cb(error, responseOfCorrectContentType) of url. A subset of npm request.
            var httpRequest = new XMLHttpRequest(), key;
        // QT bug: apparently doesn't handle onload. Workaround using readyState.
        httpRequest.onreadystatechange = function () {
            var READY_STATE_DONE = 4;
            var HTTP_OK = 200;
            if (httpRequest.readyState >= READY_STATE_DONE) {
                var error = (httpRequest.status !== HTTP_OK) && httpRequest.status.toString() + ':' + httpRequest.statusText,
                    response = !error && httpRequest.responseText,
                    contentType = !error && httpRequest.getResponseHeader('content-type');
                if (!error && contentType.indexOf('application/json') === 0) { // ignoring charset, etc.
                    try {
                        response = JSON.parse(response);
                    } catch (e) {
                        error = e;
                    }
                }
                if (error) {
                    response = { statusCode: httpRequest.status };
                }
                callback(error, response);
            }
        };
        if (typeof options === 'string') {
            options = { uri: options };
        }
        if (options.url) {
            options.uri = options.url;
        }
        if (!options.method) {
            options.method = 'GET';
        }
        if (options.body && (options.method === 'GET')) { // add query parameters
            var params = [], appender = (-1 === options.uri.search('?')) ? '?' : '&';
            for (key in options.body) {
                if (options.body.hasOwnProperty(key)) {
                    params.push(key + '=' + options.body[key]);
                }
            }
            options.uri += appender + params.join('&');
            delete options.body;
        }
        if (options.json) {
            options.headers = options.headers || {};
            options.headers["Content-type"] = "application/json";
            options.body = JSON.stringify(options.body);
        }
        for (key in options.headers || {}) {
            if (options.headers.hasOwnProperty(key)) {
                httpRequest.setRequestHeader(key, options.headers[key]);
            }
        }
        httpRequest.open(options.method, options.uri, true);
        httpRequest.send(options.body);
        }
};

// ===========================================================================================
// @function - debug logging
function debug() {
    print('RequestModule | ' + [].slice.call(arguments).join(' '));
}
