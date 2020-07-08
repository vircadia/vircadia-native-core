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
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () { // BEGIN LOCAL_SCOPE
var AppUi = Script.require('appUi');

function gotoOpened() {
    shouldShowDot = false;
    ui.messagesWaiting(shouldShowDot);
}

function notificationDataProcessPage(data) {
    return data.user_stories;
}

var shouldShowDot = false;
var pingPong = false;
var storedAnnouncements = {};
var storedFeaturedStories = {};
var message;
function notificationPollCallback(userStoriesArray) {
    //
    // START logic for keeping track of new info
    //
    pingPong = !pingPong;
    var totalNewStories = 0;
    var shouldNotifyIndividually = !ui.isOpen && ui.notificationInitialCallbackMade[0];
    userStoriesArray.forEach(function (story) {
        if (story.audience !== "for_connections" &&
            story.audience !== "for_feed") {
            return;
        }

        var stored = storedAnnouncements[story.id] || storedFeaturedStories[story.id];
        var storedOrNew = stored || story;
        storedOrNew.pingPong = pingPong;
        if (stored) {
            return;
        }

        totalNewStories++;

        if (story.audience === "for_connections") {
            storedAnnouncements[story.id] = story;

            if (shouldNotifyIndividually) {
                message = story.username + " says something is happening in " +
                    story.place_name + ". Open GOTO to join them.";
                ui.notificationDisplayBanner(message);
            }
        } else if (story.audience === "for_feed") {
            storedFeaturedStories[story.id] = story;

            if (shouldNotifyIndividually) {
                message = story.username + " invites you to an event in " +
                    story.place_name + ". Open GOTO to join them.";
                ui.notificationDisplayBanner(message);
            }
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

    var totalStories = Object.keys(storedAnnouncements).length +
        Object.keys(storedFeaturedStories).length;
    shouldShowDot = totalNewStories > 0 || (totalStories > 0 && shouldShowDot);
    ui.messagesWaiting(shouldShowDot && !ui.isOpen);

    if (totalStories > 0 && !ui.isOpen && !ui.notificationInitialCallbackMade[0]) {
        message = "There " + (totalStories === 1 ? "is " : "are ") + totalStories + " event" +
            (totalStories === 1 ? "" : "s") + " to know about. " +
            "Open GOTO to see " + (totalStories === 1 ? "it" : "them") + ".";
        ui.notificationDisplayBanner(message);
    }
}

function isReturnedDataEmpty(data) {
    var storiesArray = data.user_stories;
    return storiesArray.length === 0;
}

var ui;
var GOTO_QML_SOURCE = "hifi/tablet/TabletAddressDialog.qml";
var BUTTON_NAME = "OLD GOTO";
function startup() {
    var options = [
        'include_actions=announcement',
        'restriction=open,hifi',
        'require_online=true',
        'protocol=' + encodeURIComponent(Window.protocolSignature()),
        'per_page=10'
    ];
    var endpoint = '/api/v1/user_stories?' + options.join('&');

    ui = new AppUi({
        buttonName: BUTTON_NAME,
        normalButton: "icons/tablet-icons/goto-i.svg",
        activeButton: "icons/tablet-icons/goto-a.svg",
        sortOrder: 8,
        onOpened: gotoOpened,
        home: GOTO_QML_SOURCE,
        notificationPollEndpoint: [endpoint],
        notificationPollTimeoutMs: [60000],
        notificationDataProcessPage: [notificationDataProcessPage],
        notificationPollCallback: [notificationPollCallback],
        notificationPollStopPaginatingConditionMet: [isReturnedDataEmpty],
        notificationPollCaresAboutSince: [false]
    });
}

startup();
}()); // END LOCAL_SCOPE
