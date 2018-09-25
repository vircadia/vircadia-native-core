"use strict";
/* jslint vars:true, plusplus:true, forin:true */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
/* global Window, Script, Tablet, HMD, Controller, Account, XMLHttpRequest, location, print */

//
//  tablet-goto.js
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
var AppUi = Script.require('appUi');
var DEBUG = false;
function debug() {
    if (!DEBUG) {
        return;
    }
    print('tablet-goto.js:', [].map.call(arguments, JSON.stringify));
}

var stories = {}, pingPong = false;
function expire(id) {
    var options = {
        uri: Account.metaverseServerURL + '/api/v1/user_stories/' + id,
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
var PER_PAGE_DEBUG = 10;
var PER_PAGE_NORMAL = 100;
function pollForAnnouncements() {
    // We could bail now if !Account.isLoggedIn(), but what if we someday have system-wide announcments?
    var actions = 'announcement';
    var count = DEBUG ? PER_PAGE_DEBUG : PER_PAGE_NORMAL;
    var options = [
        'now=' + new Date().toISOString(),
        'include_actions=' + actions,
        'restriction=' + (Account.isLoggedIn() ? 'open,hifi' : 'open'),
        'require_online=true',
        'protocol=' + encodeURIComponent(Window.protocolSignature()),
        'per_page=' + count
    ];
    var url = Account.metaverseServerURL + '/api/v1/user_stories?' + options.join('&');
    request({
        uri: url
    }, function (error, data) {
        debug(url, error, data);
        if (error || (data.status !== 'success')) {
            print("Error: unable to get", url, error || data.status);
            return;
        }
        var didNotify = false, key;
        pingPong = !pingPong;
        data.user_stories.forEach(function (story) {
            var stored = stories[story.id], storedOrNew = stored || story;
            debug('story exists:', !!stored, storedOrNew);
            if ((storedOrNew.username === Account.username) && (storedOrNew.place_name !== location.placename)) {
                if (storedOrNew.audience === 'for_connections') { // Only expire if we haven't already done so.
                    expire(story.id);
                }
                return; // before marking
            }
            storedOrNew.pingPong = pingPong;
            if (stored) { // already seen
                return;
            }
            stories[story.id] = story;
            var message = story.username + " " + story.action_string + " in " +
                story.place_name + ". Open GOTO to join them.";
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
            ui.messagesWaiting(true);
            if (HMD.isHandControllerAvailable()) {
                var STRENGTH = 1.0, DURATION_MS = 60, HAND = 2; // both hands
                Controller.triggerHapticPulse(STRENGTH, DURATION_MS, HAND);
            }
        } else if (!Object.keys(stories).length) { // If there's nothing being tracked, then any messageWaiting has expired.
            ui.messagesWaiting(false);
        }
    });
}
var MS_PER_SEC = 1000;
var DEBUG_POLL_TIME_SEC = 10;
var NORMAL_POLL_TIME_SEC = 60;
var ANNOUNCEMENTS_POLL_TIME_MS = (DEBUG ? DEBUG_POLL_TIME_SEC : NORMAL_POLL_TIME_SEC) * MS_PER_SEC;
var pollTimer = Script.setInterval(pollForAnnouncements, ANNOUNCEMENTS_POLL_TIME_MS);

function gotoOpened() {
    ui.messagesWaiting(false);
}

var ui;
var GOTO_QML_SOURCE = "hifi/tablet/TabletAddressDialog.qml";
var BUTTON_NAME = "GOTO";
function startup() {
    ui = new AppUi({
        buttonName: BUTTON_NAME,
        sortOrder: 8,
        onOpened: gotoOpened,
        home: GOTO_QML_SOURCE
    });
}

function shutdown() {
    Script.clearInterval(pollTimer);
}

startup();
Script.scriptEnding.connect(shutdown);
}()); // END LOCAL_SCOPE
