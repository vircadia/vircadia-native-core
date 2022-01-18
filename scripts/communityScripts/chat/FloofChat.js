/* globals OverlayWindow */
"use strict";

//
//  FloofChat.js
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

var ROOT = Script.resolvePath('').split("FloofChat.js")[0];
var H_KEY = 72;
var ENTER_KEY = 16777220;
var ESC_KEY = 16777216;
var CONTROL_KEY = 67108864;
var SHIFT_KEY = 33554432;
var FLOOF_CHAT_CHANNEL = "Chat";
var FLOOF_NOTIFICATION_CHANNEL = "Floof-Notif";
var SYSTEM_NOTIFICATION_CHANNEL = "System-Notifications";

var MAIN_CHAT_WINDOW_HEIGHT = 450;
var MAIN_CHAT_WINDOW_WIDTH = 750;

var CHAT_BAR_HISTORY_LIMIT = 256;
var CHAT_HISTORY_LIMIT = 500;

Script.scriptEnding.connect(function () {
    shutdown();
});

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var button = tablet.addButton({
    icon: ROOT + "chat.png",
    text: "CHAT"
});

Script.scriptEnding.connect(function () { // So if anything errors out the tablet/toolbar button gets removed!
    tablet.removeButton(button);
});

var appUUID = Uuid.generate();

var chatBar;
var chatHistory;
var chatBarHistoryLimit = Settings.getValue(settingsRoot + "/chatBarHistoryLimit", CHAT_BAR_HISTORY_LIMIT);
var chatHistoryLimit = Settings.getValue(settingsRoot + "/chatHistoryLimit", CHAT_HISTORY_LIMIT);
var chatBarHistory = Settings.getValue(settingsRoot + "/chatBarHistory", ["Meow :3"]);
var historyLog = [];

var visible = false;
var historyVisible = false;
var settingsRoot = "FloofChat";

var vircadiaGotoUrl = "https://metaverse.vircadia.com/interim/d-goto/app/goto.json";
var gotoJSONUrl = Settings.getValue(settingsRoot + "/gotoJSONUrl", vircadiaGotoUrl);

var muted = Settings.getValue(settingsRoot + "/muted", {"Local": false, "Domain": true, "Grid": true});
var mutedAudio = Settings.getValue(settingsRoot + "/mutedAudio", {"Local": false, "Domain": true, "Grid": true});
var notificationSound = SoundCache.getSound(Script.resolvePath("resources/bubblepop.wav"));

var ws;
var wsReady = false;
var WEB_SOCKET_URL = "ws://chat.vircadia.com:8880";  // WebSocket for Grid chat.
var shutdownBool = false;

var defaultColour = {red: 255, green: 255, blue: 255};
var colours = {};
colours["localChatColour"] = Settings.getValue(settingsRoot + "/localChatColour", defaultColour);
colours["domainChatColour"] = Settings.getValue(settingsRoot + "/domainChatColour", defaultColour);
colours["gridChatColour"] = Settings.getValue(settingsRoot + "/gridChatColour", defaultColour);

init();

function init() {
    Messages.subscribe(FLOOF_CHAT_CHANNEL);
    Messages.subscribe(SYSTEM_NOTIFICATION_CHANNEL);
    historyLog = [];
    try {
        historyLog = JSON.parse(Settings.getValue(settingsRoot + "/HistoryLog", "[]"));
    } catch (e) {
        //
    }

    setupHistoryWindow(false);

    chatBar = new OverlayWindow({
        source: ROOT + "FloofChat.qml?" + Date.now(),
        width: 360,
        height: 180
    });

    button.clicked.connect(toggleMainChatWindow);
    chatBar.fromQml.connect(fromQml);
    chatBar.sendToQml(JSON.stringify({visible: false, history: chatBarHistory}));
    Controller.keyPressEvent.connect(keyPressEvent);
    Messages.messageReceived.connect(messageReceived);
    AvatarManager.avatarAddedEvent.connect(avatarJoinsDomain);
    AvatarManager.avatarRemovedEvent.connect(avatarLeavesDomain);

    connectWebSocket();
}

