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

var localLightDirections = [ {x: 1.0, y:1.0, z: 0.0}, {x: 0.0, y:1.0, z: 1.0} ];
var localLightColors = [ {x: 0.0, y:1.0, z: 0.0}, {x: 1.0, y:0.0, z: 0.0} ];

var currentSelection = 0;
var currentNumLights = 1;
var maxNumLights = 2;
var currentNumAvatars = 0;
var changeDelta = 0.1;

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
    	
    	setAllLightColors();
    	print("CHANGE RED light " + currentSelection + " color (" + localLightColors[currentSelection].x + ", " + localLightColors[currentSelection].y + ", " + localLightColors[currentSelection].z + " )" );
    }
    else if (event.text == "6" ) {
    	localLightColors[currentSelection].y += changeDelta;
    	if ( localLightColors[currentSelection].y > 1.0) {
    		localLightColors[currentSelection].y = 0.0;
    	}
    	
    	setAllLightColors();
    	print("CHANGE GREEN light " + currentSelection + " color (" + localLightColors[currentSelection].x + ", " + localLightColors[currentSelection].y + ", " + localLightColors[currentSelection].z + " )" );
    }
    else if (event.text == "7" ) {
    	localLightColors[currentSelection].z += changeDelta;
    	if ( localLightColors[currentSelection].z > 1.0) {
    		localLightColors[currentSelection].z = 0.0;
    	}
    	
    	setAllLightColors();
    	print("CHANGE BLUE light " + currentSelection + " color (" + localLightColors[currentSelection].x + ", " + localLightColors[currentSelection].y + ", " + localLightColors[currentSelection].z + " )" );
    }
    else if (event.text == "8" ) {
    	localLightDirections[currentSelection].x += changeDelta;
    	if (localLightDirections[currentSelection].x > 1.0) {
    		localLightDirections[currentSelection].x = -1.0;
    	}
    	
    	setAllLightDirections();
    	print("PLUS X light " + currentSelection + " direction (" + localLightDirections[currentSelection].x + ", " + localLightDirections[currentSelection].y + ", " + localLightDirections[currentSelection].z + " )" );
    }
    else if (event.text == "9" ) {
    	localLightDirections[currentSelection].x -= changeDelta;
    	if (localLightDirections[currentSelection].x < -1.0) {
    		localLightDirections[currentSelection].x = 1.0;
    	}
    	
    	setAllLightDirections();
    	print("MINUS X light " + currentSelection + " direction (" + localLightDirections[currentSelection].x + ", " + localLightDirections[currentSelection].y + ", " + localLightDirections[currentSelection].z + " )" );
    }
    else if (event.text == "0" ) {
    	localLightDirections[currentSelection].y += changeDelta;
    	if (localLightDirections[currentSelection].y > 1.0) {
    		localLightDirections[currentSelection].y = -1.0;
    	}
    	
    	setAllLightDirections();
    	print("PLUS Y light " + currentSelection + " direction (" + localLightDirections[currentSelection].x + ", " + localLightDirections[currentSelection].y + ", " + localLightDirections[currentSelection].z + " )" );
    }
    else if (event.text == "-" ) {
    	localLightDirections[currentSelection].y -= changeDelta;
    	if (localLightDirections[currentSelection].y < -1.0) {
    		localLightDirections[currentSelection].y = 1.0;
    	}
    	
    	setAllLightDirections();
    	print("MINUS Y light " + currentSelection + " direction (" + localLightDirections[currentSelection].x + ", " + localLightDirections[currentSelection].y + ", " + localLightDirections[currentSelection].z + " )" );
    }
    else if (event.text == "," ) {
    	if (currentNumLights + 1 <= maxNumLights) {
    		++currentNumLights;
    		
    		for (var i = 0; i < currentNumAvatars; i++) {
    			AvatarManager.addAvatarLocalLight(i);
    		
    			for (var j = 0; j < currentNumLights; j++) {
    				AvatarManager.setAvatarLightColor(localLightColors[j], j, i); 
					AvatarManager.setAvatarLightDirection(localLightDirections[j], j, i);
    			}
    		}
    	}
    	
    	print("ADD LIGHT, number of lights " + currentNumLights);
    }
    else if (event.text == "." ) {
    	if (currentNumLights - 1 >= 0 ) {
    		--currentNumLights;	
    		
    		for (var i = 0; i < currentNumAvatars; i++) {
    			AvatarManager.removeAvatarLocalLight(i);
    		
    			for (var j = 0; j < currentNumLights; j++) {
    				AvatarManager.setAvatarLightColor(localLightColors[j], j, i); 
					AvatarManager.setAvatarLightDirection(localLightDirections[j], j, i);
    			}
    		}
    	
    	}
    	
    	print("REMOVE LIGHT, number of lights " + currentNumLights);
    }   
}

function updateLocalLights()
{
	// new avatars, so add lights
	var numAvatars = AvatarManager.getNumAvatars();
	if (numAvatars != currentNumAvatars) {
			
		for (var i = 0; i < numAvatars; i++) {
			var numLights = AvatarManager.getNumLightsInAvatar(i);
			
			// check if new avatar has lights
			if (numLights <= 0) {
				AvatarManager.addAvatarLocalLight(i);
				
				// set color and direction for new avatar
				for (var j = 0; j < maxNumLights; j++) {
					AvatarManager.setAvatarLightColor(localLightColors[j], j, i); 
					AvatarManager.setAvatarLightDirection(localLightDirections[j], j, i);
				}
			}
		}
		
		currentNumAvatars = numAvatars;
	}
}

function setAllLightColors()
{
	for (var i = 0; i < currentNumAvatars; i++) {
		for (var j = 0; j < maxNumLights; j++) {
			AvatarManager.setAvatarLightColor(localLightColors[j], j, i); 
		}
	}
}

function setAllLightDirections()
{
	for (var i = 0; i < currentNumAvatars; i++) {
		for (var j = 0; j < maxNumLights; j++) {
			AvatarManager.setAvatarLightDirection(localLightDirections[j], j, i); 
		}
	}
}

// main
Script.update.connect(updateLocalLights);
Controller.keyPressEvent.connect(keyPressEvent);

