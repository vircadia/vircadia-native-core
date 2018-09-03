"use strict";
/*global Tablet, Script*/
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
        that[name] = (base.indexOf('/') >= 0) ? base : (that.graphicsDirectory + base); //poor man's merge
    }

    // Defaults:
    that.tabletName = "com.highfidelity.interface.tablet.system";
    that.inject = "";
    that.graphicsDirectory = "icons/tablet-icons/"; // Where to look for button svgs. See below.
    that.checkIsOpen = function checkIsOpen(type, tabletUrl) { // Are we active? Value used to set isOpen.
        return (type === that.type) && that.currentUrl && (tabletUrl.indexOf(that.currentUrl) >= 0); // Actual url may have prefix or suffix.
    };
    that.setCurrentData = function setCurrentData(url) {
        that.currentUrl = url;
        that.type = /.qml$/.test(url) ? 'QML' : 'Web';
    }
    that.open = function open(optionalUrl) { // How to open the app.
        var url = optionalUrl || that.home;
        that.setCurrentData(url);
        if (that.isQML()) {
            that.tablet.loadQMLSource(url);
        } else {
            that.tablet.gotoWebScreen(url, that.inject);
        }
    };
    that.close = function close() { // How to close the app.
        that.currentUrl = "";
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
    that.isQML = function isQML() { // We set type property in onClick.
        return that.type === 'QML';
    };
    that.eventSignal = function eventSignal() { // What signal to hook onMessage to.
        return that.isQML() ? that.tablet.fromQml : that.tablet.webEventReceived;
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

    // Handlers
    that.onScreenChanged = function onScreenChanged(type, url) {
        // Set isOpen, wireEventBridge, set buttonActive as appropriate,
        // and finally call onOpened() or onClosed() IFF defined.
        console.debug(that.buttonName, 'onScreenChanged', type, url, that.isOpen);
        if (that.checkIsOpen(type, url)) {
            if (!that.isOpen) {
                that.wireEventBridge(true);
                that.buttonActive(true);
                if (that.onOpened) {
                    that.onOpened();
                }
                that.isOpen = true;
            }

        } else { // Not us.  Should we do something for type Home, Menu, and particularly Closed (meaning tablet hidden?
            if (that.isOpen) {
                that.wireEventBridge(false);
                that.buttonActive(false);
                if (that.onClosed) {
                    that.onClosed();
                }
                that.isOpen = false;
            }
        }
    };
    that.hasEventBridge = false;
    // HTML event bridge uses strings, not objects. Here we abstract over that.
    // (Although injected javascript still has to use JSON.stringify/JSON.parse.)
    that.sendToHtml = function (messageObject) { that.tablet.emitScriptEvent(JSON.stringify(messageObject)); };
    that.fromHtml = function (messageString) { that.onMessage(JSON.parse(messageString)); };
    that.sendMessage = that.ignore;
    that.wireEventBridge = function wireEventBridge(on) {
        // Uniquivocally sets that.sendMessage(messageObject) to do the right thing.
        // Sets hasEventBridge and wires onMessage to eventSignal as appropriate, IFF onMessage defined.
        var handler, isQml = that.isQML();
        // Outbound (always, regardless of whether there is an inbound handler).
        if (on) {
            that.sendMessage = isQml ? that.tablet.sendToQml : that.sendToHtml;
        } else {
            that.sendMessage = that.ignore;
        }

        if (!that.onMessage) { return; }

        // Inbound
        handler = isQml ? that.onMessage : that.fromHtml;
        if (on) {
            if (!that.hasEventBridge) {
                console.debug(that.buttonName, 'connecting', that.eventSignal());
                that.eventSignal().connect(handler);
                that.hasEventBridge = true;
            }
        } else {
            if (that.hasEventBridge) {
                console.debug(that.buttonName, 'disconnecting', that.eventSignal());
                that.eventSignal().disconnect(handler);
                that.hasEventBridge = false;
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
