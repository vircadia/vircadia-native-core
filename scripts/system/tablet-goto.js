"use strict";
/*jslint vars:true, plusplus:true, forin:true*/
/*global Window, Script, Tablet, HMD, Controller, Account, XMLHttpRequest, location, print*/

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

(function () { // BEGIN LOCAL_SCOPE

    var request = Script.require('request').request;
    var DEBUG = false;
    function debug() {
        if (!DEBUG) {
            return;
        }
        print('tablet-goto.js:', [].map.call(arguments, JSON.stringify));
    }

    var gotoQmlSource = "TabletAddressDialog.qml";
    var buttonName = "GOTO";
    var onGotoScreen = false;
    var shouldActivateButton = false;
    function ignore() { }

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var NORMAL_ICON    = "icons/tablet-icons/goto-i.svg";
    var NORMAL_ACTIVE  = "icons/tablet-icons/goto-a.svg";
    var WAITING_ICON   = "icons/tablet-icons/goto-msg.svg";
    var button = tablet.addButton({
        icon: NORMAL_ICON,
        activeIcon: NORMAL_ACTIVE,
        text: buttonName,
        sortOrder: 8
    });
    
    function fromQml(message) {
        console.debug('tablet-goto::fromQml: message = ', JSON.stringify(message));

        var response = {id: message.id, jsonrpc: "2.0"};
        switch (message.method) {
        case 'request':
            request(message.params, function (error, data) {
                debug('rpc', request, 'error:', error, 'data:', data);
                response.error = error;
                response.result = data;
                tablet.sendToQml(response);
            });
            return;
        default:
            response.error = {message: 'Unrecognized message', data: message};
        }
        tablet.sendToQml(response);
    }
    function messagesWaiting(isWaiting) {
        button.editProperties({
            icon: isWaiting ? WAITING_ICON : NORMAL_ICON
            // No need for a different activeIcon, because we issue messagesWaiting(false) when the button goes active anyway.
        });
    }

    var hasEventBridge = false;
    function wireEventBridge(on) {
        if (on) {
            if (!hasEventBridge) {
                tablet.fromQml.connect(fromQml);
                hasEventBridge = true;
            }
        } else {
            if (hasEventBridge) {
                tablet.fromQml.disconnect(fromQml);
                hasEventBridge = false;
            }
        }
    }

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
        ignore(type);
        if (url === gotoQmlSource) {
            onGotoScreen = true;
            shouldActivateButton = true;
            button.editProperties({isActive: shouldActivateButton});
            wireEventBridge(true);
            messagesWaiting(false);
            tablet.sendToQml({ method: 'refreshFeeds' })

        } else {
            shouldActivateButton = false;
            onGotoScreen = false;
            button.editProperties({isActive: shouldActivateButton});
            wireEventBridge(false);
        }
    }
    button.clicked.connect(onClicked);
    tablet.screenChanged.connect(onScreenChanged);

    var stories = {}, pingPong = false;
    function expire(id) {
        var options = {
            uri: location.metaverseServerUrl + '/api/v1/user_stories/' + id,
            method: 'PUT',
            json: true,
            body: {expire: "true"}
        };
        request(options, function (error, response) {
            debug('expired story', options, 'error:', error, 'response:', response);
            if (error || (response.status !== 'success')) {
                print("ERROR expiring story: ", error || response.status);
            }
        });
    }
    function pollForAnnouncements() {
        // We could bail now if !Account.isLoggedIn(), but what if we someday have system-wide announcments?
        var actions = 'announcement';
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
            debug(url, error, data);
            if (error || (data.status !== 'success')) {
                print("Error: unable to get", url,  error || data.status);
                return;
            }
            var didNotify = false, key;
            pingPong = !pingPong;
            data.user_stories.forEach(function (story) {
                var stored = stories[story.id], storedOrNew = stored || story;
                debug('story exists:', !!stored, storedOrNew);
                if ((storedOrNew.username === Account.username) && (storedOrNew.place_name !== location.placename)) {
                    if (storedOrNew.audience == 'for_connections') { // Only expire if we haven't already done so.
                        expire(story.id);
                    }
                    return; // before marking
                }
                storedOrNew.pingPong = pingPong;
                if (stored) { // already seen
                    return;
                }
                stories[story.id] = story;
                var message = story.username + " says something is happening in " + story.place_name + ". Open GOTO to join them.";
                Window.displayAnnouncement(message);
                didNotify = true;
            });
            for (key in stories) { // Any story we were tracking that was not marked, has expired.
                if (stories[key].pingPong !== pingPong) {
                    debug('removing story', key);
                    delete stories[key];
                }
            }
            if (didNotify) {
                messagesWaiting(true);
                if (HMD.isHandControllerAvailable()) {
                    var STRENGTH = 1.0, DURATION_MS = 60, HAND = 2; // both hands
                    Controller.triggerHapticPulse(STRENGTH, DURATION_MS, HAND);
                }
            } else if (!Object.keys(stories).length) { // If there's nothing being tracked, then any messageWaiting has expired.
                messagesWaiting(false);
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
