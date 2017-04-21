"use strict";

//
//  goto.js
//  scripts/system/
//
//  Created by Dante Ruiz on 8 February 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    var gotoQmlSource = "TabletAddressDialog.qml";
    var buttonName = "GOTO";
    var onGotoScreen = false;
    var shouldActivateButton = false;

    function onClicked() {
        if (onGotoScreen) {
            // for toolbar-mode: go back to home screen, this will close the window.
            tablet.gotoHomeScreen();
        } else {
            shouldActivateButton = true;
            tablet.loadQMLSource(gotoQmlSource);
            onGotoScreen = true;
        }
    }

    function onScreenChanged(type, url) {
        if (url === gotoQmlSource) {
            onGotoScreen = true;
            shouldActivateButton = true;
            button.editProperties({isActive: shouldActivateButton});
            messagesWaiting(false);
        } else { 
            shouldActivateButton = false;
            onGotoScreen = false;
            button.editProperties({isActive: shouldActivateButton});
        }
    }

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var NORMAL_ICON    = "icons/tablet-icons/goto-i.svg";
    var NORMAL_ACTIVE  = "icons/tablet-icons/goto-a.svg";
    var WAITING_ICON   = "icons/tablet-icons/help-i.svg"; // To be changed when we get the artwork.
    var WAITING_ACTIVE = "icons/tablet-icons/help-a.svg";
    var button = tablet.addButton({
        icon: NORMAL_ICON,
        activeIcon: NORMAL_ACTIVE,
        text: buttonName,
        sortOrder: 8
    });
    function messagesWaiting(isWaiting) {
        button.editProperties({
            icon: isWaiting ? WAITING_ICON : NORMAL_ICON,
            activeIcon: isWaiting ? WAITING_ACTIVE : NORMAL_ACTIVE
        });
    }

    button.clicked.connect(onClicked);
    tablet.screenChanged.connect(onScreenChanged);

    var METAVERSE_BASE = location.metaverseServerUrl;
    function request(options, callback) { // cb(error, responseOfCorrectContentType) of url. A subset of npm request.
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
                callback(error, response);
            }
        };
        if (typeof options === 'string') {
            options = {uri: options};
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
                params.push(key + '=' + options.body[key]);
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
            httpRequest.setRequestHeader(key, options.headers[key]);
        }
        httpRequest.open(options.method, options.uri, true);
        httpRequest.send(options.body);
    }

    var stories = {};
    var DEBUG = false;
    function pollForAnnouncements() {
        var actions = DEBUG ? 'snapshot' : 'announcement';
        var count = DEBUG ? 10 : 100;
        var options = [
            'now=' + new Date().toISOString(),
            'include_actions=' + actions,
            'restriction=' + (Account.isLoggedIn() ? 'open,hifi' : 'open'),
            'require_online=true',
            'protocol=' + encodeURIComponent(location.protocolVersion()),
            'per_page=' + count
        ];
        var url = location.metaverseServerUrl + '/api/v1/user_stories?' + options.join('&');
        request({
            uri: url
        }, function (error, data) {
            if (error || (data.status !== 'success')) {
                print("Error: unable to get", url,  error || response.status);
                return;
            }
            var didNotify = false;
            data.user_stories.forEach(function (story) {
                if (stories[story.id]) { // already seen
                    return;
                }
                stories[story.id] = story;
                var message = story.username + " says something is happending in " + story.place_name + ". Open GOTO to join them.";
                Window.displayAnnouncement(message);
                didNotify = true;
            });
            if (didNotify) {
                messagesWaiting(true);
                if (HMD.isHandControllerAvailable()) {
                    var STRENGTH = 1.0, DURATION_MS = 60, HAND = 2; // both hands
                    Controller.triggerHapticPulse(STRENGTH, DURATION_MS, HAND);
                }
            }
        });
    }
    var ANNOUNCEMENTS_POLL_TIME_MS = (DEBUG ? 10 : 60) * 1000;
    var pollTimer = Script.setInterval(pollForAnnouncements, ANNOUNCEMENTS_POLL_TIME_MS);

    Script.scriptEnding.connect(function () {
        Script.clearInterval(pollTimer);
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
        tablet.screenChanged.disconnect(onScreenChanged);
    });

}()); // END LOCAL_SCOPE
