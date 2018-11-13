"use strict";
/* global Tablet, Script */
//
//  libraries/appUi.js
//
//  Created by Howard Stearns on 3/20/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function AppUi(properties) {
    var request = Script.require('request').request;
    /* Example development order:
       1. var AppUi = Script.require('appUi');
       2. Put appname-i.svg, appname-a.svg in graphicsDirectory (where non-default graphicsDirectory can be added in #3).
       3. ui = new AppUi({buttonName: "APPNAME", home: "qml-or-html-path"});
          (And if converting an existing app,
            define var tablet = ui.tablet, button = ui.button; as needed.
            remove button.clicked.[dis]connect and tablet.remove(button).)
       4. Define onOpened and onClosed behavior in #3, if any.
          (And if converting an existing app, remove screenChanged.[dis]connect.)
       5. Define onMessage and sendMessage in #3, if any. onMessage is wired/unwired on open/close. If you
          want a handler to be "always on", connect it yourself at script startup.
          (And if converting an existing app, remove code that [un]wires that message handling such as
          fromQml/sendToQml or webEventReceived/emitScriptEvent.)
       6. (If converting an existing app, cleanup stuff that is no longer necessary, like references to button, tablet,
           and use isOpen, open(), and close() as needed.)
       7. lint!
     */
    var that = this;
    function defaultButton(name, suffix) {
        var base = that[name] || (that.buttonPrefix + suffix);
        that[name] = (base.indexOf('/') >= 0) ? base : (that.graphicsDirectory + base); // poor man's merge
    }

    // Defaults:
    that.tabletName = "com.highfidelity.interface.tablet.system";
    that.inject = "";
    that.graphicsDirectory = "icons/tablet-icons/"; // Where to look for button svgs. See below.
    that.additionalAppScreens = [];
    that.checkIsOpen = function checkIsOpen(type, tabletUrl) { // Are we active? Value used to set isOpen.
        // Actual url may have prefix or suffix.
        return that.currentVisibleUrl &&
            ((that.home.indexOf(that.currentVisibleUrl) > -1) ||
            (that.additionalAppScreens.indexOf(that.currentVisibleUrl) > -1));
    };
    that.setCurrentVisibleScreenMetadata = function setCurrentVisibleScreenMetadata(type, url) {
        that.currentVisibleScreenType = type;
        that.currentVisibleUrl = url;
    };
    that.open = function open(optionalUrl, optionalInject) { // How to open the app.
        var url = optionalUrl || that.home;
        var inject = optionalInject || that.inject;

        if (that.isQMLUrl(url)) {
            that.tablet.loadQMLSource(url);
        } else {
            that.tablet.gotoWebScreen(url, inject);
        }
    };
    // Opens some app on top of the current app (on desktop, opens new window)
    that.openNewAppOnTop = function openNewAppOnTop(url, optionalInject) {
        var inject = optionalInject || "";
        if (that.isQMLUrl(url)) {
            that.tablet.loadQMLOnTop(url);
        } else {
            that.tablet.loadWebScreenOnTop(url, inject);
        }
    };
    that.close = function close() { // How to close the app.
        that.currentVisibleUrl = "";
        // for toolbar-mode: go back to home screen, this will close the window.
        that.tablet.gotoHomeScreen();
    };
    that.buttonActive = function buttonActive(isActive) { // How to make the button active (white).
        that.button.editProperties({isActive: isActive});
    };
    that.isQMLUrl = function isQMLUrl(url) {
        var type = /.qml$/.test(url) ? 'QML' : 'Web';
        return type === 'QML';
    };
    that.isCurrentlyOnQMLScreen = function isCurrentlyOnQMLScreen() {
        return that.currentVisibleScreenType === 'QML';
    };

    //
    // START Notification Handling Defaults
    //
    that.messagesWaiting = function messagesWaiting(isWaiting) { // How to indicate a message light on button.
        // Note that waitingButton doesn't have to exist unless someone explicitly calls this with isWaiting true.
        that.button.editProperties({
            icon: isWaiting ? that.normalMessagesButton : that.normalButton,
            activeIcon: isWaiting ? that.activeMessagesButton : that.activeButton
        });
    };
    that.notificationPollTimeout = [false];
    that.notificationPollTimeoutMs = [60000];
    that.notificationPollEndpoint = [false];
    that.notificationPollStopPaginatingConditionMet = [false];
    that.notificationDataProcessPage = function (data) {
        return data;
    };
    that.notificationPollCallback = [that.ignore];
    that.notificationPollCaresAboutSince = [false];
    that.notificationInitialCallbackMade = [false];
    that.notificationDisplayBanner = function (message) {
        if (!that.isOpen) {
            Window.displayAnnouncement(message);
        }
    };
    //
    // END Notification Handling Defaults
    //

    // Handlers
    that.onScreenChanged = function onScreenChanged(type, url) {
        // Set isOpen, wireEventBridge, set buttonActive as appropriate,
        // and finally call onOpened() or onClosed() IFF defined.
        that.setCurrentVisibleScreenMetadata(type, url);

        if (that.checkIsOpen(type, url)) {
            that.wireEventBridge(true);
            if (!that.isOpen) {
                that.buttonActive(true);
                if (that.onOpened) {
                    that.onOpened();
                }
                that.isOpen = true;
            }
        } else {
            // A different screen is now visible, or the tablet has been closed.
            // Tablet visibility is controlled separately by `tabletShownChanged()`
            that.wireEventBridge(false);
            if (that.isOpen) {
                that.buttonActive(false);
                if (that.onClosed) {
                    that.onClosed();
                }
                that.isOpen = false;
            }
        }
    };

    // Overwrite with the given properties:
    Object.keys(properties).forEach(function (key) {
        that[key] = properties[key];
    });

    //
    // START Notification Handling
    //

    var currentDataPageToRetrieve = [];
    var concatenatedServerResponse = [];
    for (var i = 0; i < that.notificationPollEndpoint.length; i++) {
        currentDataPageToRetrieve[i] = 1;
        concatenatedServerResponse[i] = new Array();
    }

    var MAX_LOG_LENGTH_CHARACTERS = 300;
    function requestCallback(error, response, optionalParams) {
        var indexOfRequest = optionalParams.indexOfRequest;
        var urlOfRequest = optionalParams.urlOfRequest;

        if (error || (response.status !== 'success')) {
            print("Error: unable to complete request from URL. Error:", error || response.status);
            startNotificationTimer(indexOfRequest);
            return;
        }

        if (!that.notificationPollStopPaginatingConditionMet[indexOfRequest] ||
            that.notificationPollStopPaginatingConditionMet[indexOfRequest](response)) {
            startNotificationTimer(indexOfRequest);

            var notificationData;
            if (concatenatedServerResponse[indexOfRequest].length) {
                notificationData = concatenatedServerResponse[indexOfRequest];
            } else {
                notificationData = that.notificationDataProcessPage[indexOfRequest](response);
            }
            console.debug(that.buttonName,
                'truncated notification data for processing:',
                JSON.stringify(notificationData).substring(0, MAX_LOG_LENGTH_CHARACTERS));
            that.notificationPollCallback[indexOfRequest](notificationData);
            that.notificationInitialCallbackMade[indexOfRequest] = true;
            currentDataPageToRetrieve[indexOfRequest] = 1;
            concatenatedServerResponse[indexOfRequest] = new Array();
        } else {
            concatenatedServerResponse[indexOfRequest] =
                concatenatedServerResponse[indexOfRequest].concat(that.notificationDataProcessPage[indexOfRequest](response));
            currentDataPageToRetrieve[indexOfRequest]++;
            request({
                json: true,
                uri: (urlOfRequest + "&page=" + currentDataPageToRetrieve[indexOfRequest])
            }, requestCallback, optionalParams);
        }
    }


    var METAVERSE_BASE = Account.metaverseServerURL;
    var MS_IN_SEC = 1000;
    that.notificationPoll = function (i) {
        if (!that.notificationPollEndpoint[i]) {
            return;
        }

        // User is "appearing offline" or is not logged in
        if (GlobalServices.findableBy === "none" || Account.username === "Unknown user") {
            // The notification polling will restart when the user changes their availability
            // or when they log in, so it's not necessary to restart a timer here.
            console.debug(that.buttonName + " Notifications: User is appearing offline or not logged in. " +
                that.buttonName + " will poll for notifications when user logs in and has their availability " +
                "set to not appear offline.");
            return;
        }

        var url = METAVERSE_BASE + that.notificationPollEndpoint[i];

        var settingsKey = "notifications/" + that.notificationPollEndpoint[i] + "/lastPoll";
        var currentTimestamp = new Date().getTime();
        var lastPollTimestamp = Settings.getValue(settingsKey, currentTimestamp);
        if (that.notificationPollCaresAboutSince[i]) {
            url = url + "&since=" + lastPollTimestamp / MS_IN_SEC;
        }
        Settings.setValue(settingsKey, currentTimestamp);

        request({
            json: true,
            uri: url
        },
        requestCallback,
        {
            indexOfRequest: i,
            urlOfRequest: url
        });
    };

    // This won't do anything if there isn't a notification endpoint set
    for (i = 0; i < that.notificationPollEndpoint.length; i++) {
        that.notificationPoll(i);
    }

    function startNotificationTimer(indexOfRequest) {
        that.notificationPollTimeout[indexOfRequest] = Script.setTimeout(function () {
            that.notificationPoll(indexOfRequest);
        }, that.notificationPollTimeoutMs[indexOfRequest]);
    }

    function restartNotificationPoll() {
        for (var j = 0; j < that.notificationPollEndpoint.length; j++) {
            that.notificationInitialCallbackMade[j] = false;
            if (that.notificationPollTimeout[j]) {
                Script.clearTimeout(that.notificationPollTimeout[j]);
                that.notificationPollTimeout[j] = false;
            }
            that.notificationPoll(j);
        }
    }
    //
    // END Notification Handling
    //

    // Properties:
    that.tablet = Tablet.getTablet(that.tabletName);
    // Must be after we gather properties.
    that.buttonPrefix = that.buttonPrefix || that.buttonName.toLowerCase() + "-";
    defaultButton('normalButton', 'i.svg');
    defaultButton('activeButton', 'a.svg');
    defaultButton('normalMessagesButton', 'i-msg.svg');
    defaultButton('activeMessagesButton', 'a-msg.svg');
    var buttonOptions = {
        icon: that.normalButton,
        activeIcon: that.activeButton,
        text: that.buttonName
    };
    // `TabletScriptingInterface` looks for the presence of a `sortOrder` key.
    // What it SHOULD do is look to see if the value inside that key is defined.
    // To get around the current code, we do this instead.
    if (that.sortOrder) {
        buttonOptions.sortOrder = that.sortOrder;
    }
    that.button = that.tablet.addButton(buttonOptions);
    that.ignore = function ignore() { };
    that.hasOutboundEventBridge = false;
    that.hasInboundQmlEventBridge = false;
    that.hasInboundHtmlEventBridge = false;
    // HTML event bridge uses strings, not objects. Here we abstract over that.
    // (Although injected javascript still has to use JSON.stringify/JSON.parse.)
    that.sendToHtml = function (messageObject) {
        that.tablet.emitScriptEvent(JSON.stringify(messageObject));
    };
    that.fromHtml = function (messageString) {
        var parsedMessage = JSON.parse(messageString);
        parsedMessage.messageSrc = "HTML";
        that.onMessage(parsedMessage);
    };
    that.sendMessage = that.ignore;
    that.wireEventBridge = function wireEventBridge(on) {
        // Uniquivocally sets that.sendMessage(messageObject) to do the right thing.
        // Sets has*EventBridge and wires onMessage to the proper event bridge as appropriate, IFF onMessage defined.
        var isCurrentlyOnQMLScreen = that.isCurrentlyOnQMLScreen();
        // Outbound (always, regardless of whether there is an inbound handler).
        if (on) {
            that.sendMessage = isCurrentlyOnQMLScreen ? that.tablet.sendToQml : that.sendToHtml;
            that.hasOutboundEventBridge = true;
        } else {
            that.sendMessage = that.ignore;
            that.hasOutboundEventBridge = false;
        }

        if (!that.onMessage) {
            return;
        }

        // Inbound
        if (on) {
            if (isCurrentlyOnQMLScreen && !that.hasInboundQmlEventBridge) {
                console.debug(that.buttonName, 'connecting', that.tablet.fromQml);
                that.tablet.fromQml.connect(that.onMessage);
                that.hasInboundQmlEventBridge = true;
            } else if (!isCurrentlyOnQMLScreen && !that.hasInboundHtmlEventBridge) {
                console.debug(that.buttonName, 'connecting', that.tablet.webEventReceived);
                that.tablet.webEventReceived.connect(that.fromHtml);
                that.hasInboundHtmlEventBridge = true;
            }
        } else {
            if (that.hasInboundQmlEventBridge) {
                console.debug(that.buttonName, 'disconnecting', that.tablet.fromQml);
                that.tablet.fromQml.disconnect(that.onMessage);
                that.hasInboundQmlEventBridge = false;
            }
            if (that.hasInboundHtmlEventBridge) {
                console.debug(that.buttonName, 'disconnecting', that.tablet.webEventReceived);
                that.tablet.webEventReceived.disconnect(that.fromHtml);
                that.hasInboundHtmlEventBridge = false;
            }
        }
    };
    that.isOpen = false;
    // To facilitate incremental development, only wire onClicked to do something when "home" is defined in properties.
    that.onClicked = that.home
        ? function onClicked() {
            // Call open() or close(), and reset type based on current home property.
            if (that.isOpen) {
                that.close();
            } else {
                that.open();
            }
        } : that.ignore;
    that.onScriptEnding = function onScriptEnding() {
        // Close if necessary, clean up any remaining handlers, and remove the button.
        GlobalServices.myUsernameChanged.disconnect(restartNotificationPoll);
        GlobalServices.findableByChanged.disconnect(restartNotificationPoll);
        if (that.isOpen) {
            that.close();
        }
        that.tablet.screenChanged.disconnect(that.onScreenChanged);
        if (that.button) {
            if (that.onClicked) {
                that.button.clicked.disconnect(that.onClicked);
            }
            that.tablet.removeButton(that.button);
        }
        for (var i = 0; i < that.notificationPollTimeout.length; i++) {
            if (that.notificationPollTimeout[i]) {
                Script.clearInterval(that.notificationPollTimeout[i]);
                that.notificationPollTimeout[i] = false;
            }
        }
    };
    // Set up the handlers.
    that.tablet.screenChanged.connect(that.onScreenChanged);
    that.button.clicked.connect(that.onClicked);
    Script.scriptEnding.connect(that.onScriptEnding);
    GlobalServices.findableByChanged.connect(restartNotificationPoll);
    GlobalServices.myUsernameChanged.connect(restartNotificationPoll);
    if (that.buttonName === Settings.getValue("startUpApp")) {
        Settings.setValue("startUpApp", "");
        Script.setTimeout(function () {
            that.open();
        }, 1000);
    }
}
module.exports = AppUi;