function connectWebSocket(timeout) {
    ws = new WebSocket(WEB_SOCKET_URL);
    ws.onmessage = function incoming(_data) {
        var message = _data.data;
        var cmd = {FAILED: true};
        try {
            cmd = JSON.parse(message);
        } catch (e) {
            //
        }
        if (!cmd.FAILED) {
            addToLog(cmd.message, cmd.displayName, cmd.colour, cmd.channel);
            
            if (!mutedAudio["Grid"] && MyAvatar.sessionDisplayName !== cmd.displayName) {
                playNotificationSound();
            }
            
            if (!muted["Grid"]) {
                Messages.sendLocalMessage(FLOOF_NOTIFICATION_CHANNEL, JSON.stringify({
                    sender: "(G) " + cmd.displayName,
                    text: replaceFormatting(cmd.message),
                    colour: {text: cmd.colour}
                }));
            }
        }
    };

    ws.onopen = function open() {
        wsReady = true;
    };

    ws.onclose = function close() {
        wsReady = false;
        console.log('disconnected');

        timeout = timeout | 0;
        if (!shutdownBool) {
            if (timeout > (30 * 1000)) {
                timeout = 30 * 1000;
            } else if (timeout < (30 * 1000)) {
                timeout += 1000;
            }
            Script.setTimeout(function () {
                connectWebSocket(timeout);
            }, timeout);
        } else {
            wsReady = -1;
        }
    };
}


function sendWS(msg, timeout) {
    if (wsReady === true) {
        ws.send(JSON.stringify(msg));
    } else {
        timeout = timeout | 0;
        if (!shutdownBool) {
            if (timeout > (30 * 1000)) {
                timeout = 30 * 1000;
            } else if (timeout < (30 * 1000)) {
                timeout += 1000;
            }
            Script.setTimeout(function () {
                if (wsReady === -1) {
                    connectWebSocket();
                }
                sendWS(msg, timeout);
            }, timeout);
        }
    }
}

function setupHistoryWindow() {
    chatHistory = new OverlayWebWindow({
        title: 'Chat',
        source: ROOT + "FloofChat.html?appUUID=" + appUUID + "&" + Date.now(),
        width: MAIN_CHAT_WINDOW_WIDTH,
        height: MAIN_CHAT_WINDOW_HEIGHT,
        visible: false
    });
    chatHistory.setPosition({x: 0, y: Window.innerHeight - MAIN_CHAT_WINDOW_HEIGHT});
    chatHistory.webEventReceived.connect(onWebEventReceived);
    chatHistory.closed.connect(toggleMainChatWindow);
}

function emitScriptEvent(obj) {
    obj.appUUID = appUUID;
    tablet.emitScriptEvent(JSON.stringify(obj));
}

function toggleMainChatWindow() {
    historyVisible = !historyVisible;
    button.editProperties({isActive: historyVisible});
    chatHistory.visible = historyVisible;
}

function chatColour(tab) {
    if (tab === "Local") {
        return colours["localChatColour"];
    } else if (tab === "Domain") {
        return colours["domainChatColour"];
    } else if (tab === "Grid") {
        return colours["gridChatColour"];
    } else {
        return defaultColour;
    }
}

var chatBarChannel = "Local";

function go2(msg) {
    var dest = false;
    var domainsList = [];
    try {
        domainsList = Script.require(gotoJSONUrl + "?" + Date.now());
    } catch (e) {
        //
    }
    domainsList.forEach(function (domain) {
        if (domain["Domain Name"].toLowerCase().indexOf(msg.toLowerCase()) !== -1 && !dest) {
            dest = {"name": domain["Domain Name"], "url": domain["Visit"]};
        }
    });
    return dest;
}

function gotoConfirm(url, name, confirm) {
    confirm = confirm === undefined ? true : confirm;
    name = name === undefined ? url : name;
    var result = true;
    if (confirm) {
        result = Window.confirm("Do you want to go to '" + ((name.indexOf("/") !== -1) ? url.split("/")[2] : name) + "'?");
    }
    if (result) {
        location = url;
    }
}

