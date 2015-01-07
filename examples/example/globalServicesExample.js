//
//  globalServicesExample.js
//  examples
//
//  Created by Thijs Wenker on 9/12/14.
//  Copyright 2014 High Fidelity, Inc.
//  
//  Example usage of the GlobalServices object. You could use it to make your own chatbox.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function onConnected() {
    if (GlobalServices.onlineUsers.length > 0) {
        sendMessageForm()
        return;
    }
    Script.setTimeout(function() { sendMessageForm(); }, 5000);
}

function onDisconnected(reason) {
    switch(reason) {
        case "logout":
            Window.alert("logged out!");
            break;
        
    }
}

function onOnlineUsersChanged(users) {
    print(users);
}

function onIncommingMessage(user, message) {
    print(user + ": " + message);
    if (message === "hello") {
        GlobalServices.chat("hello, @" + user + "!");
    }
}

function sendMessageForm() {
    var form =
    [
        { label: "To:", options: ["(noone)"].concat(GlobalServices.onlineUsers) },
        { label: "Message:", value: "Enter message here" }
    ];
    if (Window.form("Send message on public chat", form)) {
        GlobalServices.chat(form[0].value == "(noone)" ? form[1].value : "@" + form[0].value + ", " + form[1].value);
        
    }
}

GlobalServices.connected.connect(onConnected);
GlobalServices.disconnected.connect(onDisconnected);
GlobalServices.onlineUsersChanged.connect(onOnlineUsersChanged);
GlobalServices.incomingMessage.connect(onIncommingMessage);