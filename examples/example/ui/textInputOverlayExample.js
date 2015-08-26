// 
//  textInputOverlayExample.js

//    Captures keystrokes and generates a string which is displayed as a text Overlay
//  demonstrates keystroke events, mouse click events and overlay buttons.
//  Created by Adrian McCarlie 7 October 2014
//
//  Copyright 2014 High Fidelity, Inc.
//
//  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var locationX = 500; // on screen location X
var locationY = 175; // on screen location Y
var width = 600; //width of input window
var height = 100; // height of input window
var topMargin = 20;
var leftMargin = 10;
var textColor =  { red: 228, green: 228, blue: 228}; // text color
var backColor =  { red: 170, green: 200, blue: 255}; // default background color
var readyColor = { red: 2, green: 255, blue: 2};  // green background and button
var clickedColor = { red: 255, green: 220, blue: 20}; //amber background and button
var backgroundAlpha = 0.6;
var fontSize = 12;
var writing ="Click here and type input, then click green button to submit.\n \n \n \n \n \n \n \n Click red button to close,";
var buttonLocationX = locationX - 50; // buttons locked relative to input window
var clickedText = false;
var clickedButton = false;
var cursor = "|";
// add more characters to the string if required
var keyString = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\
    ~!@#$%^&*()_+`1234567890-={}|[]\\:\";'<>?,./"; //permitted characters


// This will create a text overlay that displays what you type
var inputWindow = Overlays.addOverlay("text", {
    x: locationX,
    y: locationY,
    width: width,
    height: height,
    color: textColor,
    backgroundColor: backColor,
    alpha: backgroundAlpha,
    backgroundAlpha: backgroundAlpha,
    topMargin: topMargin,
    leftMargin: leftMargin,
    font: {size: fontSize},
    text: writing,
    visible: true
});
// This will create an image overlay of a button.
var button1 = Overlays.addOverlay("image", { // green button
    x: buttonLocationX,
    y: locationY + 10,
    width: 40,
    height: 35,
    subImage: { x: 0, y: 0, width: 39, height: 35 },
    imageURL: "https://public.highfidelity.io/images/thumb.png",
    color: readyColor,
    visible: true
});
// This will create an image overlay of another button.                
var button2 = Overlays.addOverlay("image", { // red button
    x: buttonLocationX,
    y: locationY + 60,
    width: 40,
    height: 35,
    subImage: { x: 0, y: 0, width: 39, height: 35 },
    imageURL: "https://public.highfidelity.io/images/thumb.png",
    color: { red: 250, green: 2, blue: 2},
    visible: true,
});                                            
// When our script shuts down, we should clean up all of our overlays
function scriptEnding() {
    Overlays.deleteOverlay(inputWindow);
    Overlays.deleteOverlay(button1);
    Overlays.deleteOverlay(button2);        
    //Return control of keys to default on script ending
    for(var i=0; i<keyString.length; i++){
        var nextChar = keyString.charAt(i);
        Controller.releaseKeyEvents({ text: nextChar}); 
    }    
    Controller.releaseKeyEvents({"key": 0x5c}); // forward slash    
    Controller.releaseKeyEvents({ text: "SHIFT"});       
    Controller.releaseKeyEvents({ text: "BACKSPACE"});    
    Controller.releaseKeyEvents({ text: "SPACE"});
    Controller.releaseKeyEvents({"key":16777220} ); //Enter    
    Controller.releaseKeyEvents({"key":16777219} ); //Backspace
}
Script.scriptEnding.connect(scriptEnding);


function resetForm(){
    writing = ""; // Start with a blank string                
    Overlays.editOverlay(button1, {color: readyColor} );
    Overlays.editOverlay(inputWindow, {backgroundColor: readyColor});
    clickedText = true;            
}

function submitForm(){
    print("form submitted");
    writingOutput = writing; // writingOutput is the data output
    writing = writing + ".\nYour data has been stored.\n\nClick here to reset form or \n\n\nClick red button to close,";
    Overlays.editOverlay(button1, { color: clickedColor} );
    Overlays.editOverlay(inputWindow, {backgroundColor: clickedColor});
    clickedText = false;
    clickedButton = true;    
}

// handle click detection
function mousePressEvent(event) {
    
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y}); //identify which overlay was clicked
    
    if (clickedOverlay == inputWindow) { // if the user clicked on the text window, prepare the overlay 
        if (clickedText == false){ // first time clicked?
            resetForm();        
        }
    }
        
    if (clickedOverlay == button1) { // if the user clicked on the green button
        if (clickedText == true){ // nothing happens unless the text window was clicked first
            submitForm();
            //    clickedText == false;
        }
        else { // if the form has been submitted already
            resetForm();
        }
    }
    
    if (clickedOverlay == button2) { // if the user clicked on the red button
        print ("script ending");
        Script.stop();
    }    
}

//handle key press detection
function keyPressEvent(key) {

    if (clickedText == true){

        if (key.text == "SPACE") { //special conditions for space bar
            writing = writing + " ";
            key.text ="";
        }    
        else if (key.text == "BACKSPACE") { // Backspace 
            var myString = writing;
            writing = myString.substr(0, myString.length-1); 
            key.text ="";
        } 
            
        if (key.text == "\r") { //special conditions for enter key
            writing = writing + "\n";
            key.text ="";
        }    
        else if ( keyString.indexOf(key.text) == -1) { // prevent all other keys not in keyString 
            key.text ="";
        }    
        // build the string
        writing = writing + key.text;    
    }            
}

var count = 0;
function updateWriting(deltaTime){
    count++;    
    // every half second or so, remove and replace the pipe to create a blinking cursor
    if (count % 30 == 0) {
        if (cursor == "|") {
            cursor="";
        } else {
            cursor = "|";
        }
    }
    //    attempt at some overflow control of the text
    if  ((writing.length % 53) == 0) { 
        writing = writing + "\n";
    }
    // add blinking cursor to window during typing
    var    addCursor = writing + cursor;
    if (clickedText == true){    
        Overlays.editOverlay(inputWindow, { text: addCursor});
    }else{
        Overlays.editOverlay(inputWindow, { text: writing});
    }
}

    
    
// test keystroke against keyString and capture permitted keys
for (var i=0; i<keyString.length; i++){
    var nextChar = keyString.charAt(i);
    Controller.captureKeyEvents({ text: nextChar}); 
}
// capture special keys
Controller.captureKeyEvents({ "key": 0x5c}); //forward slash key
Controller.captureKeyEvents({ text: "SHIFT"});
//Controller.captureKeyEvents({ text: "BACKSPACE"});
Controller.captureKeyEvents({ text: "SPACE"});
Controller.captureKeyEvents({"key":16777220} ); // enter key
Controller.captureKeyEvents({"key":16777219} ); // backspace key
Controller.keyPressEvent.connect(keyPressEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Script.update.connect(updateWriting);
