//
//  loadScriptFromMessage.js
//  examples
//
//  Created by Thijs Wenker on 9/15/14.
//  Copyright 2014 High Fidelity, Inc.
//  
//  Filters script links out of incomming messages and prompts you to run the script.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//Javascript link RegEX
const JS_LINK_REGEX = /https?:\/\/[^ ]+\.js/i;

function onIncomingMessage(user, message) {
    var script_link = JS_LINK_REGEX.exec(message);
    if (script_link == null) {
        return;
    }
    if (Window.confirm("@" + user + " sent the following script:\n" + script_link + "\nwould you like to run it?")) {
        Script.load(script_link);
    }
}

GlobalServices.incomingMessage.connect(onIncomingMessage);