//
//  sitOnEntity.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example of an entity script for sitting.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){ 

    this.entityID = null;
    this.properties = null;
    this.standUpButton = null;
    this.indicatorsAdded = false;
    this.indicator = new Array();
    

    var buttonImageUrl = "https://worklist-prod.s3.amazonaws.com/attachment/0aca88e1-9bd8-5c1d.svg";
    var windowDimensions = Controller.getViewportDimensions();
    var buttonWidth = 37;
    var buttonHeight = 46;
    var buttonPadding = 50; // inside the normal toolbar position
    var buttonPositionX = windowDimensions.x - buttonPadding - buttonWidth;
    var buttonPositionY = (windowDimensions.y - buttonHeight) / 2 - (buttonHeight + buttonPadding);

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

    var externalThis = this;
    
    var goToSeatAnimation = function(deltaTime) {
        passedTime += deltaTime;
        var factor = passedTime/animationLenght;
    
        if (passedTime <= animationLenght) {
            var targetPosition = Vec3.sum(seat.position, { x: 0.3, y: 0.5, z: 0 });
            MyAvatar.position = Vec3.sum(Vec3.multiply(startPosition, 1 - factor), Vec3.multiply(targetPosition, factor));
        } else if (passedTime <= 2 * animationLenght) {
            //Quat.print("MyAvatar: ", MyAvatar.orientation);
            //Quat.print("Seat: ", seat.rotation);
            MyAvatar.orientation = Quat.mix(startRotation, seat.rotation, factor - 1);
        } else {
            Script.update.disconnect(goToSeatAnimation);
            externalThis.sitDown();
            externalThis.showIndicators(false);
        }
    }
    
    var globalMouseClick = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
        if (clickedOverlay == externalThis.standUpButton) {
            seat.model = null;
            externalThis.standUp();
            Controller.mousePressEvent.disconnect(globalMouseClick);
        }
    };

    this.sitDown = function() {
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
        Overlays.editOverlay(this.standUpButton, { visible: true });
        Controller.mousePressEvent.connect(globalMouseClick);
    }

    this.standUp = function() {
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
        Overlays.editOverlay(this.standUpButton, { visible: false });
        Controller.mousePressEvent.disconnect(globalMouseClick);
    }

    function SeatIndicator(modelProperties, seatIndex) {
        var halfDiagonal = Vec3.length(modelProperties.dimensions) / 2.0;

        this.position =  Vec3.sum(modelProperties.position,
                                  Vec3.multiply(Vec3.multiplyQbyV(modelProperties.rotation, modelProperties.sittingPoints[seatIndex].position),
                                                halfDiagonal)); // hack
                              
        this.orientation = Quat.multiply(modelProperties.rotation,
                                         modelProperties.sittingPoints[seatIndex].rotation);
        this.scale = MyAvatar.scale / 3;
    
        this.sphere = Overlays.addOverlay("image3d", {
                                          subImage: { x: 0, y: buttonHeight, width: buttonWidth, height: buttonHeight},
                                          url: buttonImageUrl,
                                          position: this.position,
                                          scale: this.scale,
                                          size: this.scale,
                                          solid: true,
                                          color: { red: 255, green: 255, blue: 255 },
                                          alpha: 0.8,
                                          visible: true,
                                          isFacingAvatar: true
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

    function update(deltaTime){
        var newWindowDimensions = Controller.getViewportDimensions();
        if( newWindowDimensions.x != windowDimensions.x || newWindowDimensions.y != windowDimensions.y ){
            windowDimensions = newWindowDimensions;
            var newX = windowDimensions.x - buttonPadding - buttonWidth;
            var newY = (windowDimensions.y - buttonHeight) / 2 ;
            Overlays.editOverlay( this.standUpButton, {x: newX, y: newY} );
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
                }
            }
            frame++;
        }
    }

    this.addIndicators = function() {
        if (!this.indicatorsAdded) {
            if (this.properties.sittingPoints.length > 0) {
                for (var i = 0; i < this.properties.sittingPoints.length; ++i) {
                    this.indicator[i] = new SeatIndicator(this.properties, i);
                }
                this.indicatorsAdded = true;
            }
        }
    }

    this.removeIndicators = function() {
        for (var i = 0; i < this.properties.sittingPoints.length; ++i) {
            this.indicator[i].cleanup();
        }
    }

    this.showIndicators = function(doShow) {
        this.addIndicators();
        if (this.indicatorsAdded) {
            for (var i = 0; i < this.properties.sittingPoints.length; ++i) {
                this.indicator[i].show(doShow);
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

    this.cleanup = function() {
        this.standUp();
        MyAvatar.clearReferential();
        for (var i = 0; i < pose.length; i++){
            MyAvatar.clearJointData(pose[i].joint);
        }
        Overlays.deleteOverlay(this.standUpButton);
        for (var i = 0; i < this.indicator.length; ++i) {
            this.indicator[i].cleanup();
        }
    };
    
    
    this.createStandupButton = function() {
        this.standUpButton = Overlays.addOverlay("image", {
                                                 x: buttonPositionX, y: buttonPositionY, width: buttonWidth, height: buttonHeight,
                                                 subImage: { x: buttonWidth, y: buttonHeight, width: buttonWidth, height: buttonHeight},
                                                 imageURL: buttonImageUrl,
                                                 visible: false,
                                                 alpha: 1.0
                                                 });
    };

    this.handleClickEvent = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

        if (clickedOverlay == this.standUpButton) {
            seat.model = null;
            this.standUp();
        } else {
            this.addIndicators();
            if (this.indicatorsAdded) {
                var pickRay = Camera.computePickRay(event.x, event.y);
                       
                var clickedOnSeat = false;

                for (var i = 0; i < this.properties.sittingPoints.length; ++i) {
                   if (raySphereIntersection(pickRay.origin,
                                             pickRay.direction,
                                             this.indicator[i].position,
                                             this.indicator[i].scale / 2)) {
                       clickedOnSeat = true;
                       seat.model = this.entityID;
                       seat.position = this.indicator[i].position;
                       seat.rotation = this.indicator[i].orientation;
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
        }
    };


    // All callbacks start by updating the properties
    this.updateProperties = function(entityID) {
        if (this.entityID === null) {
            this.entityID = entityID;
        }
        this.properties = Entities.getEntityProperties(this.entityID);
    };

    this.unload = function(entityID) {
        this.cleanup();
        Script.update.disconnect(update);
    };
    
    this.preload = function(entityID) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.createStandupButton();
        Script.update.connect(update);
    };


    this.hoverOverEntity = function(entityID, mouseEvent) { 
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.showIndicators(true);
    }; 
    this.hoverLeaveEntity = function(entityID, mouseEvent) { 
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.showIndicators(false);
    }; 
    
    this.clickDownOnEntity = function(entityID, mouseEvent) { 
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.handleClickEvent(mouseEvent);
    }; 

    this.clickReleaseOnEntity = function(entityID, mouseEvent) { 
        this.updateProperties(entityID); // All callbacks start by updating the properties
    }; 
    
})
