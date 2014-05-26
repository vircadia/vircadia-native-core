//
// sit.js
// examples
//
//  Created by Mika Impola on February 8, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var buttonImageUrl = "https://worklist-prod.s3.amazonaws.com/attachment/0aca88e1-9bd8-5c1d.svg";

var windowDimensions = Controller.getViewportDimensions();

var buttonWidth = 37;
var buttonHeight = 46;
var buttonPadding = 10;

var buttonPositionX = windowDimensions.x - buttonPadding - buttonWidth;
var buttonPositionY = (windowDimensions.y - buttonHeight) / 2 ;

var sitDownButton = Overlays.addOverlay("image", {
                                         x: buttonPositionX, y: buttonPositionY, width: buttonWidth, height: buttonHeight,
                                         subImage: { x: 0, y: buttonHeight, width: buttonWidth, height: buttonHeight},
                                         imageURL: buttonImageUrl,
                                         visible: true,
                                         alpha: 1.0
                                         });
var standUpButton = Overlays.addOverlay("image", {
                                         x: buttonPositionX, y: buttonPositionY, width: buttonWidth, height: buttonHeight,
                                         subImage: { x: buttonWidth, y: buttonHeight, width: buttonWidth, height: buttonHeight},
                                         imageURL: buttonImageUrl,
                                         visible: false,
                                         alpha: 1.0
                                         });

var passedTime = 0.0;
var startPosition = null;
var animationLenght = 2.0;

// This is the pose we would like to end up
var pose = [
	{joint:"RightUpLeg", rotation: {x:100.0, y:15.0, z:0.0}},
	{joint:"RightLeg", rotation: {x:-130.0, y:15.0, z:0.0}},
	{joint:"RightFoot", rotation: {x:30, y:15.0, z:0.0}},
	{joint:"LeftUpLeg", rotation: {x:100.0, y:-15.0, z:0.0}},
	{joint:"LeftLeg", rotation: {x:-130.0, y:-15.0, z:0.0}},
	{joint:"LeftFoot", rotation: {x:30, y:15.0, z:0.0}},

	{joint:"Spine2", rotation: {x:20, y:0.0, z:0.0}},
	
	{joint:"RightShoulder", rotation: {x:0.0, y:40.0, z:0.0}},
	{joint:"LeftShoulder", rotation: {x:0.0, y:-40.0, z:0.0}}

];

var startPoseAndTransition = [];

function storeStartPoseAndTransition() {
	for (var i = 0; i < pose.length; i++){
		var startRotation = Quat.safeEulerAngles(MyAvatar.getJointRotation(pose[i].joint));
		var transitionVector = Vec3.subtract( pose[i].rotation, startRotation );
		startPoseAndTransition.push({joint: pose[i].joint, start: startRotation, transition: transitionVector});
	}
}

function updateJoints(factor){
	for (var i = 0; i < startPoseAndTransition.length; i++){
		var scaledTransition = Vec3.multiply(startPoseAndTransition[i].transition, factor);
		var rotation = Vec3.sum(startPoseAndTransition[i].start, scaledTransition);
		MyAvatar.setJointData(startPoseAndTransition[i].joint, Quat.fromVec3Degrees( rotation ));
	}
}

var sittingDownAnimation = function(deltaTime) {

	passedTime += deltaTime;
	var factor = passedTime/animationLenght;
	
	if ( passedTime <= animationLenght  ) {
		updateJoints(factor);

		var pos = { x: startPosition.x - 0.3 * factor, y: startPosition.y - 0.5 * factor, z: startPosition.z};
		MyAvatar.position = pos;
	}
}

var standingUpAnimation = function(deltaTime){

	passedTime += deltaTime;
	var factor = 1 - passedTime/animationLenght;

	if ( passedTime <= animationLenght  ) {
		
		updateJoints(factor);

		var pos = { x: startPosition.x + 0.3 * (passedTime/animationLenght), y: startPosition.y + 0.5 * (passedTime/animationLenght), z: startPosition.z};
		MyAvatar.position = pos;
	}
}

Controller.mousePressEvent.connect(function(event){

	var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

	if (clickedOverlay == sitDownButton) {
		passedTime = 0.0;
		startPosition = MyAvatar.position;
		storeStartPoseAndTransition();
		try{
			Script.update.disconnect(standingUpAnimation);
		} catch(e){
			// no need to handle. if it wasn't connected no harm done
		}
		Script.update.connect(sittingDownAnimation);
		Overlays.editOverlay(sitDownButton, { visible: false });
		Overlays.editOverlay(standUpButton, { visible: true });
	} else if (clickedOverlay == standUpButton) {
		passedTime = 0.0;
		startPosition = MyAvatar.position;
		try{
			Script.update.disconnect(sittingDownAnimation);
		} catch (e){}
		Script.update.connect(standingUpAnimation);
		Overlays.editOverlay(standUpButton, { visible: false });
		Overlays.editOverlay(sitDownButton, { visible: true });
	}
})

function update(deltaTime){
	var newWindowDimensions = Controller.getViewportDimensions();
	if( newWindowDimensions.x != windowDimensions.x || newWindowDimensions.y != windowDimensions.y ){
		windowDimensions = newWindowDimensions;
		var newX = windowDimensions.x - buttonPadding - buttonWidth;
		var newY = (windowDimensions.y - buttonHeight) / 2 ;
		Overlays.editOverlay( standUpButton, {x: newX, y: newY} );
		Overlays.editOverlay( sitDownButton, {x: newX, y: newY} );
	}		
}

Script.update.connect(update);

Script.scriptEnding.connect(function() {

	for (var i = 0; i < pose.length; i++){
		    MyAvatar.clearJointData(pose[i][0]);
	}		

	Overlays.deleteOverlay(sitDownButton);
	Overlays.deleteOverlay(standUpButton);
});
