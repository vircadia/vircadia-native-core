"use strict";

// Chat.js
// By Don Hopkins (dhopkins@donhopkins.com)
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var webPageURL = Script.resolvePath("html/ChatPage.html"); // URL of tablet web page.
    var randomizeWebPageURL = true; // Set to true for debugging.
    var lastWebPageURL = ""; // Last random URL of tablet web page.
    var onChatPage = false; // True when chat web page is opened.
    var webHandlerConnected = false; // True when the web handler has been connected.
    var channelName = "Chat"; // Unique name for channel that we listen to.
    var tabletButtonName = "CHAT"; // Tablet button label.
    var tabletButtonIcon = "icons/tablet-icons/menu-i.svg"; // Icon for chat button.
    var tabletButtonActiveIcon = "icons/tablet-icons/menu-a.svg"; // Active icon for chat button.
    var tabletButton = null; // The button we create in the tablet.
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system"); // The awesome tablet.
    var chatLog = []; // Array of chat messages in the form of [avatarID, displayName, message, data].
    var avatarIdentifiers = {}; // Map of avatar ids to dict of identifierParams.
    var speechBubbleShowing = false; // Is the speech bubble visible?
    var speechBubbleMessage = null; // The message shown in the speech bubble.
    var speechBubbleData = null; // The data of the speech bubble message.
    var speechBubbleTextID = null; // The id of the speech bubble local text entity.
    var speechBubbleTimer = null; // The timer to pop down the speech bubble.
    var speechBubbleParams = null; // The params used to create or edit the speech bubble.

    // Persistent variables saved in the Settings.
    var chatName = ''; // The user's name shown in chat.
    var chatLogMaxSize = 100; // The maximum number of chat messages we remember.
    var sendTyping = true; // Send typing begin and end notification.
    var identifyAvatarDuration = 10; // How long to leave the avatar identity line up, in seconds.
    var identifyAvatarLineColor = { red: 0, green: 255, blue: 0 }; // The color of the avatar identity line.
    var identifyAvatarMyJointName = 'Head'; // My bone from which to draw the avatar identity line.
    var identifyAvatarYourJointName = 'Head'; // Your bone to which to draw the avatar identity line.
    var speechBubbleDuration = 10; // How long to leave the speech bubble up, in seconds.
    var speechBubbleTextColor = {red: 255, green: 255, blue: 255}; // The text color of the speech bubble.
    var speechBubbleBackgroundColor = {red: 0, green: 0, blue: 0}; // The background color of the speech bubble.
    var speechBubbleOffset = {x: 0, y: 0.3, z: 0.0}; // The offset from the joint to whic the speech bubble is attached.
    var speechBubbleJointName = 'Head'; // The name of the joint to which the speech bubble is attached.
    var speechBubbleLineHeight = 0.05; // The height of a line of text in the speech bubble.
    var SPEECH_BUBBLE_MAX_WIDTH = 1; // meters

    // Load the persistent variables from the Settings, with defaults.
    function loadSettings() {
        chatName = Settings.getValue('Chat_chatName', MyAvatar.displayName);
        if (!chatName) {
            chatName = randomAvatarName();
        }
        chatLogMaxSize = Settings.getValue('Chat_chatLogMaxSize', 100);
        sendTyping = Settings.getValue('Chat_sendTyping', true);
        identifyAvatarDuration = Settings.getValue('Chat_identifyAvatarDuration', 10);
        identifyAvatarLineColor = Settings.getValue('Chat_identifyAvatarLineColor', { red: 0, green: 255, blue: 0 });
        identifyAvatarMyJointName = Settings.getValue('Chat_identifyAvatarMyJointName', 'Head');
        identifyAvatarYourJointName = Settings.getValue('Chat_identifyAvatarYourJointName', 'Head');
        speechBubbleDuration = Settings.getValue('Chat_speechBubbleDuration', 10);
        speechBubbleTextColor = Settings.getValue('Chat_speechBubbleTextColor', {red: 255, green: 255, blue: 255});
        speechBubbleBackgroundColor = Settings.getValue('Chat_speechBubbleBackgroundColor', {red: 0, green: 0, blue: 0});
        speechBubbleOffset = Settings.getValue('Chat_speechBubbleOffset', {x: 0.0, y: 0.3, z:0.0});
        speechBubbleJointName = Settings.getValue('Chat_speechBubbleJointName', 'Head');
        speechBubbleLineHeight = Settings.getValue('Chat_speechBubbleLineHeight', 0.05);

        saveSettings();
    }

    // Save the persistent variables to the Settings.
    function saveSettings() {
        Settings.setValue('Chat_chatName', chatName);
        Settings.setValue('Chat_chatLogMaxSize', chatLogMaxSize);
        Settings.setValue('Chat_sendTyping', sendTyping);
        Settings.setValue('Chat_identifyAvatarDuration', identifyAvatarDuration);
        Settings.setValue('Chat_identifyAvatarLineColor', identifyAvatarLineColor);
        Settings.setValue('Chat_identifyAvatarMyJointName', identifyAvatarMyJointName);
        Settings.setValue('Chat_identifyAvatarYourJointName', identifyAvatarYourJointName);
        Settings.setValue('Chat_speechBubbleDuration', speechBubbleDuration);
        Settings.setValue('Chat_speechBubbleTextColor', speechBubbleTextColor);
        Settings.setValue('Chat_speechBubbleBackgroundColor', speechBubbleBackgroundColor);
        Settings.setValue('Chat_speechBubbleOffset', speechBubbleOffset);
        Settings.setValue('Chat_speechBubbleJointName', speechBubbleJointName);
        Settings.setValue('Chat_speechBubbleLineHeight', speechBubbleLineHeight);
    }

    // Reset the Settings and persistent variables to the defaults.
    function resetSettings() {
        Settings.setValue('Chat_chatName', null);
        Settings.setValue('Chat_chatLogMaxSize', null);
        Settings.setValue('Chat_sendTyping', null);
        Settings.setValue('Chat_identifyAvatarDuration', null);
        Settings.setValue('Chat_identifyAvatarLineColor', null);
        Settings.setValue('Chat_identifyAvatarMyJointName', null);
        Settings.setValue('Chat_identifyAvatarYourJointName', null);
        Settings.setValue('Chat_speechBubbleDuration', null);
        Settings.setValue('Chat_speechBubbleTextColor', null);
        Settings.setValue('Chat_speechBubbleBackgroundColor', null);
        Settings.setValue('Chat_speechBubbleOffset', null);
        Settings.setValue('Chat_speechBubbleJointName', null);
        Settings.setValue('Chat_speechBubbleLineHeight', null);

        loadSettings();
    }

    // Update anything that might depend on the settings.
    function updateSettings() {
        updateSpeechBubble();
        trimChatLog();
        updateChatPage();
    }

    // Trim the chat log so it is no longer than chatLogMaxSize lines.
    function trimChatLog() {
        if (chatLog.length > chatLogMaxSize) {
            chatLog.splice(0, chatLogMaxSize - chatLog.length);
        }
    }

    // Clear the local chat log.
    function clearChatLog() {
        //print("clearChatLog");
        chatLog = [];
        updateChatPage();
    }

    // We got a chat message from the channel. 
    // Trim the chat log, save the latest message in the chat log, 
    // and show the message on the tablet, if the chat page is showing.
    function handleTransmitChatMessage(avatarID, displayName, message, data) {
        //print("receiveChat", "avatarID", avatarID, "displayName", displayName, "message", message, "data", data);

        trimChatLog();
        chatLog.push([avatarID, displayName, message, data]);

        if (onChatPage) {
            tablet.emitScriptEvent(
                JSON.stringify({
                    type: "ReceiveChatMessage",
                    avatarID: avatarID,
                    displayName: displayName,
                    message: message,
                    data: data
                }));
        }
    }

    // Trim the chat log, save the latest log message in the chat log, 
    // and show the message on the tablet, if the chat page is showing.
    function logMessage(message, data) {
        //print("logMessage", message, data);

        trimChatLog();
        chatLog.push([null, null, message, data]);

        if (onChatPage) {
            tablet.emitScriptEvent(
                JSON.stringify({
                    type: "LogMessage",
                    message: message,
                    data: data
                }));
        }
    }

    // An empty chat message was entered.
    // Hide our speech bubble.
    function emptyChatMessage(data) {
        popDownSpeechBubble();
    }

    // Notification that we typed a keystroke.
    function type() {
        //print("type");
    }

    // Notification that we began typing. 
    // Notify everyone that we started typing.
    function beginTyping() {
        //print("beginTyping");
        if (!sendTyping) {
            return;
        }

        Messages.sendMessage(
            channelName, 
            JSON.stringify({
                type: 'AvatarBeginTyping',
                avatarID: MyAvatar.sessionUUID,
                displayName: chatName
            }));
    }

    // Notification that somebody started typing.
    function handleAvatarBeginTyping(avatarID, displayName) {
        //print("handleAvatarBeginTyping:", "avatarID", avatarID, displayName);
    }

    // Notification that we stopped typing.
    // Notify everyone that we stopped typing.
    function endTyping() {
        //print("endTyping");
        if (!sendTyping) {
            return;
        }

        Messages.sendMessage(
            channelName, 
            JSON.stringify({
                type: 'AvatarEndTyping',
                avatarID: MyAvatar.sessionUUID,
                displayName: chatName
            }));
    }

    // Notification that somebody stopped typing.
    function handleAvatarEndTyping(avatarID, displayName) {
        //print("handleAvatarEndTyping:", "avatarID", avatarID, displayName);
    }

    // Identify an avatar by drawing a line from our head to their head.
    // If the avatar is our own, then just draw a line up into the sky.
    function identifyAvatar(yourAvatarID) {
        //print("identifyAvatar", yourAvatarID);

        unidentifyAvatars();

        var myAvatarID = MyAvatar.sessionUUID;
        var myJointIndex = MyAvatar.getJointIndex(identifyAvatarMyJointName);
        var myJointRotation = 
            Quat.multiply(
                MyAvatar.orientation,
                MyAvatar.getAbsoluteJointRotationInObjectFrame(myJointIndex));
        var myJointPosition =
            Vec3.sum(
                MyAvatar.position, 
                Vec3.multiplyQbyV(
                    MyAvatar.orientation,
                    MyAvatar.getAbsoluteJointTranslationInObjectFrame(myJointIndex)));

        var yourJointIndex = -1;
        var yourJointPosition;

        if (yourAvatarID == myAvatarID) {

            // You pointed at your own name, so draw a line up from your head.

            yourJointPosition = {
                x: myJointPosition.x,
                y: myJointPosition.y + 1000.0,
                z: myJointPosition.z
            };

        } else {

            // You pointed at somebody else's name, so draw a line from your head to their head.

            var yourAvatar = AvatarList.getAvatar(yourAvatarID);
            if (!yourAvatar) {
                return;
            }
            
            yourJointIndex = yourAvatar.getJointIndex(identifyAvatarMyJointName)

            var yourJointRotation = 
                Quat.multiply(
                    yourAvatar.orientation,
                    yourAvatar.getAbsoluteJointRotationInObjectFrame(yourJointIndex));
            yourJointPosition =
                Vec3.sum(
                    yourAvatar.position, 
                    Vec3.multiplyQbyV(
                        yourAvatar.orientation,
                        yourAvatar.getAbsoluteJointTranslationInObjectFrame(yourJointIndex)));

        }

        var identifierParams = {
            parentID: myAvatarID,
            parentJointIndex: myJointIndex,
            lifetime: identifyAvatarDuration,
            start: myJointPosition,
            endParentID: yourAvatarID,
            endParentJointIndex: yourJointIndex,
            end: yourJointPosition,
            color: identifyAvatarLineColor,
            alpha: 1,
            lineWidth: 1
        };

        avatarIdentifiers[yourAvatarID] = identifierParams;

        identifierParams.lineID = Overlays.addOverlay("line3d", identifierParams);

        //print("ADDOVERLAY lineID", lineID, "myJointPosition", JSON.stringify(myJointPosition), "yourJointPosition", JSON.stringify(yourJointPosition), "lineData", JSON.stringify(lineData));

        identifierParams.timer =
            Script.setTimeout(function() {
                //print("DELETEOVERLAY lineID");
                unidentifyAvatar(yourAvatarID);
            }, identifyAvatarDuration * 1000);

    }

    // Stop identifying an avatar.
    function unidentifyAvatar(yourAvatarID) {
        //print("unidentifyAvatar", yourAvatarID);

        var identifierParams = avatarIdentifiers[yourAvatarID];
        if (!identifierParams) {
            return;
        }

        if (identifierParams.timer) {
            Script.clearTimeout(identifierParams.timer);
        }

        if (identifierParams.lineID) {
            Overlays.deleteOverlay(identifierParams.lineID);
        }

        delete avatarIdentifiers[yourAvatarID];
    }

    // Stop identifying all avatars.
    function unidentifyAvatars() {
        var ids = [];

        for (var avatarID in avatarIdentifiers) {
            ids.push(avatarID);
        }

        for (var i = 0, n = ids.length; i < n; i++) {
            var avatarID = ids[i];
            unidentifyAvatar(avatarID);
        }

    }

    // Turn to face another avatar.
    function faceAvatar(yourAvatarID, displayName) {
        //print("faceAvatar:", yourAvatarID, displayName);

        var myAvatarID = MyAvatar.sessionUUID;
        if (yourAvatarID == myAvatarID) {
            // You clicked on yourself.
            return;
        }

        var yourAvatar = AvatarList.getAvatar(yourAvatarID);
        if (!yourAvatar) {
            logMessage(displayName + ' is not here!', null);
            return;
        }

        // Project avatar positions to the floor and get the direction between those points,
        // then face my avatar towards your avatar.
        var yourPosition = yourAvatar.position;
        yourPosition.y = 0;
        var myPosition = MyAvatar.position;
        myPosition.y = 0;
        var myOrientation = Quat.lookAtSimple(myPosition, yourPosition);
        MyAvatar.orientation = myOrientation;
    }

    // Make a hopefully unique random anonymous avatar name.
    function randomAvatarName() {
        return 'Anon_' + Math.floor(Math.random() * 1000000);
    }

    // Change the avatar size to bigger.
    function biggerSize() {
        //print("biggerSize");
        logMessage("Increasing avatar size", null);
        MyAvatar.increaseSize();
    }

    // Change the avatar size to smaller.
    function smallerSize() {
        //print("smallerSize");
        logMessage("Decreasing avatar size", null);
        MyAvatar.decreaseSize();
    }

    // Set the avatar size to normal.
    function normalSize() {
        //print("normalSize");
        logMessage("Resetting avatar size to normal!", null);
        MyAvatar.resetSize();
    }

    // Send out a "Who" message, including our avatarID as myAvatarID,
    // which will be sent in the response, so we can tell the reply
    // is to our request.
    function transmitWho() {
        //print("transmitWho");
        logMessage("Who is here?", null);
        Messages.sendMessage(
            channelName,
            JSON.stringify({
                type: 'Who',
                myAvatarID: MyAvatar.sessionUUID
            }));
    }

    // Send a reply to a "Who" message, with a friendly message, 
    // our avatarID and our displayName. myAvatarID is the id
    // of the avatar who send the Who message, to whom we're
    // responding.
    function handleWho(myAvatarID) {
        var avatarID = MyAvatar.sessionUUID;
        if (myAvatarID == avatarID) {
            // Don't reply to myself.
            return;
        }

        var message = "I'm here!";
        var data = {};

        Messages.sendMessage(
            channelName,
            JSON.stringify({
                type: 'ReplyWho',
                myAvatarID: myAvatarID,
                avatarID: avatarID,
                displayName: chatName,
                message: message,
                data: data
            }));
    }

    // Receive the reply to a "Who" message. Ignore it unless we were the one
    // who sent it out (if myAvatarIS is our avatar's id).
    function handleReplyWho(myAvatarID, avatarID, displayName, message, data) {
        if (myAvatarID != MyAvatar.sessionUUID) {
            return;
        }

        handleTransmitChatMessage(avatarID, displayName, message, data);
    }

    // Handle input form the user, possibly multiple lines separated by newlines.
    // Each line may be a chat command starting with "/", or a chat message.
    function handleChatMessage(message, data) {

        var messageLines = message.trim().split('\n');

        for (var i = 0, n = messageLines.length; i < n; i++) {
            var messageLine = messageLines[i];

            if (messageLine.substr(0, 1) == '/') {
                handleChatCommand(messageLine, data);
            } else {
                transmitChatMessage(messageLine, data);
            }
        }

    }

    // Handle a chat command prefixed by "/".
    function handleChatCommand(message, data) {

        var commandLine = message.substr(1);
        var tokens = commandLine.trim().split(' ');
        var command = tokens[0];
        var rest = commandLine.substr(command.length + 1).trim();

        //print("commandLine", commandLine, "command", command, "tokens", tokens, "rest", rest);

        switch (command) {

            case '?':
            case 'help':
                logMessage('Type "/?" or "/help" for help', null);
                logMessage('Type "/name <name>" to set your chat name, or "/name" to use your display name. If your display name is not defined, a random name will be used.', null);
                logMessage('Type "/close" to close your overhead chat message.', null);
                logMessage('Type "/say <something>" to display a new message.', null);
                logMessage('Type "/clear" to clear your chat log.', null);
                logMessage('Type "/who" to ask who is in the chat session.', null);
                logMessage('Type "/bigger", "/smaller" or "/normal" to change your avatar size.', null);
                break;

            case 'name':
                if (rest == '') {
                    if (MyAvatar.displayName) {
                        chatName = MyAvatar.displayName;
                        saveSettings();
                        logMessage('Your chat name has been set to your display name "' + chatName + '".', null);
                    } else {
                        chatName = randomAvatarName();
                        saveSettings();
                        logMessage('Your avatar\'s display name is not defined, so your chat name has been set to "' + chatName + '".', null);
                    }
                } else {
                    chatName = rest;
                    saveSettings();
                    logMessage('Your chat name has been set to "' + chatName + '".', null);
                }
                break;

            case 'close':
                popDownSpeechBubble();
                logMessage('Overhead chat message closed.', null);
                break;

            case 'say':
                if (rest == '') {
                    emptyChatMessage(data);
                } else {
                    transmitChatMessage(rest, data);
                }
                break;

            case 'who':
                transmitWho();
                break;

            case 'clear':
                clearChatLog();
                break;

            case 'bigger':
                biggerSize();
                break;

            case 'smaller':
                smallerSize();
                break;

            case 'normal':
                normalSize();
                break;

            case 'resetsettings':
                resetSettings();
                updateSettings();
                break;

            case 'speechbubbleheight':
                var y = parseInt(rest);
                if (!isNaN(y)) {
                    speechBubbleOffset.y = y;
                }
                saveSettings();
                updateSettings();
                break;

            case 'speechbubbleduration':
                var duration = parseFloat(rest);
                if (!isNaN(duration)) {
                    speechBubbleDuration = duration;
                }
                saveSettings();
                updateSettings();
                break;

            default:
                logMessage('Unknown chat command. Type "/help" or "/?" for help.', null);
                break;

        }

    }

    // Send out a chat message to everyone.
    function transmitChatMessage(message, data) {
        //print("transmitChatMessage", 'avatarID', avatarID, 'displayName', displayName, 'message', message, 'data', data);

        popUpSpeechBubble(message, data);

        Messages.sendMessage(
            channelName, 
            JSON.stringify({
                type: 'TransmitChatMessage',
                avatarID: MyAvatar.sessionUUID,
                displayName: chatName,
                message: message,
                data: data
            }));

    }

    // Show the speech bubble.
    function popUpSpeechBubble(message, data) {
        //print("popUpSpeechBubble", message, data);

        popDownSpeechBubble();

        speechBubbleShowing = true;
        speechBubbleMessage = message;
        speechBubbleData = data;

        updateSpeechBubble();

        if (speechBubbleDuration > 0) {
            speechBubbleTimer = Script.setTimeout(
                function () {
                    popDownSpeechBubble();
                },
                speechBubbleDuration * 1000);
        }
    }

    // Update the speech bubble. 
    // This is factored out so we can update an existing speech bubble if any settings change.
    function updateSpeechBubble() {
        if (!speechBubbleShowing) {
            return;
        }

        var jointIndex = MyAvatar.getJointIndex(speechBubbleJointName);
        var dimensions = {
            x: 100.0,
            y: 100.0,
            z: 0.1
        };

        speechBubbleParams = {
            type: "Text",
            lifetime: speechBubbleDuration,
            parentID: MyAvatar.sessionUUID,
            jointIndex: jointIndex,
            dimensions: dimensions,
            lineHeight: speechBubbleLineHeight,
            leftMargin: 0,
            topMargin: 0,
            rightMargin: 0,
            bottomMargin: 0,
            faceCamera: true,
            drawInFront: true,
            ignoreRayIntersection: true,
            text: speechBubbleMessage,
            textColor: speechBubbleTextColor,
            color: speechBubbleTextColor,
            backgroundColor: speechBubbleBackgroundColor
        };

        // Only overlay text3d has a way to measure the text, not entities.
        // So we make a temporary one just for measuring text, then delete it.
        var speechBubbleTextOverlayID = Overlays.addOverlay("text3d", speechBubbleParams);
        var textSize = Overlays.textSize(speechBubbleTextOverlayID, speechBubbleMessage);
        try {
            Overlays.deleteOverlay(speechBubbleTextOverlayID);
        } catch (e) {}

        //print("updateSpeechBubble:", "speechBubbleMessage", speechBubbleMessage, "textSize", textSize.width, textSize.height);

        var fudge = 0.02;

        var width = textSize.width + fudge;
        var height = speechBubbleLineHeight + fudge;

        if (textSize.width >= SPEECH_BUBBLE_MAX_WIDTH) {
            var numLines = Math.ceil(width);
            height = speechBubbleLineHeight * numLines + fudge;
            width = SPEECH_BUBBLE_MAX_WIDTH;
        } 

        dimensions = {
            x: width,
            y: height,
            z: 0.1
        };
        speechBubbleParams.dimensions = dimensions;

        var headRotation = 
            Quat.multiply(
                MyAvatar.orientation,
                MyAvatar.getAbsoluteJointRotationInObjectFrame(jointIndex));
        var headPosition =
            Vec3.sum(
                MyAvatar.position, 
                Vec3.multiplyQbyV(
                    MyAvatar.orientation,
                    MyAvatar.getAbsoluteJointTranslationInObjectFrame(jointIndex)));
        var rotatedOffset =
            Vec3.multiplyQbyV(
                headRotation,
                speechBubbleOffset);
        var position =
            Vec3.sum(
                headPosition,
                rotatedOffset);
        position.y += height / 2; // offset based on half of bubble height
        speechBubbleParams.position = position;

        if (!speechBubbleTextID) {
            speechBubbleTextID = 
                Entities.addEntity(speechBubbleParams, true);
        } else {
            Entities.editEntity(speechBubbleTextID, speechBubbleParams);
        }

        //print("speechBubbleTextID:", speechBubbleTextID, "speechBubbleParams", JSON.stringify(speechBubbleParams));
    }

    // Hide the speech bubble.
    function popDownSpeechBubble() {
        cancelSpeechBubbleTimer();

        speechBubbleShowing = false;

        //print("popDownSpeechBubble speechBubbleTextID", speechBubbleTextID);

        if (speechBubbleTextID) {
            try {
                Entities.deleteEntity(speechBubbleTextID);
            } catch (e) {}
            speechBubbleTextID = null;
        }
    }

    // Cancel the speech bubble popup timer.
    function cancelSpeechBubbleTimer() {
        if (speechBubbleTimer) {
            Script.clearTimeout(speechBubbleTimer);
            speechBubbleTimer = null;
        }
    }

    // Show the tablet web page and connect the web handler.
    function showTabletWebPage() {
        var url = Script.resolvePath(webPageURL);
        if (randomizeWebPageURL) {
            url += '?rand=' + Math.random();
        }
        lastWebPageURL = url;
        onChatPage = true;
        tablet.gotoWebScreen(lastWebPageURL);
        // Connect immediately so we don't miss anything.
        connectWebHandler();
    }

    // Update the tablet web page with the chat log.
    function updateChatPage() {
        if (!onChatPage) {
            return;
        }

        tablet.emitScriptEvent(
            JSON.stringify({
                type: "Update",
                chatLog: chatLog
            }));
    }

    function onChatMessageReceived(channel, message, senderID) {

        // Ignore messages to any other channel than mine.
        if (channel != channelName) {
            return;
        }

        // Parse the message and pull out the message parameters.
        var messageData = JSON.parse(message);
        var messageType = messageData.type;

        //print("MESSAGE", message);
        //print("MESSAGEDATA", messageData, JSON.stringify(messageData));

        switch (messageType) {

            case 'TransmitChatMessage':
                handleTransmitChatMessage(messageData.avatarID, messageData.displayName, messageData.message, messageData.data);
                break;

            case 'AvatarBeginTyping':
                handleAvatarBeginTyping(messageData.avatarID, messageData.displayName);
                break;

            case 'AvatarEndTyping':
                handleAvatarEndTyping(messageData.avatarID, messageData.displayName);
                break;

            case 'Who':
                handleWho(messageData.myAvatarID);
                break;

            case 'ReplyWho':
                handleReplyWho(messageData.myAvatarID, messageData.avatarID, messageData.displayName, messageData.message, messageData.data);
                break;

            default:
                print("onChatMessageReceived: unknown messageType", messageType, "message", message);
                break;

        }

    }

    // Handle events from the tablet web page.
    function onWebEventReceived(event) {
        if (!onChatPage) {
            return;
        }

        //print("onWebEventReceived: event", event);

        var eventData = JSON.parse(event);
        var eventType = eventData.type;

        switch (eventType) {

            case 'Ready':
                updateChatPage();
                break;

            case 'Update':
                updateChatPage();
                break;

            case 'HandleChatMessage':
                var message = eventData.message;
                var data = eventData.data;
                //print("onWebEventReceived: HandleChatMessage:", 'message', message, 'data', data);
                handleChatMessage(message, data);
                break;

            case 'PopDownSpeechBubble':
                popDownSpeechBubble();
                break;

            case 'EmptyChatMessage':
                emptyChatMessage();
                break;

            case 'Type':
                type();
                break;

            case 'BeginTyping':
                beginTyping();
                break;

            case 'EndTyping':
                endTyping();
                break;

            case 'IdentifyAvatar':
                identifyAvatar(eventData.avatarID);
                break;

            case 'UnidentifyAvatar':
                unidentifyAvatar(eventData.avatarID);
                break;

            case 'FaceAvatar':
                faceAvatar(eventData.avatarID, eventData.displayName);
                break;

            case 'ClearChatLog':
                clearChatLog();
                break;

            case 'Who':
                transmitWho();
                break;

            case 'Bigger':
                biggerSize();
                break;

            case 'Smaller':
                smallerSize();
                break;

            case 'Normal':
                normalSize();
                break;

            default:
                print("onWebEventReceived: unexpected eventType", eventType);
                break;

        }
    }

    function onScreenChanged(type, url) {
        //print("onScreenChanged", "type", type, "url", url, "lastWebPageURL", lastWebPageURL);
    
        if ((type === "Web") && 
            (url === lastWebPageURL)) {
            if (!onChatPage) {
                onChatPage = true;
                connectWebHandler();
            }
        } else { 
            if (onChatPage) {
                onChatPage = false;
                disconnectWebHandler();
            }
        }

    }

    function connectWebHandler() {
        if (webHandlerConnected) {
            return;
        }

        try {
            tablet.webEventReceived.connect(onWebEventReceived);
        } catch (e) {
            print("connectWebHandler: error connecting: " + e);
            return;
        }

        webHandlerConnected = true;
        //print("connectWebHandler connected");

        updateChatPage();
    }

    function disconnectWebHandler() {
        if (!webHandlerConnected) {
            return;
        }

        try {
            tablet.webEventReceived.disconnect(onWebEventReceived);
        } catch (e) {
            print("disconnectWebHandler: error disconnecting web handler: " + e);
            return;
        }
        webHandlerConnected = false;

        //print("disconnectWebHandler: disconnected");
    }

    // Show the tablet web page when the chat button on the tablet is clicked.
    function onTabletButtonClicked() {
        showTabletWebPage();
    }

    // Shut down the chat application when the tablet button is destroyed.
    function onTabletButtonDestroyed() {
        shutDown();
    }

    // Start up the chat application.
    function startUp() {
        //print("startUp");

        loadSettings();

        tabletButton = tablet.addButton({
            icon: tabletButtonIcon,
            activeIcon: tabletButtonActiveIcon,
            text: tabletButtonName,
            sortOrder: 0
        });

        Messages.subscribe(channelName);

        tablet.screenChanged.connect(onScreenChanged);

        Messages.messageReceived.connect(onChatMessageReceived);

        tabletButton.clicked.connect(onTabletButtonClicked);

        Script.scriptEnding.connect(onTabletButtonDestroyed);

        logMessage('Type "/?" or "/help" for help with chat.', null);

        //print("Added chat button to tablet.");
    }

    // Shut down the chat application.
    function shutDown() {
        //print("shutDown");

        popDownSpeechBubble();
        unidentifyAvatars();
        disconnectWebHandler();

        if (onChatPage) {
            tablet.gotoHomeScreen();
            onChatPage = false;
        }

        tablet.screenChanged.disconnect(onScreenChanged);

        Messages.messageReceived.disconnect(onChatMessageReceived);

        // Clean up the tablet button we made.
        tabletButton.clicked.disconnect(onTabletButtonClicked);
        tablet.removeButton(tabletButton);
        tabletButton = null;

        //print("Removed chat button from tablet.");
    }

    // Kick off the chat application!
    startUp();

}());
