/* globals OverlayWindow */

var ROOT = Script.resolvePath('').split("FloofChat.js")[0];

Script.scriptEnding.connect(function () {
    shutdown();
});

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var button = tablet.addButton({
    icon: ROOT + "chat.png",
    text: "Chat\nHistory"
});

Script.scriptEnding.connect(function () { // So if anything errors out the tablet/toolbar button gets removed!
    tablet.removeButton(button);
});

var appUUID = Uuid.generate();

var chatBar;
var chatHistory;
var historyLog = [];

var visible = false;
var historyVisible = false;

init();

function init() {
    Messages.subscribe("Chat");
    historyLog = [];
    try {
        historyLog = JSON.parse(Settings.getValue("HistoryLog", "[]"));
    } catch (e) {
        //
    }

    setupHistoryWindow(false);

    chatBar = new OverlayWindow({
        // source: "http://localhost:8000/fluffytest.qml",
        source: Paths.defaultScripts + '/communityModules/chat/FloofChat.qml?' + Date.now(),
        width: 360,
        height: 180
    });

    button.clicked.connect(toggleChatHistory);
    chatBar.fromQml.connect(fromQml);
    chatBar.sendToQml(JSON.stringify({visible: false}));
    Controller.keyPressEvent.connect(keyPressEvent);
    Messages.messageReceived.connect(messageReceived);
}

function setupHistoryWindow(popout) {
    if (!popout) {
        chatHistory = new OverlayWebWindow({
            title: 'Chat History',
            source: ROOT + "FloofChat2.html?appUUID=" + appUUID + "&" + Date.now(),
            width: 900,
            height: 700,
            visible: false
        });
        chatHistory.setPosition({x: 0, y: Window.innerHeight - 700});
        chatHistory.webEventReceived.connect(onWebEventReceived);
        chatHistory.closed.connect(toggleChatHistory);
    } else {
        chatHistory = Desktop.createWindow(Paths.defaultScripts + '/communityModules/chat/webview.qml?' + Date.now(), {
            title: "Chat History",
            additionalFlags: Desktop.CLOSE_BUTTON_HIDES,
            presentationMode: Desktop.PresentationMode.NATIVE,
            visible: false,
            size: {x: 900, y: 700},
            position: {x: 0, y: Window.innerHeight - 700}
        });
        chatHistory.webEventReceived.connect(onWebEventReceived);
        chatHistory.closed.connect(toggleChatHistory);
    }
}

function emitScriptEvent(obj) {
    obj.appUUID = appUUID;
    tablet.emitScriptEvent(JSON.stringify(obj));
}

function toggleChatHistory() {
    historyVisible = !historyVisible;
    button.editProperties({isActive: historyVisible});
    chatHistory.visible = historyVisible;
}

function onWebEventReceived(event) {
    console.log("event " + event);
    event = JSON.parse(event);
    if (event.type === "ready") {
        chatHistory.emitScriptEvent(JSON.stringify(historyLog));
    }
    if (event.type === "CMD") {
        if (event.cmd === "REDOCK") {
            if(popout){
                
            }else{
                chatHistory.setPosition({x: 0, y: Window.innerHeight - 700});
            }
        }
        if (event.cmd === "GOTO") {
            var result = Window.confirm("Do you want to goto " + event.url.split("/")[2] + " ?");
            if (result) {
                location = event.url;
            }
        }
        if (event.cmd === "URL") {
            new OverlayWebWindow({
                title: 'Web',
                source: event.url,
                width: 900,
                height: 700,
                visible: true
            });
        }
        if (event.cmd === "COPY") {
            Window.copyToClipboard(event.url);
        }
    }
    if (event.type === "MSG") {
        if (event.message === "") return;
        Messages.sendMessage("Chat", JSON.stringify({
            type: "TransmitChatMessage",
            position: MyAvatar.position,
            channel: event.tab,
            colour: {red: 222, green: 222, blue: 222},
            message: event.message,
            displayName: MyAvatar.displayName
        }));
        setVisible(false);
    }
}

function messageReceived(channel, message, sender, local) {
    if (channel === "Chat" || channel === "Support") {

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
                if (cmd.channel === "Local") {
                    if (Vec3.withinEpsilon(MyAvatar.position, cmd.position, 20)) {
                        addToLog(cmd.message, cmd.displayName, cmd.colour, cmd.channel);
                        Messages.sendLocalMessage("Floof-Notif", JSON.stringify({
                            sender: "L " + cmd.displayName,
                            text: cmd.message,
                            colour: {text: cmd.colour}
                        }));
                    }
                } else {
                    addToLog(cmd.message, cmd.displayName, cmd.colour, cmd.channel);
                    Messages.sendLocalMessage("Floof-Notif", JSON.stringify({
                        sender: ((cmd.channel === "Domain") ? "D " : "") + cmd.displayName,
                        text: cmd.message,
                        colour: {text: cmd.colour}
                    }));
                }
            }
        }
    }
}

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

function addToLog(msg, dp, colour, tab) {
    historyLog.push([time(), msg, dp, colour, tab]);
    chatHistory.emitScriptEvent(JSON.stringify([[time(), msg, dp, colour, tab]]));
    if (historyLog.length > 500) {
        historyLog.pop();
    }
    Settings.setValue("HistoryLog", JSON.stringify(historyLog))
}

function fromQml(message) {
    var cmd = {FAILED: true};
    try {
        cmd = JSON.parse(message);
    } catch (e) {
        //
    }
    if (!cmd.FAILED) {
        console.log(JSON.stringify(cmd.event));
        if (cmd.type === "MSG") {
            if (cmd.message !== "") {
                if (cmd.event.modifiers === 67108864) {
                    Messages.sendMessage("Chat", JSON.stringify({
                        type: "TransmitChatMessage", channel: "Domain", colour: {red: 222, green: 222, blue: 222},
                        message: cmd.message,
                        displayName: MyAvatar.displayName
                    }));
                } else {
                    Messages.sendMessage("Chat", JSON.stringify({
                        type: "TransmitChatMessage",
                        channel: "Local",
                        position: MyAvatar.position,
                        colour: {red: 222, green: 222, blue: 222},
                        message: cmd.message,
                        displayName: MyAvatar.displayName
                    }));
                }
            }
            setVisible(false);
        } else if (cmd.type === "CMD") {
            if (cmd.cmd === "Clicked") {
                toggleChatHistory()
            }

            /*
            Messages.sendLocalMessage("Floof-Notif", JSON.stringify({
                sender: "Chat",
                text: msg,
                colour: {text: {red: 200, green: 200, blue: 200}}
            }));*/
        }
    }
}

function setVisible(_visible) {
    if (_visible) {
        Messages.sendLocalMessage("Floof-Notif", JSON.stringify({
            type: "options",
            offset: 64
        }));
        chatBar.sendToQml(JSON.stringify({visible: true}));
    } else {
        Messages.sendLocalMessage("Floof-Notif", JSON.stringify({
            type: "options",
            offset: -1
        }));
        chatBar.sendToQml(JSON.stringify({visible: false}));
    }

    visible = _visible;
}

function keyPressEvent(event) {
    if (event.key === 72 && !event.isAutoRepeat && event.isControl) {
        toggleChatHistory()
    }
    if (event.key === 16777220 && !event.isAutoRepeat && !visible) {
        setVisible(true);
    }
    if (event.key === 16777216 && !event.isAutoRepeat && visible) {
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
        Controller.keyPressEvent.disconnect(keyPressEvent);
    } catch (e) {
        // empty
    }
    chatBar.close();
    chatHistory.close();
}
