// 
//  notifications.js 
//  Version 0.801 
//  Created by Adrian 
//
//  Adrian McCarlie 8-10-14
//  This script demonstrates on-screen overlay type notifications.
//  Copyright 2014 High Fidelity, Inc.
//
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

//  This script generates notifications created via a number of ways, such as:
//  keystroke:
//
//  "q" returns number of users currently online (for debug purposes)

//  CTRL/s for snapshot.
//  CTRL/m for mic mute and unmute.

//  System generated notifications:
//  Displays users online at startup. 
//  If Screen is resized.
//  Triggers notification if @MyUserName is mentioned in chat.
//  Announces existing user logging out.
//  Announces new user logging in.
//  If mic is muted for any reason.
//  
//  To add a new System notification type:
//
//  1. Set the Event Connector at the bottom of the script. 
//  example: 
//  GlobalServices.incomingMessage.connect(onIncomingMessage);
//
//  2. Create a new function to produce a text string, do not include new line returns. 
//  example:
//  function onIncomingMessage(user, message) {
//      //do stuff here; 
//      var text = "This is a notification";
//      wordWrap(text);
//  }
//
//  This new function must call wordWrap(text) if the length of message is longer than 42 chars or unknown.
//  wordWrap() will format the text to fit the notifications overlay and send it to createNotification(text). 
//  If the message is 42 chars or less you should bypass wordWrap() and call createNotification() directly.


//  To add a keypress driven notification:
//
//  1. Add a key to the keyPressEvent(key).
//  2. Declare a text string.
//  3. Call createNotifications(text) parsing the text.
//  example:
//    var welcome;
//    if (key.text == "q") { //queries number of users online
//        var welcome = "There are " + GlobalServices.onlineUsers.length + " users online now.";
//        createNotification(welcome);
//    }

var width = 340.0; //width of notification overlay
var height = 40.0; // height of a single line notification overlay
var windowDimensions = Controller.getViewportDimensions(); // get the size of the interface window
var overlayLocationX = (windowDimensions.x - (width + 20.0)); // positions window 20px from the right of the interface window
var buttonLocationX = overlayLocationX + (width - 28.0);
var locationY = 20.0; // position down from top of interface window
var topMargin = 13.0;
var leftMargin = 10.0;
var textColor =  { red: 228, green: 228, blue: 228}; // text color
var backColor =  { red: 2, green: 2, blue: 2}; // background color was 38,38,38 
var backgroundAlpha = 0;
var fontSize = 12.0;
var persistTime = 10.0; // time in seconds before notification fades
var clickedText = false;
var frame = 0;
var ourWidth = Window.innerWidth;
var ourHeight = Window.innerHeight;
var text = "placeholder";
var last_users = GlobalServices.onlineUsers;
var users = [];
var ctrlIsPressed = false;
var ready = true;
var notifications = [];
var buttons = [];
var times = [];
var heights = [];
var myAlpha = [];
var arrays = [];

//  push data from above to the 2 dimensional array
function createArrays(notice, button, createTime, height, myAlpha) {
    arrays.push([notice, button, createTime, height, myAlpha]);
}

//  This handles the final dismissal of a notification after fading
function dismiss(firstNoteOut, firstButOut, firstOut) {
    Overlays.deleteOverlay(firstNoteOut);
    Overlays.deleteOverlay(firstButOut);
    notifications.splice(firstOut, 1);
    buttons.splice(firstOut, 1);
    times.splice(firstOut, 1);
    heights.splice(firstOut, 1);
    myAlpha.splice(firstOut, 1);
}

function fadeIn(noticeIn, buttonIn) {
    var q = 0,
        qFade,
        pauseTimer = null;

    pauseTimer = Script.setInterval(function () {
        q += 1;
        qFade = q / 10.0;
        Overlays.editOverlay(noticeIn, { alpha: qFade });
        Overlays.editOverlay(buttonIn, { alpha: qFade });
        if (q >= 9.0) {
            Script.clearInterval(pauseTimer);
        }
    }, 10);
}

