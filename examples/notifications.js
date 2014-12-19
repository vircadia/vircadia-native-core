// 
//  notifications.js
//  Created by Adrian 
//
//  Adrian McCarlie 8-10-14
//  This script demonstrates on-screen overlay type notifications.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

//  This script demonstrates notifications created via a number of ways, such as:
//  Simple key press alerts, which only depend on a key being pressed, 
//    dummy examples of this are "q", "w", "e", "r", and "SPACEBAR". 
//    actual working examples are "a" for left turn, "d" for right turn and Ctrl/s for snapshot.

//  System generated alerts such as users joining and leaving and chat messages which mention this user.
//  System generated alerts which originate with a user interface event such as Window Resize, and Mic Mute/Unmute.
//    Mic Mute/Unmute may appear to be a key press alert, but it actually gets the call from the system as mic is muted and unmuted,
//    so the mic mute/unmute will also trigger the notification by clicking the Mic Mute button at the top of the screen.


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
//  if (key.text == "a") {
//      var noteString = "Turning to the Left";
//      createNotification(noteString);
//  }


var width = 340.0; //width of notification overlay
var height = 40.0; // height of a single line notification overlay
var windowDimensions = Controller.getViewportDimensions(); // get the size of the interface window
var overlayLocationX = (windowDimensions.x - (width + 60.0));// positions window 60px from the right of the interface window
var buttonLocationX = overlayLocationX + (width - 28.0); 
var locationY = 20.0; // position down from top of interface window
var topMargin = 13.0;
var leftMargin = 10.0;
var textColor =  { red: 228, green: 228, blue: 228}; // text color
var backColor =  { red: 38, green: 38, blue: 38}; // background color
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
			
//  When our script shuts down, we should clean up all of our overlays
function scriptEnding() { 
    for (i = 0; i < notifications.length; i++) {
        Overlays.deleteOverlay(notifications[i]);
        Overlays.deleteOverlay(buttons[i]);
    }
}
Script.scriptEnding.connect(scriptEnding);

var notifications = [];
var buttons = [];	
var times = [];
var heights = [];
var myAlpha = [];
var arrays = [];

//  This function creates and sizes the overlays
function createNotification(text) {
    var count = (text.match(/\n/g) || []).length;
    var breakPoint = 43.0; // length when new line is added
    var extraLine = 0;
    var breaks = 0;
    var height = 40.0;
    var stack = 0;
    if (text.length >= breakPoint) {
        breaks = count;
    }
    var extraLine = breaks * 16.0;
    for (i = 0; i < heights.length; i++) {
        stack = stack + heights[i];
    }
    var level = (stack  + 20.0);
    height = height + extraLine;
    var overlayProperties = {
        x: overlayLocationX,
        y: level,  
        width: width,
        height: height,
        color: textColor,
        backgroundColor: backColor,
        alpha: backgroundAlpha,
        backgroundAlpha: backgroundAlpha,
        topMargin: topMargin,
        leftMargin: leftMargin,
        font: {size: fontSize},
        text: text,						
        };
    var bLevel = level + 12.0;
    var buttonProperties = { 
        x: buttonLocationX,
        y: bLevel,
        width: 15.0,
        height: 15.0,
        subImage: { x: 0, y: 0, width: 10, height: 10 },
        imageURL: "http://hifi-public.s3.amazonaws.com/images/close-small-light.svg",
        color: { red: 255, green: 255, blue: 255},
        visible: true,
        alpha: backgroundAlpha,
        };		
					
    Notify(overlayProperties, buttonProperties, height);	
	
}

//  Pushes data to each array and sets up data for 2nd dimension array 
//  to handle auxiliary data not carried by the overlay class
//  specifically notification "heights", "times" of creation, and . 
function Notify(notice, button, height){ 
		
    notifications.push((Overlays.addOverlay("text", notice)));
    buttons.push((Overlays.addOverlay("image",button)));
    times.push(new Date().getTime() / 1000);
    height = height + 1.0;
    heights.push(height);
    myAlpha.push(0);
    var last = notifications.length - 1;
    createArrays(notifications[last], buttons[last], times[last], heights[last], myAlpha[last]);
    fadeIn(notifications[last], buttons[last])
}

