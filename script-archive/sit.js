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


var buttonImageUrl = "https://s3.amazonaws.com/hifi-public/images/tools/sit.svg";

var windowDimensions = Controller.getViewportDimensions();

var buttonWidth = 37;
var buttonHeight = 46;
var buttonPadding = 10;

var buttonPositionX = windowDimensions.x - buttonPadding - buttonWidth;
var buttonPositionY = (windowDimensions.y - buttonHeight) / 2 - (buttonHeight + buttonPadding);

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
var startRotation = null;
var animationLenght = 2.0;

var avatarOldPosition = { x: 0, y: 0, z: 0 };

var sittingSettingsHandle = "SitJsSittingPosition";
var sitting = Settings.getValue(sittingSettingsHandle, false) == "true";
print("Original sitting status: " + sitting);
var frame = 0;

var seat = new Object();
var hiddingSeats = false;

// This is the pose we would like to end up
var pose = [
    {joint:"RightUpLeg", rotation: {x:100.0, y:15.0, z:0.0}},
    {joint:"RightLeg", rotation: {x:-130.0, y:15.0, z:0.0}},
    {joint:"RightFoot", rotation: {x:30, y:15.0, z:0.0}},
    {joint:"LeftUpLeg", rotation: {x:100.0, y:-15.0, z:0.0}},
    {joint:"LeftLeg", rotation: {x:-130.0, y:-15.0, z:0.0}},
    {joint:"LeftFoot", rotation: {x:30, y:15.0, z:0.0}}
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
        MyAvatar.setJointRotation(startPoseAndTransition[i].joint, Quat.fromVec3Degrees( rotation ));
    }
}

var sittingDownAnimation = function(deltaTime) {

    passedTime += deltaTime;
    var factor = passedTime/animationLenght;
    
    if ( passedTime <= animationLenght  ) {
        updateJoints(factor);

        var pos = { x: startPosition.x - 0.3 * factor, y: startPosition.y - 0.5 * factor, z: startPosition.z};
        MyAvatar.position = pos;
    } else {
        Script.update.disconnect(sittingDownAnimation);
        if (seat.model) {
            MyAvatar.setModelReferential(seat.model.id);
        }
    }
}

var standingUpAnimation = function(deltaTime) {

    passedTime += deltaTime;
    var factor = 1 - passedTime/animationLenght;

    if ( passedTime <= animationLenght  ) {
        
        updateJoints(factor);

        var pos = { x: startPosition.x + 0.3 * (passedTime/animationLenght), y: startPosition.y + 0.5 * (passedTime/animationLenght), z: startPosition.z};
        MyAvatar.position = pos;
    } else {
        Script.update.disconnect(standingUpAnimation);
        
    }
}

var goToSeatAnimation = function(deltaTime) {
    passedTime += deltaTime;
    var factor = passedTime/animationLenght;
    
    if (passedTime <= animationLenght) {
        var targetPosition = Vec3.sum(seat.position, { x: 0.3, y: 0.5, z: 0 });
        MyAvatar.position = Vec3.sum(Vec3.multiply(startPosition, 1 - factor), Vec3.multiply(targetPosition, factor));
    } else if (passedTime <= 2 * animationLenght) {
        Quat.print("MyAvatar: ", MyAvatar.orientation);
        Quat.print("Seat: ", seat.rotation);
        MyAvatar.orientation = Quat.mix(startRotation, seat.rotation, factor - 1);
    } else {
        Script.update.disconnect(goToSeatAnimation);
        sitDown();
        showIndicators(false);
    }
}

function sitDown() {
    sitting = true;
    Settings.setValue(sittingSettingsHandle, sitting);
    print("sitDown sitting status: " + Settings.getValue(sittingSettingsHandle, false));
    passedTime = 0.0;
    startPosition = MyAvatar.position;
    storeStartPoseAndTransition();
    try {
        Script.update.disconnect(standingUpAnimation);
    } catch(e){
            // no need to handle. if it wasn't connected no harm done
    }
    Script.update.connect(sittingDownAnimation);
    Overlays.editOverlay(sitDownButton, { visible: false });
    Overlays.editOverlay(standUpButton, { visible: true });
}

function standUp() {
    sitting = false;
    Settings.setValue(sittingSettingsHandle, sitting);
    print("standUp sitting status: " + Settings.getValue(sittingSettingsHandle, false));
    passedTime = 0.0;
    startPosition = MyAvatar.position;
    MyAvatar.clearReferential();
    try{
        Script.update.disconnect(sittingDownAnimation);
    } catch (e){}
    Script.update.connect(standingUpAnimation);
    Overlays.editOverlay(standUpButton, { visible: false });
    Overlays.editOverlay(sitDownButton, { visible: true });
}

var models = new Object();
function SeatIndicator(modelProperties, seatIndex) {
    this.position =  Vec3.sum(modelProperties.position,
                              Vec3.multiply(Vec3.multiplyQbyV(modelProperties.rotation,
                                                              modelProperties.sittingPoints[seatIndex].position),
                                            modelProperties.radius));
                              
    this.orientation = Quat.multiply(modelProperties.rotation,
                                     modelProperties.sittingPoints[seatIndex].rotation);
    this.scale = MyAvatar.scale / 12;
    
    this.sphere = Overlays.addOverlay("image3d", {
                                      subImage: { x: 0, y: buttonHeight, width: buttonWidth, height: buttonHeight},
                                      url: buttonImageUrl,
                                       position: this.position,
                                      scale: this.scale * 4,
                                      solid: true,
                                      color: { red: 0, green: 0, blue: 255 },
                                      alpha: 0.3,
                                      visible: true
                                      });
    
    this.show = function(doShow) {
        Overlays.editOverlay(this.sphere, { visible: doShow });
    }
    
    this.update = function() {
        Overlays.editOverlay(this.sphere, {
                             position: this.position,
                             size: this.scale
                             });
    }
    
    this.cleanup = function() {
        Overlays.deleteOverlay(this.sphere);
    }
}