//  this fades the notification ready for dismissal, and removes it from the arrays
function fadeOut(noticeOut, buttonOut, arraysOut) {
    var r = 9.0,
        rFade,
        pauseTimer = null;

    pauseTimer = Script.setInterval(function () {
        r -= 1;
        rFade = r / 10.0;
        Overlays.editOverlay(noticeOut, { alpha: rFade });
        Overlays.editOverlay(buttonOut, { alpha: rFade });
        if (r < 0) {
            dismiss(noticeOut, buttonOut, arraysOut);
            arrays.splice(arraysOut, 1);
            ready = true;
            Script.clearInterval(pauseTimer);
        }
    }, 20);
}

//  Pushes data to each array and sets up data for 2nd dimension array 
//  to handle auxiliary data not carried by the overlay class
//  specifically notification "heights", "times" of creation, and . 
function notify(notice, button, height) {
    notifications.push((Overlays.addOverlay("text", notice)));
    buttons.push((Overlays.addOverlay("image", button)));
    times.push(new Date().getTime() / 1000);
    height = height + 1.0;
    heights.push(height);
    myAlpha.push(0);
    var last = notifications.length - 1;
    createArrays(notifications[last], buttons[last], times[last], heights[last], myAlpha[last]);
    fadeIn(notifications[last], buttons[last]);
}

//  This function creates and sizes the overlays
function createNotification(text) {
    var count = (text.match(/\n/g) || []).length,
        breakPoint = 43.0, // length when new line is added
        extraLine = 0,
        breaks = 0,
        height = 40.0,
        stack = 0,
        level,
        overlayProperties,
        bLevel,
        buttonProperties,
        i;

    if (text.length >= breakPoint) {
        breaks = count;
    }
    extraLine = breaks * 16.0;
    for (i = 0; i < heights.length; i += 1) {
        stack = stack + heights[i];
    }

    level = (stack + 20.0);
    height = height + extraLine;
    overlayProperties = {
        x: overlayLocationX,
        y: level,
        width: width,
        height: height,
        color: textColor,
        backgroundColor: backColor,
        alpha: backgroundAlpha,
        topMargin: topMargin,
        leftMargin: leftMargin,
        font: {size: fontSize},
        text: text
    };

    bLevel = level + 12.0;
    buttonProperties = {
        x: buttonLocationX,
        y: bLevel,
        width: 10.0,
        height: 10.0,
        subImage: { x: 0, y: 0, width: 10, height: 10 },
        imageURL: "http://hifi-public.s3.amazonaws.com/images/close-small-light.svg",
        color: { red: 255, green: 255, blue: 255},
        visible: true,
        alpha: backgroundAlpha
    };

    notify(overlayProperties, buttonProperties, height);
}

//  wraps whole word to newline
function stringDivider(str, slotWidth, spaceReplacer) {
    var p,
        left,
        right;

    if (str.length > slotWidth) {
        p = slotWidth;
        for (; p > 0 && str[p] != ' '; p--) {
        }
        if (p > 0) {
            left = str.substring(0, p);
            right = str.substring(p + 1);
            return left + spaceReplacer + stringDivider(right, slotWidth, spaceReplacer);
        }
    }
    return str;
}

//  formats string to add newline every 43 chars
function wordWrap(str) {
    var result = stringDivider(str, 43.0, "\n");
    createNotification(result);
}

//  This fires a notification on window resize
function checkSize() {
    if ((Window.innerWidth !== ourWidth) || (Window.innerHeight !== ourHeight)) {
        var windowResize = "Window has been resized";
        ourWidth = Window.innerWidth;
        ourHeight = Window.innerHeight;
        windowDimensions = Controller.getViewportDimensions();
        overlayLocationX = (windowDimensions.x - (width + 60.0));
        buttonLocationX = overlayLocationX + (width - 35.0);
        createNotification(windowResize);
    }
}

function update() {
    var nextOverlay,
        noticeOut,
        buttonOut,
        arraysOut,
        i,
        j,
        k;

    frame += 1;
    if ((frame % 60.0) === 0) { // only update once a second
        checkSize(); // checks for size change to trigger windowResize notification
        locationY = 20.0;
        for (i = 0; i < arrays.length; i += 1) { //repositions overlays as others fade
            nextOverlay = Overlays.getOverlayAtPoint({ x: overlayLocationX, y: locationY });
            Overlays.editOverlay(notifications[i], { x: overlayLocationX, y: locationY });
            Overlays.editOverlay(buttons[i], { x: buttonLocationX, y: locationY + 12.0 });
            locationY = locationY + arrays[i][3];
        }
    }

    //  This checks the age of the notification and prepares to fade it after 9.0 seconds (var persistTime - 1)
    for (i = 0; i < arrays.length; i += 1) {
        if (ready) {
            j = arrays[i][2];
            k = j + persistTime;
            if (k < (new Date().getTime() / 1000)) {
                ready = false;
                noticeOut = arrays[i][0];
                buttonOut = arrays[i][1];
                arraysOut = i;
                fadeOut(noticeOut, buttonOut, arraysOut);
            }
        }
    }
}