function processChat(cmd) {

    function commandResult() {
        msg = "";
        setVisible(false);
    }

    var msg = cmd.message;
    if (msg.indexOf("/") === 0) {
        msg = msg.substring(1);
        var commandList = msg.split(" ");
        var tempList = commandList.concat();
        tempList.shift();
        var restOfMsg = tempList.join(" ");

        msg = "/" + msg;
        var cmd1 = commandList[0].toLowerCase();
        if (cmd1 === "l") {
            chatBarChannel = "Local";
            commandResult();
        }
        if (cmd1 === "d") {
            chatBarChannel = "Domain";
            commandResult();
        }
        if (cmd1 === "g") {
            chatBarChannel = "Grid";
            commandResult();
        }

        if (cmd1 === "goto" || cmd1 === "gotos") {
            var result = go2(restOfMsg);
            if (result) {
                gotoConfirm(result.url, result.name, (cmd1 === "goto"));
            } else {
                gotoConfirm(commandList[1], undefined, (cmd1 === "goto"));
            }
            commandResult();
        }

        if (cmd1 === "me") {
            msg = cmd.avatarName + " " + restOfMsg;
            cmd.avatarName = "";
        }
    }
    cmd.message = msg;
    return cmd;
}

function onWebEventReceived(event) {
    event = JSON.parse(event);
    if (event.type === "ready") {
        chatHistory.emitScriptEvent(JSON.stringify({type: "MSG", data: historyLog}));
        chatHistory.emitScriptEvent(JSON.stringify({type: "CMD", cmd: "MUTED", muted: muted}));
        chatHistory.emitScriptEvent(JSON.stringify({type: "CMD", cmd: "MUTEDAUDIO", muted: mutedAudio}));
    }
    if (event.type === "CMD") {
        if (event.cmd === "MUTED") {
            muted = event.muted;
            Settings.setValue(settingsRoot + "/muted", muted);
        }
        if (event.cmd === "MUTEDAUDIO") {
            mutedAudio = event.muted;
            Settings.setValue(settingsRoot + "/mutedAudio", mutedAudio);
        }
        if (event.cmd === "COLOUR") {
            Settings.setValue(settingsRoot + "/" + event.colourType + "Colour", event.colour);
            colours[event.colourType] = event.colour;
        }
        if (event.cmd === "REDOCK") {
            chatHistory.setPosition({x: 0, y: Window.innerHeight - MAIN_CHAT_WINDOW_HEIGHT});
        }
        if (event.cmd === "GOTO") {
            gotoConfirm(event.url);
        }
        if (event.cmd === "URL") {
            Window.openWebBrowser(event.url);
        }
        if (event.cmd === "EXTERNALURL") {
            Window.openUrl(event.url);
        }
        if (event.cmd === "COPY") {
            Window.copyToClipboard(event.url);
        }
    }
    if (event.type === "WEBMSG") {
        event.avatarName = MyAvatar.sessionDisplayName;
        event = processChat(event);
        if (event.message === "") return;
        sendWS({
            uuid: "",
            type: "WebChat",
            channel: event.tab,
            colour: chatColour(event.tab),
            message: event.message,
            displayName: event.avatarName
        });
    }
    if (event.type === "MSG") {
        event.avatarName = MyAvatar.sessionDisplayName;
        event = processChat(event);
        if (event.message === "") return;
        Messages.sendMessage("Chat", JSON.stringify({
            type: "TransmitChatMessage",
            position: MyAvatar.position,
            channel: event.tab,
            colour: chatColour(event.tab),
            message: event.message,
            displayName: event.avatarName
        }));
        setVisible(false);
    }
}

function playNotificationSound() {
    if (notificationSound.downloaded) {
        var injectorOptions = {
            localOnly: true,
            position: MyAvatar.position,
            volume: 0.02
        };
        Audio.playSound(notificationSound, injectorOptions);
    }
}

