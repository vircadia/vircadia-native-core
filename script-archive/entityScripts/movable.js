//
//  movable.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function(){

    this.entityID = null;
    this.properties = null;
    this.graboffset = null;
    this.clickedAt = null;
    this.firstHolding = true;
    this.clicked = { x: -1, y: -1};
    this.lastMovedPosition = { x: -1, y: -1};
    this.lastMovedTime = 0;
    this.rotateOverlayTarget = null;
    this.rotateOverlayInner = null;
    this.rotateOverlayOuter = null;
    this.rotateOverlayCurrent = null;
    this.rotateMode = false;
    this.originalRotation = null;

    this.moveSoundURLS = [
        "http://public.highfidelity.io/sounds/MovingFurniture/FurnitureMove1.wav",
        "http://public.highfidelity.io/sounds/MovingFurniture/FurnitureMove2.wav",
        "http://public.highfidelity.io/sounds/MovingFurniture/FurnitureMove3.wav"
    ];

    this.turnSoundURLS = [

        "http://public.highfidelity.io/sounds/MovingFurniture/FurnitureMove1.wav",
        "http://public.highfidelity.io/sounds/MovingFurniture/FurnitureMove2.wav",
        "http://public.highfidelity.io/sounds/MovingFurniture/FurnitureMove3.wav"

        // TODO: determine if these or other turn sounds work better than move sounds.
        //"http://public.highfidelity.io/sounds/MovingFurniture/FurnitureTurn1.wav",
        //"http://public.highfidelity.io/sounds/MovingFurniture/FurnitureTurn2.wav",
        //"http://public.highfidelity.io/sounds/MovingFurniture/FurnitureTurn3.wav"
    ];


    this.moveSounds = new Array();
    this.turnSounds = new Array();
    this.moveSound = null;
    this.turnSound = null;
    this.moveInjector = null;
    this.turnInjector = null;

    var debug = false;
    var displayRotateTargets = true; // change to false if you don't want the rotate targets
    var rotateOverlayTargetSize = 10000; // really big target
    var innerSnapAngle = 22.5; // the angle which we snap to on the inner rotation tool
    var innerRadius;
    var outerRadius;
    var yawCenter;
    var yawZero;
    var rotationNormal;
    var yawNormal;
    var stopSoundDelay = 100; // number of msecs of not moving to have sound stop

    this.getRandomInt = function(min, max) {
        return Math.floor(Math.random() * (max - min + 1)) + min;
    }

    this.downloadSounds = function() {
        for (var i = 0; i < this.moveSoundURLS.length; i++) {
            this.moveSounds[i] = SoundCache.getSound(this.moveSoundURLS[i]);
        }
        for (var i = 0; i < this.turnSoundURLS.length; i++) {
            this.turnSounds[i] = SoundCache.getSound(this.turnSoundURLS[i]);
        }
    }

    this.pickRandomSounds = function() {
        var moveIndex = this.getRandomInt(0, this.moveSounds.length - 1);
        var turnIndex = this.getRandomInt(0, this.turnSounds.length - 1);
        if (debug) {
            print("Random sounds -- turn:" + turnIndex + " move:" + moveIndex);
        }
        this.moveSound = this.moveSounds[moveIndex];
        this.turnSound = this.turnSounds[turnIndex];
    }

    // Play move sound
    this.playMoveSound = function() {
        if (debug) {
            print("playMoveSound()");
        }
        if (this.moveSound && this.moveSound.downloaded) {
            if (debug) {
                print("playMoveSound() --- calling this.moveInjector = Audio.playSound(this.moveSound...)");
            }

            if (!this.moveInjector) {
                this.moveInjector = Audio.playSound(this.moveSound, { position: this.properties.position, loop: true, volume: 0.1 });
            } else {
                this.moveInjector.restart();
            }
        }
    }

    // Play turn sound
    this.playTurnSound = function() {
        if (debug) {
            print("playTurnSound()");
        }
        if (this.turnSound && this.turnSound.downloaded) {
            if (debug) {
                print("playTurnSound() --- calling this.turnInjector = Audio.playSound(this.turnSound...)");
            }
            if (!this.turnInjector) {
                this.turnInjector = Audio.playSound(this.turnSound, { position: this.properties.position, loop: true, volume: 0.1 });
            } else {
                this.turnInjector.restart();
            }
        }
    }

    // stop sound
    this.stopSound = function() {
        if (debug) {
            print("stopSound()");
        }
        if (this.turnInjector) {
            this.turnInjector.stop();
        }
        if (this.moveInjector) {
            this.moveInjector.stop();
        }
    }

    // Pr, Vr are respectively the Ray's Point of origin and Vector director
    // Pp, Np are respectively the Plane's Point of origin and Normal vector
    this.rayPlaneIntersection = function(Pr, Vr, Pp, Np) {
        var d = -Vec3.dot(Pp, Np);
        var t = -(Vec3.dot(Pr, Np) + d) / Vec3.dot(Vr, Np);
        return Vec3.sum(Pr, Vec3.multiply(t, Vr));
    };

    // updates the piece position based on mouse input
    this.updatePosition = function(mouseEvent) {
        var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
        var upVector = { x: 0, y: 1, z: 0 };
        var intersection = this.rayPlaneIntersection(pickRay.origin, pickRay.direction,
                                                     this.properties.position, upVector);

        var newPosition = Vec3.sum(intersection, this.graboffset);
        Entities.editEntity(this.entityID, { position: newPosition });
    };

    this.grab = function(mouseEvent) {
        // first calculate the offset
        var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
        var upVector = { x: 0, y: 1, z: 0 };
        var intersection = this.rayPlaneIntersection(pickRay.origin, pickRay.direction,
                                                     this.properties.position, upVector);
        this.graboffset = Vec3.subtract(this.properties.position, intersection);
    };

    this.stopSoundIfNotMoving = function(mouseEvent) {
        var nowDate = new Date();
        var nowMSecs = nowDate.getTime();
        if(mouseEvent.x == this.lastMovedPosition.x && mouseEvent.y == this.lastMovedPosition.y) {
            var elapsedSinceLastMove = nowMSecs - this.lastMovedMSecs;
            if (debug) {
                print("elapsedSinceLastMove:" + elapsedSinceLastMove);
            }
            if (elapsedSinceLastMove > stopSoundDelay) {
                if (debug) {
                    print("calling stopSound()...");
                }
                this.stopSound();
            }
        } else {
            // if we've moved, then track our last move position and time...
            this.lastMovedMSecs = nowMSecs;
            this.lastMovedPosition.x = mouseEvent.x;
            this.lastMovedPosition.y = mouseEvent.y;
        }
    }

    this.move = function(mouseEvent) {
        this.updatePosition(mouseEvent);
        if (this.moveInjector === null || !this.moveInjector.playing) {
            this.playMoveSound();
        }
    };

    this.release = function(mouseEvent) {
        this.updatePosition(mouseEvent);
    };

    this.rotate = function(mouseEvent) {
        var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
        var result = Overlays.findRayIntersection(pickRay);

        if (result.intersects) {
            var center = yawCenter;
            var zero = yawZero;
            var centerToZero = Vec3.subtract(center, zero);
            var centerToIntersect = Vec3.subtract(center, result.intersection);
            var angleFromZero = Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal);

            var distanceFromCenter = Vec3.distance(center, result.intersection);
            var snapToInner = false;
            // var innerRadius = (Vec3.length(selectionManager.worldDimensions) / 2) * 1.1;
            if (distanceFromCenter < innerRadius) {
                angleFromZero = Math.floor(angleFromZero/innerSnapAngle) * innerSnapAngle;
                snapToInner = true;
            }

            var yawChange = Quat.fromVec3Degrees({ x: 0, y: angleFromZero, z: 0 });
            Entities.editEntity(this.entityID, { rotation: Quat.multiply(yawChange, this.originalRotation) });


            // update the rotation display accordingly...
            var startAtCurrent = 360-angleFromZero;
            var endAtCurrent = 360;
            var startAtRemainder = 0;
            var endAtRemainder = 360-angleFromZero;
            if (angleFromZero < 0) {
                startAtCurrent = 0;
                endAtCurrent = -angleFromZero;
                startAtRemainder = -angleFromZero;
                endAtRemainder = 360;
            }

            if (snapToInner) {
                Overlays.editOverlay(this.rotateOverlayOuter, { startAt: 0, endAt: 360 });
                Overlays.editOverlay(this.rotateOverlayInner, { startAt: startAtRemainder, endAt: endAtRemainder });
                Overlays.editOverlay(this.rotateOverlayCurrent, { startAt: startAtCurrent, endAt: endAtCurrent, size: innerRadius,
                                                                majorTickMarksAngle: innerSnapAngle, minorTickMarksAngle: 0,
                                                                majorTickMarksLength: -0.25, minorTickMarksLength: 0, });
            } else {
                Overlays.editOverlay(this.rotateOverlayInner, { startAt: 0, endAt: 360 });
                Overlays.editOverlay(this.rotateOverlayOuter, { startAt: startAtRemainder, endAt: endAtRemainder });
                Overlays.editOverlay(this.rotateOverlayCurrent, { startAt: startAtCurrent, endAt: endAtCurrent, size: outerRadius,
                                                                majorTickMarksAngle: 45.0, minorTickMarksAngle: 5,
                                                                majorTickMarksLength: 0.25, minorTickMarksLength: 0.1, });
            }
        }

        if (this.turnInjector === null || !this.turnInjector.playing) {
            this.playTurnSound();
        }
    };
      // All callbacks start by updating the properties
    this.updateProperties = function(entityID) {
        if (this.entityID === null) {
            this.entityID = entityID;
        }
        this.properties = Entities.getEntityProperties(this.entityID);
    };

    this.cleanupRotateOverlay = function() {
        Overlays.deleteOverlay(this.rotateOverlayTarget);
        Overlays.deleteOverlay(this.rotateOverlayInner);
        Overlays.deleteOverlay(this.rotateOverlayOuter);
        Overlays.deleteOverlay(this.rotateOverlayCurrent);
        this.rotateOverlayTarget = null;
        this.rotateOverlayInner = null;
        this.rotateOverlayOuter = null;
        this.rotateOverlayCurrent = null;
    }

    this.displayRotateOverlay = function(mouseEvent) {
        var yawOverlayAngles = { x: 90, y: 0, z: 0 };
        var yawOverlayRotation = Quat.fromVec3Degrees(yawOverlayAngles);

        yawNormal   = { x: 0, y: 1, z: 0 };
        yawCenter = this.properties.position;
        rotationNormal = yawNormal;

        // Size the overlays to the current selection size
        var diagonal = (Vec3.length(this.properties.dimensions) / 2) * 1.1;
        var halfDimensions = Vec3.multiply(this.properties.dimensions, 0.5);
        innerRadius = diagonal;
        outerRadius = diagonal * 1.15;
        var innerAlpha = 0.2;
        var outerAlpha = 0.2;

        this.rotateOverlayTarget = Overlays.addOverlay("circle3d", {
                        position: this.properties.position,
                        size: rotateOverlayTargetSize,
                        color: { red: 0, green: 0, blue: 0 },
                        alpha: 0.0,
                        solid: true,
                        visible: true,
                        rotation: yawOverlayRotation,
                        ignoreRayIntersection: false
                    });

        this.rotateOverlayInner = Overlays.addOverlay("circle3d", {
                    position: this.properties.position,
                    size: innerRadius,
                    innerRadius: 0.9,
                    alpha: innerAlpha,
                    color: { red: 51, green: 152, blue: 203 },
                    solid: true,
                    visible: displayRotateTargets,
                    rotation: yawOverlayRotation,
                    hasTickMarks: true,
                    majorTickMarksAngle: innerSnapAngle,
                    minorTickMarksAngle: 0,
                    majorTickMarksLength: -0.25,
                    minorTickMarksLength: 0,
                    majorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    minorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    ignoreRayIntersection: true, // always ignore this
                });

        this.rotateOverlayOuter = Overlays.addOverlay("circle3d", {
                    position: this.properties.position,
                    size: outerRadius,
                    innerRadius: 0.9,
                    startAt: 0,
                    endAt: 360,
                    alpha: outerAlpha,
                    color: { red: 51, green: 152, blue: 203 },
                    solid: true,
                    visible: displayRotateTargets,
                    rotation: yawOverlayRotation,

                    hasTickMarks: true,
                    majorTickMarksAngle: 45.0,
                    minorTickMarksAngle: 5,
                    majorTickMarksLength: 0.25,
                    minorTickMarksLength: 0.1,
                    majorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    minorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    ignoreRayIntersection: true, // always ignore this
                });

        this.rotateOverlayCurrent = Overlays.addOverlay("circle3d", {
                    position: this.properties.position,
                    size: outerRadius,
                    startAt: 0,
                    endAt: 0,
                    innerRadius: 0.9,
                    color: { red: 224, green: 67, blue: 36},
                    alpha: 0.8,
                    solid: true,
                    visible: displayRotateTargets,
                    rotation: yawOverlayRotation,
                    ignoreRayIntersection: true, // always ignore this
                    hasTickMarks: true,
                    majorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    minorTickMarksColor: { red: 0, green: 0, blue: 0 },
                });

        var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
        var result = Overlays.findRayIntersection(pickRay);
        yawZero = result.intersection;

    };

    this.preload = function(entityID) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.downloadSounds();
    };

    this.clickDownOnEntity = function(entityID, mouseEvent) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.grab(mouseEvent);

        var nowDate = new Date();
        var nowMSecs = nowDate.getTime();
        this.clickedAt = nowMSecs;
        this.firstHolding = true;

        this.clicked.x = mouseEvent.x;
        this.clicked.y = mouseEvent.y;
        this.lastMovedPosition.x = mouseEvent.x;
        this.lastMovedPosition.y = mouseEvent.y;
        this.lastMovedMSecs = nowMSecs;

        this.pickRandomSounds();
    };

    this.holdingClickOnEntity = function(entityID, mouseEvent) {

        this.updateProperties(entityID); // All callbacks start by updating the properties

        if (this.firstHolding) {
            // if we haven't moved yet...
            if (this.clicked.x == mouseEvent.x && this.clicked.y == mouseEvent.y) {
                var d = new Date();
                var now = d.getTime();

                if (now - this.clickedAt > 500) {
                    this.displayRotateOverlay(mouseEvent);
                    this.firstHolding = false;
                    this.rotateMode = true;
                    this.originalRotation = this.properties.rotation;
                }
            } else {
                this.firstHolding = false;
            }
        }

        if (this.rotateMode) {
            this.rotate(mouseEvent);
        } else {
            this.move(mouseEvent);
        }

        this.stopSoundIfNotMoving(mouseEvent);
    };
    this.clickReleaseOnEntity = function(entityID, mouseEvent) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        if (this.rotateMode) {
            this.rotate(mouseEvent);
        } else {
            this.release(mouseEvent);
        }

        if (this.rotateOverlayTarget != null) {
            this.cleanupRotateOverlay();
            this.rotateMode = false;
        }

        this.firstHolding = false;
        this.stopSound();
    };

})
