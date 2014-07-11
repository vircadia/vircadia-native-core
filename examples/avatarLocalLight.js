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

var localLightDirections = [ {x: 1.0, y:0.0, z: 0.0}, {x: 0.0, y:1.0, z: 1.0}, {x: 0.0, y:0.0, z: 1.0}, {x: 1.0, y:1.0, z: 1.0}  ];
var localLightColors = [ {x: 0.0, y:0.0, z: 0.0}, {x: 0.0, y:0.0, z: 0.0}, {x: 0.0, y:0.0, z: 0.0}, {x: 0.0, y:0.0, z: 0.0} ];

var currentSelection = 0;
var currentNumLights = 1;
var maxNumLights = 2;

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
    	localLightColors[currentSelection].x += 0.01;
    	if ( localLightColors[currentSelection].x > 1.0) {
    		localLightColors[currentSelection].x = 0.0;
    	}
    	
    	MyAvatar.setLocalLightColor(localLightColors[currentSelection], currentSelection);
    }
    else if (event.text == "6" ) {
    	localLightColors[currentSelection].y += 0.01;
    	if ( localLightColors[currentSelection].y > 1.0) {
    		localLightColors[currentSelection].y = 0.0;
    	}
    	
    	MyAvatar.setLocalLightColor(localLightColors[currentSelection], currentSelection);
    }
    else if (event.text == "7" ) {
    	localLightColors[currentSelection].z += 0.01;
    	if ( localLightColors[currentSelection].z > 1.0) {
    		localLightColors[currentSelection].z = 0.0;
    	}
    	
    	MyAvatar.setLocalLightColor(localLightColors[currentSelection], currentSelection);
    }
    else if (event.text == "8" ) {
    	localLightDirections[currentSelection].x += 0.01;
    	if (localLightDirections[currentSelection].x > 1.0) {
    		localLightDirections[currentSelection].x = -1.0;
    	}
    	
    	MyAvatar.setLocalLightDirection(localLightDirections[currentSelection], currentSelection);
    }
    else if (event.text == "9" ) {
    	localLightDirections[currentSelection].x -= 0.01;
    	if (localLightDirections[currentSelection].x < -1.0) {
    		localLightDirections[currentSelection].x = 1.0;
    	}
    	
    	MyAvatar.setLocalLightDirection(localLightDirections[currentSelection], currentSelection);
    }
    else if (event.text == "[" ) {
    	localLightDirections[currentSelection].y += 0.01;
    	if (localLightDirections[currentSelection].y > 1.0) {
    		localLightDirections[currentSelection].y = -1.0;
    	}
    	
    	MyAvatar.setLocalLightDirection(localLightDirections[currentSelection], currentSelection);
    }
    else if (event.text == "]" ) {
    	localLightDirections[currentSelection].y -= 0.01;
    	if (localLightDirections[currentSelection].y < -1.0) {
    		localLightDirections[currentSelection].y = 1.0;
    	}
    	
    	MyAvatar.setLocalLightDirection(localLightDirections[currentSelection], currentSelection);
    }
    else if (event.text == "," ) {
    	if (currentNumLights + 1 <= maxNumLights) {
            var darkGrayColor = {x:0.3, y:0.3, z:0.3};    

    		// default light
    		localLightColors[currentNumLights].x = darkGrayColor.x;
    		localLightColors[currentNumLights].y = darkGrayColor.y;
    		localLightColors[currentNumLights].z = darkGrayColor.z;
    	
            MyAvatar.addLocalLight(); 
    		MyAvatar.setLocalLightColor(localLightColors[currentNumLights], currentNumLights);
    		MyAvatar.setLocalLightDirection(localLightDirections[currentNumLights], currentNumLights);
	
    		++currentNumLights;
    	}
    }
    else if (event.text == "." ) {
    	if (currentNumLights - 1 >= 0 ) {
    	
    		// no light contribution
    		localLightColors[currentNumLights - 1].x = 0.0;
    		localLightColors[currentNumLights - 1].y = 0.0;
    		localLightColors[currentNumLights - 1].z = 0.0;
    
            MyAvatar.removeLocalLight();	
    		--currentNumLights;	
    	}
    }   
}

Controller.keyPressEvent.connect(keyPressEvent);
