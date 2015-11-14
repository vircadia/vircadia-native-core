//
//  bow.js
//
//  This script attaches to a bow that you can pick up with a hand controller. Load an arrow and then shoot it.
//  Created by James B. Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// to do: 
// create field with haybales and targets
// create torches to start arrow on fire 
// play start fire sound
// parent the arrow fire to the arrow
// delete the arrow fire when the arrow is notched (the bow will now create and track fire)
// make arrow rotate toward ground as it flies
// working shader or transparent model for glow box

(function() {

    Script.include("../../libraries/utils.js");

    var NOTCH_ARROW_SOUND_URL = 'http://hifi-content.s3.amazonaws.com/james/bow_and_arrow/sounds/notch.wav?123';
    var SHOOT_ARROW_SOUND_URL = 'http://hifi-content.s3.amazonaws.com/james/bow_and_arrow/sounds/String_release2.L.wav';
    var STRING_PULL_SOUND_URL = 'http://hifi-content.s3.amazonaws.com/james/bow_and_arrow/sounds/Bow_draw.1.L.wav';
    var ARROW_WHIZZ_SOUND_URL = 'http://hifi-content.s3.amazonaws.com/james/bow_and_arrow/sounds/whizz.wav';
    //todo : multiple impact sounds
    var ARROW_HIT_SOUND_URL = 'http://hifi-content.s3.amazonaws.com/bow_and_arrow/sounds/Arrow_impact1.L.wav'
    var ARROW_DIMENSIONS = {
        x: 0.02,
        y: 0.02,
        z: 0.64
    };

    var ZERO_VEC = {
        x: 0,
        y: 0,
        z: 0
    };

    var LINE_ENTITY_DIMENSIONS = {
        x: 1000,
        y: 1000,
        z: 1000
    };

    var ARROW_OFFSET = -0.36;
    var ARROW_TIP_OFFSET = 0.32;
    var ARROW_GRAVITY = {
        x: 0,
        y: -5.8,
        z: 0
    };

    var TOP_NOTCH_OFFSET = 0.6;
    var BOTTOM_NOTCH_OFFSET = 0.6;

    var LINE_DIMENSIONS = {
        x: 5,
        y: 5,
        z: 5
    };

    var DRAW_STRING_THRESHOLD = 0.80;

    var TARGET_LINE_LENGTH = 1;

    var LEFT_TIP = 1;
    var RIGHT_TIP = 3;

    var NOTCH_DETECTOR_OFFSET_FORWARD = 0.08;
    var NOTCH_DETECTOR_OFFSET_UP = 0.035;

    var NOTCH_DETECTOR_DIMENSIONS = {
        x: 0.05,
        y: 0.05,
        z: 0.05
    };

    var NOTCH_DETECTOR_DISTANCE = 0.1;

    var SHOT_SCALE = {
        min1: 0,
        max1: 0.6,
        min2: 5,
        max2: 20
    }


    var _this;

    function Bow() {
        _this = this;
        return;
    }

    // pick up bow
    // pick up arrow
    // move arrow near notch detector
    // pull back on the trigger to draw the string
    // release to fire

    Bow.prototype = {
        isGrabbed: false,
        stringDrawn: false,
        arrowTipPosition: null,
        preNotchString: null,
        hasArrowNotched: false,
        notchDetector: null,
        arrow: null,
        stringData: {
            currentColor: {
                red: 255,
                green: 255,
                blue: 255
            }
        },

        preload: function(entityID) {
            print('preload bow')
            this.entityID = entityID;
            this.bowID = entityID;
            this.stringPullSound = SoundCache.getSound(STRING_PULL_SOUND_URL);
            this.shootArrowSound = SoundCache.getSound(SHOOT_ARROW_SOUND_URL);
            this.arrowHitSound = SoundCache.getSound(ARROW_HIT_SOUND_URL);
            this.arrowNotchSound = SoundCache.getSound(NOTCH_ARROW_SOUND_URL);
            this.arrowWhizzSound = SoundCache.getSound(ARROW_WHIZZ_SOUND_URL);
            Script.update.connect(this.updateArrowTrackers);

        },

        unload: function() {
            Script.update.disconnect(this.updateArrowTrackers);
        },

        setLeftHand: function() {
            if (this.isGrabbed === true) {
                return false;
            }
            this.hand = 'left';
        },

        setRightHand: function() {
            if (this.isGrabbed === true) {
                return false;
            }
            this.hand = 'right';
        },

        startNearGrab: function() {
            if (this.isGrabbed === true) {
                return false;
            }
            this.isGrabbed = true;
            this.initialHand = this.hand;

            setEntityCustomData('grabbableKey', this.entityID, {
                turnOffOtherHand: true,
                invertSolidWhileHeld: true
            });

        },

        continueNearGrab: function() {

            // print('collidable bow' + Entities.getEntityProperties(this.entityID, "collisionsWillMove").collisionsWillMove)
            // print('collidable arrow' + Entities.getEntityProperties(this.arrow, "collisionsWillMove").collisionsWillMove)
            // print('collidable notch' + Entities.getEntityProperties(this.notchDetector, "collisionsWillMove").collisionsWillMove)
            // print('collidable topstring' + Entities.getEntityProperties(this.topString, "collisionsWillMove").collisionsWillMove)
            // print('collidable bottomstring' + Entities.getEntityProperties(this.bottomString, "collisionsWillMove").collisionsWillMove)
            // print('collidable prenotchstring' + Entities.getEntityProperties(this.preNotchString, "collisionsWillMove").collisionsWillMove)

            this.bowProperties = Entities.getEntityProperties(this.entityID, ["position", "rotation", "userData"]);

            this.updateNotchDetectorPosition();

            //check to see if an arrow has notched itself in our notch detector
            var userData = JSON.parse(this.bowProperties.userData);
            if (userData.hasOwnProperty('hifiBowKey')) {
                if (this.hasArrowNotched === false && userData.hifiBowKey.arrowID !== null) {
                    //notch the arrow
                    print('NOTCHING IT!')
                    this.playArrowNotchSound();
                    this.playStringPullSound();
                    this.hasArrowNotched = userData.hifiBowKey.hasArrowNotched;

                    this.arrow = userData.hifiBowKey.arrowID;
                    this.arrowIsBurning = userData.hifiBowKey.isBurning;

                    setEntityCustomData('grabbableKey', this.entityID, {
                        turnOffOtherHand: true,
                        invertSolidWhileHeld: true
                    });
                }

            }

            //this.arrow
            //this.hasArrowNotched


            //create a string across the bow when we pick it up
            if (this.preNotchString === null) {
                print('CREATE PRE NOTCH STRING')
                this.createPreNotchString();
            }

            if (this.preNotchString !== null && this.hasArrowNotched === false) {
                // print('DRAW PRE NOTCH STRING')
                this.drawPreNotchStrings();
            }

            // create the notch detector that arrows will look for
            if (this.notchDetector === null) {
                print('CREATE NOTCH DETECTOR')
                this.createNotchDetector();
            }

            //if we have an arrow notched, then draw some new strings
            if (this.hasArrowNotched === true) {
                if (this.preNotchString !== null) {
                    //  print('MAKE PRE NOTCH INVISIBLE')
                    Entities.editEntity(this.preNotchString, {
                        visible: false
                    });
                }
                // print('CHECK STRING HAND')
                //only test for strings now that an arrow is notched


                this.checkStringHand();

            } else {
                //   print('DONT DO ANYTHING')
                //otherwise, don't do much of anything.

            }
        },

        releaseGrab: function() {
            if (this.isGrabbed === true && this.hand === this.initialHand) {
                this.isGrabbed = false;
                this.stringDrawn = false;
                this.deleteStrings();
                setEntityCustomData('grabbableKey', this.entityID, {
                    turnOffOtherHand: false,
                    invertSolidWhileHeld: true
                });
                Entities.deleteEntity(this.notchDetector);
                Entities.deleteEntity(this.preNotchString);

                this.notchDetector = null;
                this.preNotchString = null;

            }
        },

        createStrings: function() {
            this.createTopString();
            this.createBottomString();
        },

        createTopString: function() {
            var stringProperties = {
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, TOP_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS,
                collisionsWillMove: false,
                ignoreForCollisions: true,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    }
                })
            };

            this.topString = Entities.addEntity(stringProperties);
        },

        createBottomString: function() {
            var stringProperties = {
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, BOTTOM_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS,
                collisionsWillMove: false,
                ignoreForCollisions: true,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    }
                })
            };

            this.bottomString = Entities.addEntity(stringProperties);
        },

        deleteStrings: function() {
            Entities.deleteEntity(this.topString);
            Entities.deleteEntity(this.bottomString);
        },

        updateStringPositions: function() {

            var upVector = Quat.getUp(this.bowProperties.rotation);
            var upOffset = Vec3.multiply(upVector, TOP_NOTCH_OFFSET);
            var downVector = Vec3.multiply(-1, Quat.getUp(this.bowProperties.rotation));
            var downOffset = Vec3.multiply(downVector, BOTTOM_NOTCH_OFFSET);
            var backOffset = Vec3.multiply(-0.1, Quat.getFront(this.bowProperties.rotation));

            var topStringPosition = Vec3.sum(this.bowProperties.position, upOffset);
            this.topStringPosition = Vec3.sum(topStringPosition, backOffset);
            var bottomStringPosition = Vec3.sum(this.bowProperties.position, downOffset);
            this.bottomStringPosition = Vec3.sum(bottomStringPosition, backOffset);

            Entities.editEntity(this.topString, {
                position: this.topStringPosition
            });

            Entities.editEntity(this.bottomString, {
                position: this.bottomStringPosition
            });

            Entities.editEntity(this.preNotchString, {
                position: this.topStringPosition
            });

        },

        drawStrings: function() {

            this.updateStringPositions();
            var lineVectors = this.getLocalLineVectors();

            Entities.editEntity(this.topString, {
                linePoints: [{
                    x: 0,
                    y: 0,
                    z: 0
                }, lineVectors[0]],
                lineWidth: 5,
                color: this.stringData.currentColor
            });

            Entities.editEntity(this.bottomString, {
                linePoints: [{
                    x: 0,
                    y: 0,
                    z: 0
                }, lineVectors[1]],
                lineWidth: 5,
                color: this.stringData.currentColor
            });

        },


        getLocalLineVectors: function() {
            var topVector = Vec3.subtract(this.stringData.handPosition, this.topStringPosition);
            var bottomVector = Vec3.subtract(this.stringData.handPosition, this.bottomStringPosition);
            return [topVector, bottomVector];
        },



        createPreNotchString: function() {
            var stringProperties = {
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, TOP_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    }
                })
            };

            this.preNotchString = Entities.addEntity(stringProperties);
        },

        drawPreNotchStrings: function() {

            this.updateStringPositions();

            var downVector = Vec3.multiply(-1, Quat.getUp(this.bowProperties.rotation));
            var downOffset = Vec3.multiply(downVector, BOTTOM_NOTCH_OFFSET * 2);


            Entities.editEntity(this.preNotchString, {
                linePoints: [{
                    x: 0,
                    y: 0,
                    z: 0
                }, Vec3.sum({
                    x: 0,
                    y: 0,
                    z: 0
                }, downOffset)],
                lineWidth: 5,
                color: this.stringData.currentColor
            });
        },

        checkStringHand: function() {
            //invert the hands because our string will be held with the opposite hand of the first one we pick up the bow with
            if (this.initialHand === 'left') {
                this.getStringHandPosition = MyAvatar.getRightPalmPosition;
                this.getStringHandRotation = MyAvatar.getRightPalmRotation;
                this.getGrabHandPosition = MyAvatar.getLeftPalmPosition;
                this.getGrabHandRotation = MyAvatar.getLeftPalmRotation;
                this.stringTriggerAction = Controller.findAction("RIGHT_HAND_CLICK");
            } else if (this.initialHand === 'right') {
                this.getStringHandPosition = MyAvatar.getLeftPalmPosition;
                this.getStringHandRotation = MyAvatar.getLeftPalmRotation;
                this.getGrabHandPosition = MyAvatar.getRightPalmPosition;
                this.getGrabHandRotation = MyAvatar.getRightPalmRotation;
                this.stringTriggerAction = Controller.findAction("LEFT_HAND_CLICK");
            }

            this.triggerValue = Controller.getActionValue(this.stringTriggerAction);
            //  print('TRIGGER VALUE:::' + this.triggerValue)

            if (this.triggerValue < DRAW_STRING_THRESHOLD && this.stringDrawn === true) {

                // firing the arrow
                print('HIT RELEASE LOOP IN CHECK')


                this.releaseArrow();

            } else if (this.triggerValue >= DRAW_STRING_THRESHOLD && this.stringDrawn === true) {
                //  print('HIT CONTINUE LOOP IN CHECK')
                //continuing to aim the arrow
                this.stringData.handPosition = this.getStringHandPosition();
                this.stringData.handRotation = this.getStringHandRotation();
                this.stringData.grabHandPosition = this.getGrabHandPosition();
                this.stringData.grabHandRotation = this.getGrabHandRotation();
                this.drawStrings();
                this.updateArrowPositionInNotch();

            } else if (this.triggerValue >= DRAW_STRING_THRESHOLD && this.stringDrawn === false) {
                print('HIT START LOOP IN CHECK')
                    //the first time aiming the arrow
                this.stringDrawn = true;
                this.createStrings();
                var arrowTracker = this.createArrowTracker(this.arrow);
                this.arrowTrackers.push(arrowTracker)
                this.stringData.handPosition = this.getStringHandPosition();
                this.stringData.handRotation = this.getStringHandRotation();
                this.stringData.grabHandPosition = this.getGrabHandPosition();

                this.stringData.grabHandRotation = this.getGrabHandRotation();

                this.drawStrings();
                this.updateArrowPositionInNotch();

            }
        },

        setArrowTipPosition: function(arrowPosition, arrowRotation) {
            var frontVector = Quat.getFront(arrowRotation);
            var frontOffset = Vec3.multiply(frontVector, ARROW_TIP_OFFSET);
            var arrowTipPosition = Vec3.sum(arrowPosition, frontOffset);
            this.arrowTipPosition = arrowTipPosition;
            return arrowTipPosition;

        },
        getArrowPosition: function() {
            var arrowVector = Vec3.subtract(this.stringData.handPosition, this.stringData.grabHandPosition);
            arrowVector = Vec3.normalize(arrowVector);
            arrowVector = Vec3.multiply(arrowVector, ARROW_OFFSET);
            var arrowPosition = Vec3.sum(this.stringData.grabHandPosition, arrowVector);
            return arrowPosition;
        },

        updateArrowPositionInNotch: function() {
            //move it backwards
            var handToNotch = Vec3.subtract(this.notchDetectorPosition, this.stringData.handPosition);
            var pullBackDistance = Vec3.length(handToNotch);

            if (pullBackDistance >= 0.6) {
                pullBackDistance = 0.6;
            }

            var pullBackOffset = Vec3.multiply(handToNotch, -pullBackDistance);
            var arrowPosition = Vec3.sum(this.notchDetectorPosition, pullBackOffset);
            this.changeStringPullSoundVolume(pullBackDistance);
            //move it forward a bit
            var pushForwardOffset = Vec3.multiply(handToNotch, -ARROW_OFFSET);
            var finalArrowPosition = Vec3.sum(arrowPosition, pushForwardOffset);

            var arrowRotation = Quat.rotationBetween(Vec3.FRONT, handToNotch);
            // this.setArrowTipPosition(finalArrowPosition, arrowRotation);
            Entities.editEntity(this.arrow, {
                position: finalArrowPosition,
                rotation: arrowRotation
            })

        },

        releaseArrow: function() {
            print('RELEASE ARROW!!!')

            var handToNotch = Vec3.subtract(this.notchDetectorPosition, this.stringData.handPosition);
            var pullBackDistance = Vec3.length(handToNotch);
            if (pullBackDistance >= 0.6) {
                pullBackDistance = 0.6;
            }

            var arrowRotation = Quat.rotationBetween(Vec3.FRONT, handToNotch);

            print('HAND DISTANCE:: ' + pullBackDistance);
            var arrowForce = this.scaleArrowShotStrength(pullBackDistance);
            print('ARROW FORCE::' + arrowForce);
            var forwardVec = Vec3.multiply(handToNotch, arrowForce);

            var arrowProperties = {
                rotation: arrowRotation,
                velocity: forwardVec
            };

            this.playShootArrowSound();
            Entities.editEntity(this.arrow, arrowProperties);

            setEntityCustomData('hifiBowKey', this.entityID, {
                hasArrowNotched: false,
                arrowID: null
            });

            var arrowStore = this.arrow;
            this.arrow = null;
            this.hasArrowNotched = false;

            Entities.editEntity(this.preNotchString, {
                visible: true
            });

            this.stringDrawn = false;
            this.deleteStrings();

            //set an itnerval to heck how far the arrow is from the bow before adding gravity, etc.  if we add this too soon, the arrow collides with the bow.  hence, this function

            this.physicalArrowInterval = Script.setInterval(function() {
                print('in physical interval')
                var arrowProps = Entities.getEntityProperties(arrowStore, "position");
                var bowProps = Entities.getEntityProperties(_this.entityID, "position");
                var arrowPosition = arrowProps.position;
                var bowPosition = bowProps.position;

                var length = Vec3.distance(arrowPosition, bowPosition);
                print('LENGTH:::' + length);
                if (length > 2) {
                    print('make arrow physical' + arrowStore)
                    Entities.editEntity(arrowStore, {
                        ignoreForCollisions: false,
                        collisionsWillMove: true,
                        gravity: ARROW_GRAVITY
                    });
                    print('after make physical' + arrowStore)
                    var arrowProps = Entities.getEntityProperties(arrowStore)
                    print('arrowprops-collisions::' + arrowProps.collisionsWillMove);
                    print('arrowprops-graviy' + JSON.stringify(arrowProps.gravity));
                    Script.setTimeout(function() {
                        print('in timeout :: ' + arrowStore)
                        var arrowProps = Entities.getEntityProperties(arrowStore)
                        print('arrowprops-gravity2' + JSON.stringify(arrowProps.gravity));
                        print('ARROW USER DATA::' + arrowProps.userData)

                    }, 1000)

                    Script.clearInterval(_this.physicalArrowInterval)
                }
            }, 10)

        },

        scaleArrowShotStrength: function(value) {
            var min1 = SHOT_SCALE.min1;
            var max1 = SHOT_SCALE.max1;
            var min2 = SHOT_SCALE.min2;
            var max2 = SHOT_SCALE.max2;
            return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
        },

        createNotchDetector: function() {
            print('CREATE NOTCH DETECTOR')
            var detectorPosition;
            var frontVector = Quat.getFront(this.bowProperties.rotation);
            var notchVectorForward = Vec3.multiply(frontVector, NOTCH_DETECTOR_OFFSET_FORWARD);
            var upVector = Quat.getUp(this.bowProperties.rotation);
            var notchVectorUp = Vec3.multiply(upVector, NOTCH_DETECTOR_OFFSET_UP);

            detectorPosition = Vec3.sum(this.bowProperties.position, notchVectorForward);
            detectorPosition = Vec3.sum(detectorPosition, notchVectorUp);

            var detectorProperties = {
                name: 'Hifi-NotchDetector',
                type: 'Box',
                visible: false,
                collisionsWillMove: false,
                ignoreForCollisions: true,
                dimensions: NOTCH_DETECTOR_DIMENSIONS,
                position: detectorPosition,
                color: {
                    red: 0,
                    green: 255,
                    blue: 0
                },
                userData: JSON.stringify({
                    hifiBowKey: {
                        bowID: this.entityID
                    }
                })
            };

            this.notchDetector = Entities.addEntity(detectorProperties);
        },

        updateNotchDetectorPosition: function() {
            var detectorPosition;
            var frontVector = Quat.getFront(this.bowProperties.rotation);
            var notchVectorForward = Vec3.multiply(frontVector, NOTCH_DETECTOR_OFFSET_FORWARD);
            var upVector = Quat.getUp(this.bowProperties.rotation);
            var notchVectorUp = Vec3.multiply(upVector, NOTCH_DETECTOR_OFFSET_UP);

            detectorPosition = Vec3.sum(this.bowProperties.position, notchVectorForward);
            detectorPosition = Vec3.sum(detectorPosition, notchVectorUp);

            this.notchDetectorPosition = detectorPosition;

            Entities.editEntity(this.notchDetector, {
                position: this.notchDetectorPosition,
                rotation: this.bowProperties.rotation
            });
        },

        createFireParticleSystem: function() {
            print('MAKING A  FIRE')
                //will probably need to dynamically update the orientation to match the arrow???
            var myOrientation = Quat.fromPitchYawRollDegrees(-90, 0, 0.0);

            var animationSettings = JSON.stringify({
                fps: 30,
                running: true,
                loop: true,
                firstFrame: 1,
                lastFrame: 10000
            });

            var fire = Entities.addEntity({
                type: "ParticleEffect",
                name: "Hifi-Arrow-Fire",
                animationSettings: animationSettings,
                textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
                emitRate: 100,
                position: MyAvatar.position,
                colorStart: {
                    red: 70,
                    green: 70,
                    blue: 137
                },
                color: {
                    red: 200,
                    green: 99,
                    blue: 42
                },
                colorFinish: {
                    red: 255,
                    green: 99,
                    blue: 32
                },
                radiusSpread: 0.01,
                radiusStart: 0.02,
                radiusEnd: 0.001,
                particleRadius: 0.15,
                radiusFinish: 0.0,
                emitOrientation: myOrientation,
                emitSpeed: 0.3,
                speedSpread: 0.1,
                alphaStart: 0.05,
                alpha: 0.1,
                alphaFinish: 0.05,
                emitDimensions: {
                    x: 0.5,
                    y: 0.5,
                    z: 0.1
                },
                polarFinish: 0.1,
                emitAcceleration: {
                    x: 0.0,
                    y: 0.0,
                    z: 0.0
                },
                accelerationSpread: {
                    x: 0.1,
                    y: 0.01,
                    z: 0.1
                },
                lifespan: 0.5
            });

            return fire;

        },
        createArrowTracker: function(arrow, isBurning) {
            print('in create arrow tracker:::' + arrow)
            var _t = this;

            // var isBurning =  this.arrowIsBurning;
            //delete this line below once debugging is done
            var isBurning = isBurning || true;
            var arrowTracker = {
                arrowID: arrow,
                // whizzingSound: _t.playWhizzSound(),
                //fireSound: _t.createFireSound(),
                hasPlayedCollisionSound: false,
                glowBox: _t.createGlowBox(),
                fireParticleSystem: _t.createFireParticleSystem(),
                childEntities: [],
                childSounds: [],
                childParticleSystems: [],
                init: function() {
                    print('init arrow tracker')
                    this.setChildren();

                },
                setCollisionCallback: function() {
                    print('set arrow tracker callback' + this.arrowID)

                    Script.addEventHandler(this.arrowID, "collisionWithEntity", function(entityA, entityB, collision) {
                        //have to reverse lookup the tracker by the arrow id to get access to the children
                        var tracker = getArrowTrackerByArrowID(entityA);

                        print('ARROW COLLIDED WITH SOMETHING!' + tracker.glowBox)
                        print('TRACKER IN COLLISION !' + tracker)
                            // _t.playArrowHitSound(collision.contactPoint);
                            //Vec3.print('penetration = ', collision.penetration);
                            //Vec3.print('collision contact point = ', collision.contactPoint);
                            //   var orientationChange = orientationOf(collision.velocityChange);
                        Entities.deleteEntity(tracker.glowBox);
                        //we don't want to update this arrow tracker anymore
                        var index = _t.arrowTrackers.indexOf(entityA);
                        if (index > -1) {
                            //    _t.arrowTrackers.splice(index, 1);
                        }
                    });
                },
                setChildren: function() {
                    print('set arrow tracker children')
                        // this.childSounds.push(this.whizzingSound);
                        // this.childSounds.push(this.fireSound);
                    this.childEntities.push(this.glowBox);
                    this.childParticleSystems.push(this.fireParticleSystem);
                },
                updateChildEntities: function(arrowID) {

                    // print('UPDATING CHILDREN OF TRACKER:::' + this.childEntities.length);
                    var arrowProperties = Entities.getEntityProperties(this.arrowID, ["position", "rotation"]);

                    //update the positions
                    // this.soundEntities.forEach(function(injector) {
                    //     var audioProperties = {
                    //         volume: 0.25,
                    //         position: arrowProperties.position
                    //     };
                    //     injector.options = audioProperties;
                    // })

                    this.childEntities.forEach(function(child) {
                        Entities.editEntity(child, {
                            position: arrowProperties.position,
                            rotation: arrowProperties.rotation
                        })
                    })

                    this.childParticleSystems.forEach(function(child) {
                        Entities.editEntity(child, {
                            position: arrowProperties.position
                        })
                    })

                }
            };
            arrowTracker.init();
            arrowTracker.setCollisionCallback();
            print('after create arrow tracker')

            return arrowTracker
        },
        createFireBurningSound: function() {
            var audioProperties = {
                volume: 0.25,
                position: this.bowProperties.position,
                loop: true
            };

            var injector = Audio.playSound(this.fireBurningSound, audioProperties);

            return injector
        },
        createGlowBoxAsModel: function() {
            var modelURL = 'http://hifi-content.s3.amazonaws.com/james/bow_and_arrow/models/glowBox.fbx';
            var properties = {
                name: 'Hifi-Arrow-Glow',
                type: 'Model',
                modelURL: modelURL,
                dimensions: ARROW_DIMENSIONS,
                collisionsWillMove: false,
                ignoreForCollisions: true,
            }
        },
        createGlowBox: function() {
            print('creating glow box')
            var shaderUrl = Script.resolvePath('../../shaders/exampleV2.fs');
            var properties = {
                name: 'Hifi-Arrow-Glow',
                type: 'Box',
                dimensions: ARROW_DIMENSIONS,
                collisionsWillMove: false,
                ignoreForCollisions: true,
                color: {
                    red: 255,
                    green: 0,
                    blue: 255
                },
                //i want to use shader but for some reason its white... need to fix;


                // userData: JSON.stringify({
                //     "grabbableKey": {
                //         grabbable: false
                //     },
                //     "ProceduralEntity": {
                //         "shaderUrl": shaderUrl,
                //         // V2 and onwards, shaders must include a version identifier, or they will default
                //         // to V1 behavior
                //         "version": 2,
                //         // Any values specified here will be passed on to uniforms with matching names in
                //         // the shader. Only numbers and arrays of length 1-4 of numbers are supported.
                //         //
                //         // The size of the data must match the size of the uniform:
                //         //      a number or 1 value array = 'uniform float'
                //         //      2 value array = 'uniform vec2'
                //         //      3 value array = 'uniform vec3'
                //         //      4 value array = 'uniform vec4'
                //         //
                //         // Uniforms should always be declared in the shader with a default value
                //         // or failure to specify the value here will result in undefined behavior.
                //         "uniforms": {
                //             // uniform float iSpeed = 1.0;
                //             "iSpeed": 2.0,
                //             // uniform vec3 iSize = vec3(1.0);
                //             "iSize": [1.0, 2.0, 4.0]
                //         }
                //     }
                // })
            }
            var glowBox = Entities.addEntity(properties);
            return glowBox
        },
        arrowTrackers: [],
        updateArrowTrackers: function() {
            // print('updating arrow trackers:::' + _this.arrowTrackers.length);
            _this.arrowTrackers.forEach(function(tracker) {
                var arrowID = tracker.arrowID;
                // print('TRACKER ARROW ID'+tracker.arrowID)
                tracker.updateChildEntities(arrowID);
            })
        },
        playStringPullSound: function() {
            var audioProperties = {
                volume: 0.25,
                position: this.bowProperties.position
            };
            this.stringPullInjector = Audio.playSound(this.stringPullSound, audioProperties);
        },
        playShootArrowSound: function(sound) {
            var audioProperties = {
                volume: 0.25,
                position: this.bowProperties.position
            };
            Audio.playSound(this.shootArrowSound, audioProperties);
        },
        playArrowHitSound: function(position) {
            var audioProperties = {
                volume: 0.25,
                position: position
            };
            Audio.playSound(this.arrowHitSound, audioProperties);
        },
        playArrowNotchSound: function() {
            print('play arrow notch sound')
            var audioProperties = {
                volume: 0.25,
                position: this.bowProperties.position
            };
            Audio.playSound(this.arrowNotchSound, audioProperties);
        },
        playArrowWhizzSound: function() {

            var audioProperties = {
                volume: 0.0,
                position: this.bowProperties.position
            };
            var injector = Audio.playSound(this.arrowWhizzSound, audioProperties);

            return injector
        },
        changeStringPullSoundVolume: function(pullBackDistance) {
            var audioProperties = {
                volume: 0.25,
                position: this.bowProperties.position
            }

            this.stringPullInjector.options = audioProperties
        }


    };

    getArrowTrackerByArrowID = function(arrowID) {
        var result = _this.arrowTrackers.filter(function(tracker) {
            return tracker.arrowID === arrowID;
        });

        var tracker = result[0]
        return tracker
    }
    return new Bow();
});