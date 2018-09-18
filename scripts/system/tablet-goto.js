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
var AppUi = Script.require('appUi');

function gotoOpened() {
    ui.messagesWaiting(false);
}

function notificationDataProcessPage(data) {
    return data.user_stories;
}

var shouldShowDot = false;
var pingPong = false;
var storedAnnouncements = {};
var storedFeaturedStories = {};
function notificationPollCallback(userStoriesArray) {
    //
    // START logic for keeping track of new info
    //
    pingPong = !pingPong;
    var totalCountedStories = 0;
    userStoriesArray.forEach(function (story) {
        if (story.audience !== "for_connections" &&
            story.audience !== "for_feed") {
            return;
        } else {
            totalCountedStories++;
        }

        var stored = storedAnnouncements[story.id] || storedFeaturedStories[story.id];
        var storedOrNew = stored || story;
        storedOrNew.pingPong = pingPong;
        if (stored) {
            return;
        }

        if (story.audience === "for_connections") {
            storedAnnouncements[story.id] = story;
        } else if (story.audience === "for_feed") {
            storedFeaturedStories[story.id] = story;
        }
    });
    var key;
    for (key in storedAnnouncements) {
        if (storedAnnouncements[key].pingPong !== pingPong) {
            delete storedAnnouncements[key];
        }
    }
    for (key in storedFeaturedStories) {
        if (storedFeaturedStories[key].pingPong !== pingPong) {
            delete storedFeaturedStories[key];
        }
    }
    //
    // END logic for keeping track of new info
    //

    var notificationCount = Object.keys(storedAnnouncements).length +
        Object.keys(storedFeaturedStories).length;
    shouldShowDot = totalCountedStories > 0 || (notificationCount > 0 && shouldShowDot);
    ui.messagesWaiting(shouldShowDot && !ui.isOpen);

    if (notificationCount > 0 && !ui.isOpen) {
        var message;
        if (!ui.notificationInitialCallbackMade) {
            message = "You have " + userStoriesArray.length + "event invitations pending! " +
                "Open GOTO to see them.";
            ui.notificationDisplayBanner(message);
        } else {
            for (key in storedAnnouncements) {
                message = storedAnnouncements[key].username + " says something is happening in " +
                    storedAnnouncements[key].place_name + "! Open GOTO to join them.";
                ui.notificationDisplayBanner(message);
            }
            for (key in storedFeaturedStories) {
                message = storedFeaturedStories[key].username + " has invited you to an event in " +
                    storedFeaturedStories[key].place_name + "! Open GOTO to join them.";
                ui.notificationDisplayBanner(message);
            }
        }
    }
}

function isReturnedDataEmpty(data) {
    var storiesArray = data.user_stories;
    return storiesArray.length === 0;
}

var ui;
var GOTO_QML_SOURCE = "hifi/tablet/TabletAddressDialog.qml";
var BUTTON_NAME = "GOTO";
function startup() {
    var options = [
        'include_actions=announcement',
        'restriction=open,hifi',
        'require_online=true',
        'protocol=' + encodeURIComponent(Window.protocolSignature()),
        'per_page=10'
    ];
    var endpoint = '/api/v1/notifications?source=user_stories?' + options.join('&');

    ui = new AppUi({
        buttonName: BUTTON_NAME,
        sortOrder: 8,
        onOpened: gotoOpened,
        home: GOTO_QML_SOURCE,
        notificationPollEndpoint: endpoint,
        notificationPollTimeoutMs: 60000,
        notificationDataProcessPage: notificationDataProcessPage,
        notificationPollCallback: notificationPollCallback,
        notificationPollStopPaginatingConditionMet: isReturnedDataEmpty,
        notificationPollCaresAboutSince: true
    });
}

startup();
}()); // END LOCAL_SCOPE
