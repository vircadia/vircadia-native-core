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
        return (type === that.currentVisibleScreenType) &&
            that.currentVisibleUrl &&
            ((that.home.indexOf(that.currentVisibleUrl) > -1) ||
            (that.additionalAppScreens.indexOf(that.currentVisibleUrl) > -1));
    };
    that.setCurrentVisibleScreenMetadata = function setCurrentVisibleScreenMetadata(type, url) {
        that.currentVisibleScreenType = type;
        that.currentVisibleUrl = url;
    };
    that.open = function open(optionalUrl, optionalInject) { // How to open the app.
        var url = optionalUrl || that.home;
        var inject = that.inject;
        if (optionalUrl && optionalInject) {
            inject = optionalInject;
        }

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
    that.messagesWaiting = function messagesWaiting(isWaiting) { // How to indicate a message light on button.
        // Note that waitingButton doesn't have to exist unless someone explicitly calls this with isWaiting true.
        that.button.editProperties({
            icon: isWaiting ? that.normalMessagesButton : that.normalButton,
            activeIcon: isWaiting ? that.activeMessagesButton : that.activeButton
        });
    };
    that.isQMLUrl = function isQMLUrl(url) {
        var type = /.qml$/.test(url) ? 'QML' : 'Web';
        return type === 'QML';
    };
    that.isCurrentlyOnQMLScreen = function isCurrentlyOnQMLScreen() {
        return that.currentVisibleScreenType === 'QML';
    };

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
        } else { // Not us.  Should we do something for type Home, Menu, and particularly Closed (meaning tablet hidden?
            that.wireEventBridge(false);
            if (that.isOpen) {
                that.buttonActive(false);
                if (that.onClosed) {
                    that.onClosed();
                }
                that.isOpen = false;
            }
        }
        console.debug(that.buttonName + " app reports: Tablet screen changed.\nNew screen type: " + type +
            "\nNew screen URL: " + url + "\nCurrent app open status: " + that.isOpen + "\n");
    };

    // Overwrite with the given properties:
    Object.keys(properties).forEach(function (key) { that[key] = properties[key]; });

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
    that.hasQmlEventBridge = false;
    that.hasHtmlEventBridge = false;
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
        } else {
            that.sendMessage = that.ignore;
        }

        if (!that.onMessage) {
            return;
        }

        // Inbound
        if (on) {
            if (isCurrentlyOnQMLScreen && !that.hasQmlEventBridge) {
                console.debug(that.buttonName, 'connecting', that.tablet.fromQml);
                that.tablet.fromQml.connect(that.onMessage);
                that.hasQmlEventBridge = true;
            } else if (!isCurrentlyOnQMLScreen && !that.hasHtmlEventBridge) {
                console.debug(that.buttonName, 'connecting', that.tablet.webEventReceived);
                that.tablet.webEventReceived.connect(that.fromHtml);
                that.hasHtmlEventBridge = true;
            }
        } else {
            if (that.hasQmlEventBridge) {
                console.debug(that.buttonName, 'disconnecting', that.tablet.fromQml);
                that.tablet.fromQml.disconnect(that.onMessage);
                that.hasQmlEventBridge = false;
            }
            if (that.hasHtmlEventBridge) {
                console.debug(that.buttonName, 'disconnecting', that.tablet.webEventReceived);
                that.tablet.webEventReceived.disconnect(that.fromHtml);
                that.hasHtmlEventBridge = false;
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
    };
    // Set up the handlers.
    that.tablet.screenChanged.connect(that.onScreenChanged);
    that.button.clicked.connect(that.onClicked);
    Script.scriptEnding.connect(that.onScriptEnding);
}
module.exports = AppUi;