function fadeIn(noticeIn, buttonIn) {
    var	myLength = arrays.length;
    var q = 0;
    var pauseTimer = null;
    pauseTimer = Script.setInterval(function() { 
        q++;
        qFade = q / 10.0;
        Overlays.editOverlay(noticeIn, {alpha: qFade, backgroundAlpha: qFade}); 
        Overlays.editOverlay(buttonIn, {alpha: qFade});	
        if (q >= 9.0) {
            Script.clearInterval(pauseTimer);
        }
    }, 10);
}


//  push data from above to the 2 dimensional array
function createArrays(notice, button, createTime, height, myAlpha) {  
    arrays.push([notice, button, createTime, height, myAlpha]); 
}
//  handles mouse clicks on buttons
function mousePressEvent(event) {	
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y}); //identify which overlay was clicked
    for (i = 0; i < buttons.length; i++) { //if user clicked a button
        if(clickedOverlay == buttons[i]) {
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
    if (key.key == 16777249) {
        ctrlIsPressed = false;
    }
}
	
//  Triggers notification on specific key driven events
function keyPressEvent(key) {
    if (key.key == 16777249) {
        ctrlIsPressed = true;
    }
    if (key.text == "a") {
        var noteString = "Turning to the Left";
        createNotification(noteString);
        }
    if (key.text == "d") {	
        var noteString = "Turning to the Right";
        createNotification(noteString);
    }
    if (key.text == "s") { 		
        if (ctrlIsPressed == true){
            var noteString = "You have taken a snapshot";
            createNotification(noteString);
        }
    }
    if (key.text == "q") { 		
        var noteString = "Enable Scripted Motor control is now on.";
        wordWrap(noteString);
    }
    if (key.text == "w") { 			
        var noteString = "This notification spans 2 lines. The overlay will resize to fit new lines.";
        var noteString = "editVoxels.js stopped, editModels.js stopped, selectAudioDevice.js stopped.";		
        wordWrap(noteString);
    }	
    if (key.text == "e") { 			
        var noteString = "This is an example of a multiple line notification. This notification will span 3 lines."
        wordWrap(noteString);
    }				
    if (key.text == "r") { 			
        var noteString = "This is a very long line of text that we are going to use in this example to divide it into rows of maximum 43 chars and see how many lines we use.";
        wordWrap(noteString);
    }											
    if (key.text == "SPACE") {
        var noteString = "You have pressed the Spacebar, This is an example of a multiple line notification. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam.";
        wordWrap(noteString);
    }	
}

//  formats string to add newline every 43 chars
function wordWrap(str) {
    var result = stringDivider(str, 43.0, "\n");
    createNotification(result);
}
//  wraps whole word to newline
function stringDivider(str, slotWidth, spaceReplacer) {
    if (str.length > slotWidth) {
        var p = slotWidth;
        for (; p > 0 && str[p] != ' '; p--) {
        }
        if (p > 0) {
            var left = str.substring(0, p);
            var right = str.substring(p + 1);
            return left + spaceReplacer + stringDivider(right, slotWidth, spaceReplacer);
        }
    }
    return str;	
}

//  This fires a notification on window resize
function checkSize(){
    if((Window.innerWidth != ourWidth)||(Window.innerHeight != ourHeight)) {
        var windowResize = "Window has been resized";
        ourWidth = Window.innerWidth;
        ourHeight = Window.innerHeight;	
        windowDimensions = Controller.getViewportDimensions();
        overlayLocationX = (windowDimensions.x - (width + 60.0));
        buttonLocationX = overlayLocationX + (width - 35.0);
        createNotification(windowResize)
    }
}

//  Triggers notification if a user logs on or off
function onOnlineUsersChanged(users) {
    var joiners = [];
    var leavers = [];
    for (user in users) {
    if (last_users.indexOf(users[user]) == -1.0) {
        joiners.push(users[user]);
        createNotification(users[user] + " Has joined");		
        }
    }
    for (user in last_users) {
        if (users.indexOf(last_users[user]) == -1.0) {
            leavers.push(last_users[user]);
            createNotification(last_users[user] + " Has left");					
        }
    }
    last_users = users;
}

//  Triggers notification if @MyUserName is mentioned in chat and returns the message to the notification.
function onIncomingMessage(user, message) {
   var myMessage =  message;
   var alertMe = "@" + GlobalServices.myUsername;
   var thisAlert = user + ": " + myMessage;
   if (myMessage.indexOf(alertMe) > -1.0) {
        wordWrap(thisAlert);
    }
}
//  Triggers mic mute notification
function onMuteStateChanged() {
    var muteState =  AudioDevice.getMuted() ?  "Muted" :  "Unmuted";
    var muteString = "Microphone is set to " + muteState;
    createNotification(muteString);
}

function update(){
    frame++;
    if ((frame % 60.0) == 0) { // only update once a second
        checkSize(); // checks for size change to trigger windowResize notification
        locationY = 20.0;
        for (var i = 0; i < arrays.length; i++) { //repositions overlays as others fade
            var nextOverlay = Overlays.getOverlayAtPoint({x: overlayLocationX, y: locationY});	
            Overlays.editOverlay(notifications[i], { x:overlayLocationX, y:locationY});
            Overlays.editOverlay(buttons[i], { x:buttonLocationX, y:locationY + 12.0});
            locationY = locationY + arrays[i][3];
        }
    }

//  This checks the age of the notification and prepares to fade it after 9.0 seconds (var persistTime - 1)
    for (var i = 0; i < arrays.length; i++) { 	
        if (ready){		
            var j = arrays[i][2];		
            var k = j + persistTime; 
            if (k  < (new Date().getTime() / 1000)) {
                ready = false;	
                noticeOut = arrays[i][0];
                buttonOut = arrays[i][1];
                var arraysOut = i;
                fadeOut(noticeOut, buttonOut, arraysOut);
            }
        }
    }
}

//  this fades the notification ready for dismissal, and removes it from the arrays
function fadeOut(noticeOut, buttonOut, arraysOut) {
    var myLength = arrays.length;
    var r = 9.0;
    var pauseTimer = null;
    pauseTimer = Script.setInterval(function() { 
        r--;
        rFade = r / 10.0;	
        Overlays.editOverlay(noticeOut, {alpha: rFade, backgroundAlpha: rFade}); 
        Overlays.editOverlay(buttonOut, {alpha: rFade});	
        if (r < 0) {
            dismiss(noticeOut, buttonOut, arraysOut);
            arrays.splice(arraysOut, 1);
            ready = true;
            Script.clearInterval(pauseTimer);
        }
    }, 20);
}

//  This handles the final dismissal of a notification after fading
function dismiss(firstNoteOut, firstButOut, firstOut) { 
    var working = firstOut
    Overlays.deleteOverlay(firstNoteOut);
    Overlays.deleteOverlay(firstButOut);
    notifications.splice(firstOut, 1);
    buttons.splice(firstOut, 1);
    times.splice(firstOut, 1);
    heights.splice(firstOut, 1);
    myAlpha.splice(firstOut,1);
}

onMuteStateChanged();
AudioDevice.muteToggled.connect(onMuteStateChanged);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.mousePressEvent.connect(mousePressEvent);
GlobalServices.onlineUsersChanged.connect(onOnlineUsersChanged);
GlobalServices.incomingMessage.connect(onIncomingMessage);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.update.connect(update);
