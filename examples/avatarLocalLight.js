//
//  avatarLocalLight.js
//
//  Created by Tony Peng on July 2nd, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Set the local light direction and color on the avatar
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var localLightDirections = [ {x: 1.0, y:0.0, z: 0.0}, {x: 0.0, y:0.0, z: 1.0} ];
var localLightColors = [ {x: 1.0, y:1.0, z: 1.0}, {x: 1.0, y:1.0, z: 1.0} ];

var currentSelection = 0;
var currentNumLights = 2;
var maxNumLights = 2;
var currentNumAvatars = 0;
var changeDelta = 0.1;
var lightsDirty = true;

function keyPressEvent(event) {

    var choice = parseInt(event.text);
    
    if (event.text == "1") {
        currentSelection = 0;
        print("light election = " + currentSelection);
    }
    else if (event.text == "2" ) {
        currentSelection = 1;
        print("light selection = " + currentSelection);
    }
    else if (event.text == "3" ) {
        currentSelection = 2;
        print("light selection = " + currentSelection);
    }
    else if (event.text == "4" ) {
        currentSelection = 3;
        print("light selection = " + currentSelection);
    }
    else if (event.text == "5" ) {
        localLightColors[currentSelection].x += changeDelta;
        if ( localLightColors[currentSelection].x > 1.0) {
            localLightColors[currentSelection].x = 0.0;
        }
        
        lightsDirty = true;
        print("CHANGE RED light " + currentSelection + " color (" + localLightColors[currentSelection].x + ", " + localLightColors[currentSelection].y + ", " + localLightColors[currentSelection].z + " )" );
    }
    else if (event.text == "6" ) {
        localLightColors[currentSelection].y += changeDelta;
        if ( localLightColors[currentSelection].y > 1.0) {
            localLightColors[currentSelection].y = 0.0;
        }
        
        lightsDirty = true;
        print("CHANGE GREEN light " + currentSelection + " color (" + localLightColors[currentSelection].x + ", " + localLightColors[currentSelection].y + ", " + localLightColors[currentSelection].z + " )" );
    }
    else if (event.text == "7" ) {
        localLightColors[currentSelection].z += changeDelta;
        if ( localLightColors[currentSelection].z > 1.0) {
            localLightColors[currentSelection].z = 0.0;
        }
        
        lightsDirty = true;
        print("CHANGE BLUE light " + currentSelection + " color (" + localLightColors[currentSelection].x + ", " + localLightColors[currentSelection].y + ", " + localLightColors[currentSelection].z + " )" );
    }
    else if (event.text == "8" ) {
        localLightDirections[currentSelection].x += changeDelta;
        if (localLightDirections[currentSelection].x > 1.0) {
            localLightDirections[currentSelection].x = -1.0;
        }
        
        lightsDirty = true;
        print("PLUS X light " + currentSelection + " direction (" + localLightDirections[currentSelection].x + ", " + localLightDirections[currentSelection].y + ", " + localLightDirections[currentSelection].z + " )" );
    }
    else if (event.text == "9" ) {
        localLightDirections[currentSelection].x -= changeDelta;
        if (localLightDirections[currentSelection].x < -1.0) {
            localLightDirections[currentSelection].x = 1.0;
        }
        
        lightsDirty = true;
        print("MINUS X light " + currentSelection + " direction (" + localLightDirections[currentSelection].x + ", " + localLightDirections[currentSelection].y + ", " + localLightDirections[currentSelection].z + " )" );
    }
    else if (event.text == "0" ) {
        localLightDirections[currentSelection].y += changeDelta;
        if (localLightDirections[currentSelection].y > 1.0) {
            localLightDirections[currentSelection].y = -1.0;
        }
        
        lightsDirty = true;
        print("PLUS Y light " + currentSelection + " direction (" + localLightDirections[currentSelection].x + ", " + localLightDirections[currentSelection].y + ", " + localLightDirections[currentSelection].z + " )" );
    }
    else if (event.text == "-" ) {
        localLightDirections[currentSelection].y -= changeDelta;
        if (localLightDirections[currentSelection].y < -1.0) {
            localLightDirections[currentSelection].y = 1.0;
        }
        
        lightsDirty = true;
        print("MINUS Y light " + currentSelection + " direction (" + localLightDirections[currentSelection].x + ", " + localLightDirections[currentSelection].y + ", " + localLightDirections[currentSelection].z + " )" );
    }
    else if (event.text == "," ) {
        if (currentNumLights + 1 <= maxNumLights) {
            ++currentNumLights;
            lightsDirty = true;
        }
        
        print("ADD LIGHT, number of lights " + currentNumLights);
    }
    else if (event.text == "." ) {
        if (currentNumLights - 1 >= 0 ) {
            --currentNumLights;    
            lightsDirty = true;
        }
        
        print("REMOVE LIGHT, number of lights " + currentNumLights);
    }   
}

function updateLocalLights()
{
    if (lightsDirty) {
        var localLights = [];
        for (var i = 0; i < currentNumLights; i++) {
            localLights.push({ direction: localLightDirections[i], color: localLightColors[i] });
        }
        AvatarManager.setLocalLights(localLights);
        lightsDirty = false;
    }
}

// main
Script.update.connect(updateLocalLights);
Controller.keyPressEvent.connect(keyPressEvent);