function replaceFormatting(text) {
    var found = false;
    if (text.indexOf("**") !== -1) {
        var firstMatch = text.indexOf("**") + 2;
        var secondMatch = text.indexOf("**", firstMatch);
        if (firstMatch !== -1 && secondMatch !== -1) {
            found = true;
            var part1 = text.substring(0, firstMatch - 2);
            var part2 = text.substring(firstMatch, secondMatch);
            var part3 = text.substring(secondMatch + 2);
            text = part1 + "<b>" + part2 + "</b>" + part3;
        }
    } else if (text.indexOf("*") !== -1) {
        var firstMatch = text.indexOf("*") + 1;
        var secondMatch = text.indexOf("*", firstMatch);
        if (firstMatch !== -1 && secondMatch !== -1) {
            found = true;
            var part1 = text.substring(0, firstMatch - 1);
            var part2 = text.substring(firstMatch, secondMatch);
            var part3 = text.substring(secondMatch + 1);
            text = part1 + "<i>" + part2 + "</i>" + part3;
        }
    } else if (text.indexOf("__") !== -1) {
        var firstMatch = text.indexOf("__") + 2;
        var secondMatch = text.indexOf("__", firstMatch);
        if (firstMatch !== -1 && secondMatch !== -1) {
            found = true;
            var part1 = text.substring(0, firstMatch - 2);
            var part2 = text.substring(firstMatch, secondMatch);
            var part3 = text.substring(secondMatch + 2);
            text = part1 + "<u>" + part2 + "</u>" + part3;
        }
    } else if (text.indexOf("_") !== -1) {
        var firstMatch = text.indexOf("_") + 1;
        var secondMatch = text.indexOf("_", firstMatch);
        if (firstMatch !== -1 && secondMatch !== -1) {
            found = true;
            var part1 = text.substring(0, firstMatch - 1);
            var part2 = text.substring(firstMatch, secondMatch);
            var part3 = text.substring(secondMatch + 1);
            text = part1 + "<i>" + part2 + "</i>" + part3;
        }
    } else if (text.indexOf("~~") !== -1) {
        var firstMatch = text.indexOf("~~") + 2;
        var secondMatch = text.indexOf("~~", firstMatch);
        if (firstMatch !== -1 && secondMatch !== -1) {
            found = true;
            var part1 = text.substring(0, firstMatch - 2);
            var part2 = text.substring(firstMatch, secondMatch);
            var part3 = text.substring(secondMatch + 2);
            text = part1 + "<s>" + part2 + "</s>" + part3;
        }
    }
    if (found) {
        return replaceFormatting(text);
    } else {
        return text;
    }
}

function messageReceived(channel, message, sender) {
    if (channel === FLOOF_CHAT_CHANNEL) {
        var cmd = {FAILED: true};
        try {
            cmd = JSON.parse(message);
        } catch (e) {
            //
        }
        if (!cmd.FAILED) {
            if (cmd.type === "TransmitChatMessage") {
                if (!cmd.hasOwnProperty("channel")) {
                    cmd.channel = "Domain";
                }
                if (!cmd.hasOwnProperty("colour")) {
                    cmd.colour = {red: 222, green: 222, blue: 222};
                }
                if (cmd.message.indexOf("/me") === 0) {
                    cmd.message = cmd.message.replace("/me", cmd.displayName);
                    cmd.displayName = "";
                }
                if (cmd.channel === "Local") {
                    if (Vec3.withinEpsilon(MyAvatar.position, cmd.position, 20)) {
                        addToLog(cmd.message, cmd.displayName, cmd.colour, cmd.channel);
                        
                        if (!mutedAudio["Local"] && MyAvatar.sessionDisplayName !== cmd.displayName) {
                            playNotificationSound();
                        }
                        
                        if (!muted["Local"]) {
                            Messages.sendLocalMessage(FLOOF_NOTIFICATION_CHANNEL, JSON.stringify({
                                sender: "(L) " + cmd.displayName,
                                text: replaceFormatting(cmd.message),
                                colour: {text: cmd.colour}
                            }));
                        }
                    }
                } else if (cmd.channel === "Domain") {
                    addToLog(cmd.message, cmd.displayName, cmd.colour, cmd.channel);
                    
                    if (!mutedAudio["Domain"] && MyAvatar.sessionDisplayName !== cmd.displayName) {
                        playNotificationSound();
                    }
                    
                    if (!muted["Domain"]) {
                        Messages.sendLocalMessage(FLOOF_NOTIFICATION_CHANNEL, JSON.stringify({
                            sender: "(D) " + cmd.displayName,
                            text: replaceFormatting(cmd.message),
                            colour: {text: cmd.colour}
                        }));
                    }
                } else if (cmd.channel === "Grid") {
                    addToLog(cmd.message, cmd.displayName, cmd.colour, cmd.channel);
                    
                    if (!mutedAudio["Grid"] && MyAvatar.sessionDisplayName !== cmd.displayName) {
                        playNotificationSound();
                    }
                    
                    if (!muted["Grid"]) {
                        Messages.sendLocalMessage(FLOOF_NOTIFICATION_CHANNEL, JSON.stringify({
                            sender: "(G) " + cmd.displayName,
                            text: replaceFormatting(cmd.message),
                            colour: {text: cmd.colour}
                        }));
                    }
                } else {
                    addToLog(cmd.message, cmd.displayName, cmd.colour, cmd.channel);
                    
                    if (MyAvatar.sessionDisplayName !== cmd.displayName) {
                        playNotificationSound();
                    }
                    
                    Messages.sendLocalMessage(FLOOF_NOTIFICATION_CHANNEL, JSON.stringify({
                        sender: cmd.displayName,
                        text: replaceFormatting(cmd.message),
                        colour: {text: cmd.colour}
                    }));
                }
            }
            if (cmd.type === "ShowChatWindow") {
                toggleMainChatWindow();
            }
        }
    }
    
    if (channel === SYSTEM_NOTIFICATION_CHANNEL && sender == MyAvatar.sessionUUID) {
        var cmd = {FAILED: true};
        try {
            cmd = JSON.parse(message);
        } catch (e) {
            //
        }
        if (!cmd.FAILED) {
            if (cmd.type === "Update-Notification") {
                addToLog(cmd.message, cmd.category, cmd.colour, cmd.channel);
                
                Messages.sendLocalMessage(FLOOF_NOTIFICATION_CHANNEL, JSON.stringify({
                    sender: "(" + cmd.category + ")",
                    text: replaceFormatting(cmd.message),
                    colour: {text: cmd.colour}
                }));
            }
        }
    }
}