var STARTUP_TIMEOUT = 500,  // ms
    startingUp = true,
    startupTimer = null;

//  This reports the number of users online at startup
function reportUsers() {
    var welcome;

    welcome = "Welcome! There are " + GlobalServices.onlineUsers.length + " users online now.";
    createNotification(welcome);
}

function finishStartup() {
    startingUp = false;
    Script.clearTimeout(startupTimer);
    reportUsers();
}

function isStartingUp() {
    // Is starting up until get no checks that it is starting up for STARTUP_TIMEOUT
    if (startingUp) {
        if (startupTimer) {
            Script.clearTimeout(startupTimer);
        }
        startupTimer = Script.setTimeout(finishStartup, STARTUP_TIMEOUT);
    }
    return startingUp;
}

//  Triggers notification if a user logs on or off
function onOnlineUsersChanged(users) {
    var user;

    if (!isStartingUp()) {  // Skip user notifications at startup.
        for (user in users) {
            if (last_users.indexOf(users[user]) === -1.0) {
                createNotification(users[user] + " has joined");
            }
        }

        for (user in last_users) {
            if (users.indexOf(last_users[user]) === -1.0) {
                createNotification(last_users[user] + " has left");
            }
        }
    }

    last_users = users;
}

//  Triggers notification if @MyUserName is mentioned in chat and returns the message to the notification.
function onIncomingMessage(user, message) {
    var myMessage,
        alertMe,
        thisAlert;

    myMessage =  message;
    alertMe = "@" + GlobalServices.myUsername;
    thisAlert = user + ": " + myMessage;

    if (myMessage.indexOf(alertMe) > -1.0) {
        wordWrap(thisAlert);
    }
}

//  Triggers mic mute notification
function onMuteStateChanged() {
    var muteState,
        muteString;

    muteState =  AudioDevice.getMuted() ?  "muted" :  "unmuted";
    muteString = "Microphone is now " + muteState;
    createNotification(muteString);
}

//  handles mouse clicks on buttons
function mousePressEvent(event) {
    var clickedOverlay,
        i;

    clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y }); //identify which overlay was clicked

    for (i = 0; i < buttons.length; i += 1) { //if user clicked a button
        if (clickedOverlay === buttons[i]) {
            Overlays.deleteOverlay(notifications[i]);
            Overlays.deleteOverlay(buttons[i]);
            notifications.splice(i, 1);
            buttons.splice(i, 1);
            times.splice(i, 1);
            heights.splice(i, 1);
            myAlpha.splice(i, 1);
            arrays.splice(i, 1);
        }
    }
}

//  Control key remains active only while key is held down
function keyReleaseEvent(key) {
    if (key.key === 16777249) {
        ctrlIsPressed = false;
    }
}

//  Triggers notification on specific key driven events
function keyPressEvent(key) {
    var numUsers,
        welcome,
        noteString;

    if (key.key === 16777249) {
        ctrlIsPressed = true;
    }

    if (key.text === "q") { //queries number of users online
        numUsers = GlobalServices.onlineUsers.length;
        welcome = "There are " + numUsers + " users online now.";
        createNotification(welcome);
    }

    if (key.text === "s") {
        if (ctrlIsPressed === true) {
            noteString = "Snapshot taken.";
            createNotification(noteString);
        }
    }
}

//  When our script shuts down, we should clean up all of our overlays
function scriptEnding() {
    var i;

    for (i = 0; i < notifications.length; i += 1) {
        Overlays.deleteOverlay(notifications[i]);
        Overlays.deleteOverlay(buttons[i]);
    }
}

AudioDevice.muteToggled.connect(onMuteStateChanged);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.mousePressEvent.connect(mousePressEvent);
GlobalServices.onlineUsersChanged.connect(onOnlineUsersChanged);
GlobalServices.incomingMessage.connect(onIncomingMessage);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
