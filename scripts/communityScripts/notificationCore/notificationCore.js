//
//  notificationCore.js
//
//  Created by Fluffy Jenkins January 2020.
//  Copyright 2020 Fluffy Jenkins
//  Copyright 2020 Vircadia contributors.
//
//  For any future coders, please keep me in the loop when making changes.
//  Please tag me in any Pull Requests.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

"use strict";
var notificationList = [];

var sizeData = {30: {widthMul: 1.8, heightMul: 2.05, split: 35, size: 30}};

var DEFAULT_SIZE = 30;
var DEFAULT_OFFSET = 10;
var FLOOF_NOTIFICATION_CHANNEL = "Floof-Notif";
var MAIN_CHAT_APP_CHANNEL = "Chat";

var offset = DEFAULT_OFFSET;

init();

function init(){
    Messages.messageReceived.connect(messageReceived);
}

function messageReceived(channel, message, sender, local) {
    if (local) {
        if (channel === FLOOF_NOTIFICATION_CHANNEL) {
            var cmd = {FAILED: true};
            try {
                cmd = JSON.parse(message);
            } catch (e) {
                //
            }
            if (!cmd.FAILED) {
                if (cmd.type === "options") {
                    if (cmd.offset) {
                        notificationCore.setOffset(cmd.offset);
                    }
                } else {
                    notificationCore.add(cmd.text, cmd.sender, cmd.colour);
                }
            }
        }
    }
}

var notificationCore = {
    setOffset: function (offsetHeight) {
        if (offsetHeight === -1) {
            offset = DEFAULT_OFFSET;
        } else {
            offset = offsetHeight;
        }
    },
    add: function (text, sender, colour) {
        sender = sender ? sender : "NoName";
        colour = colour ? colour : {};
        colour.text = colour.text ? colour.text : {red: 255, green: 255, blue: 255};
        colour.bg = colour.bg ? colour.bg : {red: 10, green: 10, blue: 10};
        var lines = text.split("\n");
        for (var i = lines.length - 1; i >= 0; i--) {
            if (i === 0) {
                notif("[" + time() + "] " + sender + ": " + lines[i], colour);
            } else {
                notif(lines[i], colour);
            }
        }
    }
};

function time() {
    var d = new Date();
    var h = (d.getHours()).toString();
    var m = (d.getMinutes()).toString();
    var s = (d.getSeconds()).toString();
    var h2 = ("0" + h).slice(-2);
    var m2 = ("0" + m).slice(-2);
    var s2 = ("0" + s).slice(-2);
    s2 += (d.getMilliseconds() / 1000).toFixed(2).slice(1);
    return h2 + ":" + m2 + ":" + s2;
}

function findIndex(id) {
    var index = -1;
    notificationList.forEach(function (noti, i) {
        if (noti.id === id) {
            index = i;
        }
    });
    return index;
}

function cleanUp() {
    try {
        Messages.messageReceived.disconnect(messageReceived);
    } catch (e) {
        // empty
    }
    notificationList.forEach(function (noti) {
        Overlays.deleteOverlay(noti.id);
    });
}

function notif(text, colour) {

    var noti = {
        text: " " + text + " ",
        time: 10 * 1000,
        timeout: null,
        timer: null,
        fade: null,
        colour: colour,
        alpha: {text: 1, bg: 0.9}
    };
    noti.id = Overlays.addOverlay("text", {
        text: '',
        font: {size: sizeData[DEFAULT_SIZE].size},
        x: 0,
        y: Window.innerHeight,
        color: colour.text,
        backgroundColor: colour.bg,
        backgroundAlpha: noti.alpha.bg,
        alpha: noti.alpha.text
    });

    var ts = Overlays.textSize(noti.id, noti.text);
    ts.height *= sizeData[DEFAULT_SIZE].heightMul;
    ts.width *= sizeData[DEFAULT_SIZE].widthMul;
    ts.y = Window.innerHeight - (sizeData[DEFAULT_SIZE].split * (notificationList.length)) - offset;
    ts.text = noti.text;
    Overlays.editOverlay(noti.id, ts);

    noti.update = function () {
        var i = notificationList.length - findIndex(noti.id);
        Overlays.editOverlay(noti.id, {
            y: Window.innerHeight - (sizeData[DEFAULT_SIZE].split * (i)) - offset
        });
    };

    noti.startFade = function () {
        noti.fade = Script.setInterval(noti.fadeOut, 50);
    };

    noti.fadeOut = function () {
        noti.alpha.text -= 0.1;
        noti.alpha.bg -= 0.1;
        Overlays.editOverlay(noti.id, {
            alpha: noti.alpha.text,
            backgroundAlpha: noti.alpha.bg
        });

        if (Math.max(noti.alpha.text, noti.alpha.bg) <= 0) {
            Script.clearInterval(noti.fade);
            noti.end();
        }
    };

    noti.end = function () {
        Script.clearInterval(noti.timer);
        Script.clearTimeout(noti.timeout);
        Overlays.deleteOverlay(noti.id);
        notificationList.splice(findIndex(noti.id), 1);
    };

    noti.timer = Script.setInterval(noti.update, 10);
    noti.timeout = Script.setTimeout(noti.startFade, noti.time);

    notificationList.push(noti);
}

Controller.mousePressEvent.connect(function (event) {
    // Overlays.getOverlayAtPoint applies only to 2D overlays.
    var overlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
    if (overlay) {
        for (var i = 0; i < notificationList.length; i++) {
            if (overlay === notificationList[i].id) {
                Overlays.deleteOverlay(notificationList[i].id)
                Messages.sendLocalMessage(MAIN_CHAT_APP_CHANNEL, JSON.stringify({
                    type: "ShowChatWindow",
                }));
            }
        }
    }
});

Script.scriptEnding.connect(cleanUp);