function time() {
    var d = new Date();
    // Months are returned in range 0-11 instead of 1-12, so we have to add 1.
    var month = (d.getMonth() + 1).toString(); 
    var day = (d.getDate()).toString();
    var h = (d.getHours()).toString();
    var m = (d.getMinutes()).toString();
    var s = (d.getSeconds()).toString();
    var h2 = ("0" + h).slice(-2);
    var m2 = ("0" + m).slice(-2);
    var s2 = ("0" + s).slice(-2);
    // s2 += (d.getMilliseconds() / 1000).toFixed(2).slice(1);
    return month + "/" + day + " - " + h2 + ":" + m2 + ":" + s2;
}

function addToLog(msg, dp, colour, tab) {
    var currentTimestamp = time();
    historyLog.push([currentTimestamp, msg, dp, colour, tab]);
    chatHistory.emitScriptEvent(JSON.stringify({ type: "MSG", data: [[currentTimestamp, msg, dp, colour, tab]]}));
    while (historyLog.length > chatHistoryLimit) {
        historyLog.shift();
    }
    Settings.setValue(settingsRoot + "/HistoryLog", JSON.stringify(historyLog))
}

function addToChatBarHistory(msg) {
    chatBarHistory.unshift(msg);
    while (chatBarHistory.length > chatBarHistoryLimit) {
        chatBarHistory.pop();
    }
    Settings.setValue(settingsRoot + "/chatBarHistory", chatBarHistory);
}