Controller.mousePressEvent.connect(function(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

    if (clickedOverlay == sitDownButton) {
        seat.model = null;
        sitDown();
    } else if (clickedOverlay == standUpButton) {
        seat.model = null;
        standUp();
    } else {
       var pickRay = Camera.computePickRay(event.x, event.y);
                                   
       var clickedOnSeat = false;
       
       for (index in models) {
           var model = models[index];
           
           for (var i = 0; i < model.properties.sittingPoints.length; ++i) {
               if (raySphereIntersection(pickRay.origin,
                                         pickRay.direction,
                                         model.properties.sittingPoints[i].indicator.position,
                                         model.properties.sittingPoints[i].indicator.scale / 2)) {
                   clickedOnSeat = true;
                   seat.model = model;
                   seat.position = model.properties.sittingPoints[i].indicator.position;
                   seat.rotation = model.properties.sittingPoints[i].indicator.orientation;
               }
           }
       }
       if (clickedOnSeat) {
           passedTime = 0.0;
           startPosition = MyAvatar.position;
           startRotation = MyAvatar.orientation;
           try{ Script.update.disconnect(standingUpAnimation); } catch(e){}
           try{ Script.update.disconnect(sittingDownAnimation); } catch(e){}
           Script.update.connect(goToSeatAnimation);
       }
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
    
    // For a weird reason avatar joint don't update till the 10th frame
    // Set the update frame to 20 to be safe
    var UPDATE_FRAME = 20;
    if (frame <= UPDATE_FRAME) {
        if (frame == UPDATE_FRAME) {
            if (sitting == true) {
                print("Was seated: " + sitting);
                storeStartPoseAndTransition();
                updateJoints(1.0);
                Overlays.editOverlay(sitDownButton, { visible: false });
                Overlays.editOverlay(standUpButton, { visible: true });
            }
        }
        frame++;
    }

    var locationChanged = false;
    if (location.hostname != oldHost || !location.isConnected) {
        for (model in models) {
            removeIndicators(models[model]);
        }
        oldHost = location.hostname;
        locationChanged = true;
    }
    
    if (MyAvatar.position.x != avatarOldPosition.x ||
        MyAvatar.position.y != avatarOldPosition.y ||
        MyAvatar.position.z != avatarOldPosition.z ||
        locationChanged) {
        avatarOldPosition = MyAvatar.position;

        /*
        var SEARCH_RADIUS = 50;
        var foundModels = Entities.findEntities(MyAvatar.position, SEARCH_RADIUS);
        // Let's remove indicator that got out of radius
        for (model in models) {
            if (Vec3.distance(models[model].properties.position, MyAvatar.position) > SEARCH_RADIUS) {
                removeIndicators(models[model]);
            }
        }
        
        // Let's add indicators to new seats in radius
        for (var i = 0; i < foundModels.length; ++i) {
            var model = foundModels[i];
            if (typeof(models[model.id]) == "undefined") {
                model.properties = Entities.getEntityProperties(model);
                if (Vec3.distance(model.properties.position, MyAvatar.position) < SEARCH_RADIUS) {
                    addIndicators(model);
                }
            }
        }
        
        if (hiddingSeats && passedTime >= animationLenght) {
            showIndicators(true);
        }
        */
    }
}
var oldHost = location.hostname;

function addIndicators(modelID) {
    if (modelID.properties.sittingPoints.length > 0) {
        for (var i = 0; i < modelID.properties.sittingPoints.length; ++i) {
            modelID.properties.sittingPoints[i].indicator = new SeatIndicator(modelID.properties, i);
        }
        
        models[modelID] = modelID;
    }
}

function removeIndicators(modelID) {
    for (var i = 0; i < modelID.properties.sittingPoints.length; ++i) {
        modelID.properties.sittingPoints[i].indicator.cleanup();
    }
    delete models[modelID];
}

function showIndicators(doShow) {
    for (model in models) {
        var modelID = models[model];
        for (var i = 0; i < modelID.properties.sittingPoints.length; ++i) {
            modelID.properties.sittingPoints[i].indicator.show(doShow);
        }
    }
    hiddingSeats = !doShow;
}

function raySphereIntersection(origin, direction, center, radius) {
    var A = origin;
    var B = Vec3.normalize(direction);
    var P = center;
    
    var x = Vec3.dot(Vec3.subtract(P, A), B);
    var X = Vec3.sum(A, Vec3.multiply(B, x));
    var d = Vec3.length(Vec3.subtract(P, X));
    
    return (x > 0 && d <= radius);
}

function keyPressEvent(event) {
    if (event.text === ".") {
        if (sitting) {
            standUp();
        } else {
            sitDown();
        }
    }
}


Script.update.connect(update);
Controller.keyPressEvent.connect(keyPressEvent);

Script.scriptEnding.connect(function() {
    MyAvatar.clearReferential();
    for (var i = 0; i < pose.length; i++){
            MyAvatar.clearJointData(pose[i].joint);
    }

    Overlays.deleteOverlay(sitDownButton);
    Overlays.deleteOverlay(standUpButton);
    for (model in models){
        for (var i = 0; i < models[model].properties.sittingPoints.length; ++i) {
            models[model].properties.sittingPoints[i].indicator.cleanup();
        }
    }
});