function fromQml(message) {
    var cmd = {FAILED: true};
    try {
        cmd = JSON.parse(message);
    } catch (e) {
        //
    }
    if (!cmd.FAILED) {
        if (cmd.type === "MSG") {
            if (cmd.message !== "") {
                addToChatBarHistory(cmd.message);
                if (cmd.event.modifiers === CONTROL_KEY) {
                    cmd.avatarName = MyAvatar.sessionDisplayName;
                    cmd = processChat(cmd);
                    if (cmd.message === "") return;
                    Messages.sendMessage(FLOOF_CHAT_CHANNEL, JSON.stringify({
                        type: "TransmitChatMessage", 
                        channel: "Domain", 
                        colour: chatColour("Domain"),
                        message: cmd.message,
                        displayName: cmd.avatarName
                    }));
                } else if (cmd.event.modifiers === CONTROL_KEY + SHIFT_KEY) {
                    cmd.avatarName = MyAvatar.sessionDisplayName;
                    cmd = processChat(cmd);
                    if (cmd.message === "") return;
                    sendWS({
                        uuid: "",
                        type: "WebChat",
                        channel: "Grid",
                        colour: chatColour("Grid"),
                        message: cmd.message,
                        displayName: cmd.avatarName
                    });
                } else {
                    cmd.avatarName = MyAvatar.sessionDisplayName;
                    cmd = processChat(cmd);
                    if (cmd.message === "") return;
                    Messages.sendMessage(FLOOF_CHAT_CHANNEL, JSON.stringify({
                        type: "TransmitChatMessage",
                        channel: chatBarChannel,
                        position: MyAvatar.position,
                        colour: chatColour(chatBarChannel),
                        message: cmd.message,
                        displayName: cmd.avatarName
                    }));
                }
            }
            setVisible(false);
        } else if (cmd.type === "CMD") {
            if (cmd.cmd === "Clicked") {
                toggleMainChatWindow()
            }
        }
    }
}

function setVisible(_visible) {
    if (_visible) {
        Messages.sendLocalMessage(FLOOF_NOTIFICATION_CHANNEL, JSON.stringify({
            type: "options",
            offset: 64
        }));
        chatBar.sendToQml(JSON.stringify({visible: true}));
    } else {
        Messages.sendLocalMessage(FLOOF_NOTIFICATION_CHANNEL, JSON.stringify({
            type: "options",
            offset: -1
        }));
        chatBar.sendToQml(JSON.stringify({visible: false}));
    }
    visible = _visible;
}

function avatarJoinsDomain(sessionID) {
    Script.setTimeout(function () {
        var messageText = AvatarManager.getPalData([sessionID]).data[0].sessionDisplayName + " has joined."
        var messageColor = { red: 122, green: 122, blue: 122 };
        
        addToLog(messageText, "Notice", messageColor, "Domain");
        
        if (!mutedAudio["Domain"]) {
            playNotificationSound();
        }
        
        if (!muted["Domain"]) {
            Messages.sendLocalMessage(FLOOF_NOTIFICATION_CHANNEL, JSON.stringify({
                sender: "(D)",
                text:  messageText,
                colour: { text: messageColor }
            }));
        }
    }, 500); // Wait 500ms for the avatar to load to properly get info about them.
}

function avatarLeavesDomain(sessionID) {
    var messageText = AvatarManager.getPalData([sessionID]).data[0].sessionDisplayName + " has left."
    var messageColor = { red: 122, green: 122, blue: 122 };
    
    addToLog(messageText, "Notice", messageColor, "Domain");
    
    if (!mutedAudio["Domain"]) {
        playNotificationSound();
    }
    
    if (!muted["Domain"]) {
        Messages.sendLocalMessage(FLOOF_NOTIFICATION_CHANNEL, JSON.stringify({
            sender: "(D)",
            text:  messageText,
            colour: { text: messageColor }
        }));
    }
}

function keyPressEvent(event) {
    if (event.key === H_KEY && !event.isAutoRepeat && event.isControl) {
        toggleMainChatWindow()
    }
    if (event.key === ENTER_KEY && !event.isAutoRepeat && !visible) {
        setVisible(true);
    }
    if (event.key === ESC_KEY && !event.isAutoRepeat && visible) {
        setVisible(false);
    }
}

function shutdown() {
    try {
        Messages.messageReceived.disconnect(messageReceived);
    } catch (e) {
        // empty
    }
    
    try {
        AvatarManager.avatarAddedEvent.disconnect(avatarJoinsDomain);
    } catch (e) {
        // empty
    }
    
    try {
        AvatarManager.avatarRemovedEvent.disconnect(avatarLeavesDomain);
    } catch (e) {
        // empty
    }
    
    try {
        Controller.keyPressEvent.disconnect(keyPressEvent);
    } catch (e) {
        // empty
    }
    
    chatBar.close();
    chatHistory.close();
}
