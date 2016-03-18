//
//  tank.js
//  
//
//  created by James b. Pollack @imgntn on 3/9/2016
//  Copyright 2016 High Fidelity, Inc.   
// 
//  looks for fish to swim around, if there are none it makes some.  adds an attractor where the person simulating it is looking. 
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    Script.include('../../../../examples/libraries/virtualBaton.js');

    //only one person should simulate the tank at a time -- we pass around a virtual baton
    var baton;
    var iOwn = false;
    var _entityID;
    var _this;
    var connected = false;

    var TANK_SEARCH_RADIUS = 5;
    var WANT_LOOK_DEBUG_LINE = false;
    var WANT_LOOK_DEBUG_SPHERE = false;

    var INTERSECT_COLOR = {
        red: 255,
        green: 0,
        blue: 255
    }

    function FishTank() {
        _this = this;
    }

    function startUpdate() {
        //when the baton is claimed;
        //   print('trying to claim the object' + _entityID)
        iOwn = true;
        connected = true;
        Script.update.connect(_this.update);
    }

    function stopUpdateAndReclaim() {
        //when the baton is released;
        // print('i released the object ' + _entityID)
        iOwn = false;
        if (connected === true) {
            connected = false;
            Script.update.disconnect(_this.update);
            _this.clearLookAttractor();
        }
        //hook up callbacks to the baton
        baton.claim(startUpdate, stopUpdateAndReclaim);

    }

    function getOffsetFromTankCenter(VERTICAL_OFFSET, FORWARD_OFFSET, LATERAL_OFFSET) {

        var tankProperties = Entities.getEntityProperties(_this.entityID);
        print('GOT PROPERTIES FOR TANK!')

        var upVector = Quat.getUp(tankProperties.rotation);
        var frontVector = Quat.getFront(tankProperties.rotation);
        var rightVector = Quat.getRight(tankProperties.rotation);

        var upOffset = Vec3.multiply(upVector, VERTICAL_OFFSET);
        var frontOffset = Vec3.multiply(frontVector, FORWARD_OFFSET);
        var rightOffset = Vec3.multiply(rightVector, LATERAL_OFFSET);

        var finalOffset = Vec3.sum(tankProperties.position, upOffset);
        finalOffset = Vec3.sum(finalOffset, frontOffset);
        finalOffset = Vec3.sum(finalOffset, rightOffset);
        return finalOffset
    }


    FishTank.prototype = {
        fish: null,
        tankLocked: false,
        hasLookAttractor: false,
        lookAttractor: null,
        overlayLine: null,
        overlayLineDistance: 3,
        debugSphere: null,
        findFishInTank: function() {
            // print('looking for a fish in the tank')
            var results = Entities.findEntities(_this.currentProperties.position, TANK_SEARCH_RADIUS);
            var fishList = [];

            results.forEach(function(fish) {
                var properties = Entities.getEntityProperties(fish, 'name');
                if (properties.name.indexOf('hifi-fishtank-fish' + _this.entityID) > -1) {
                    fishList.push(fish);
                }
            })

            //  print('fish? ' + fishList.length)
            return fishList;
        },

        initialize: function(entityID) {
            var properties = Entities.getEntityProperties(entityID)

            if (properties.hasOwnProperty('userData') === false || properties.userData.length === 0) {
                _this.initTimeout = Script.setTimeout(function() {
                    if (properties.hasOwnProperty('userData')) {
                        // print('has user data property')
                    }
                    if (properties.userData.length === 0) {
                        // print('user data length is zero')
                    }

                    // print('try again in one second')
                    _this.initialize(entityID);
                }, 1000)

            } else {
                //  print('userdata before parse attempt' + properties.userData)
                _this.userData = null;
                try {
                    _this.userData = JSON.parse(properties.userData);
                } catch (err) {
                    // print('error parsing json');
                    // print('properties are:' + properties.userData);
                    return;
                }
                // print('after parse')
                _this.currentProperties = Entities.getEntityProperties(entityID);
                baton = virtualBaton({
                    batonName: 'io.highfidelity.fishtank:' + entityID, // One winner for each entity
                });

                stopUpdateAndReclaim();

            }
        },

        preload: function(entityID) {
            // print("preload");
            this.entityID = entityID;
            _entityID = entityID;
            this.initialize(entityID);
            this.initTimeout = null;

        },

        unload: function() {
            // print(' UNLOAD')
            if (connected === true) {
                Script.update.disconnect(_this.update);
            }
            if (WANT_LOOK_DEBUG_LINE === true) {
                _this.overlayLineOff();
            }
            if (baton) {
                // print('BATON RELEASE ')
                baton.release(function() {});
            }

        },
        update: function(deltaTime) {

            if (iOwn === false) {
                // print('i dont own')
                //exit if we're not supposed to be simulating the fish
                return
            }
            // print('i am the owner!')
            //do stuff
            updateFish(deltaTime);
            _this.seeIfOwnerIsLookingAtTheTank();
        },

        debugSphereOn: function(position) {
            if (_this.debugSphere !== null) {
                Entities.editEntity(_this.debugSphere, {
                    visible: true
                })
                return;
            }
            var sphereProperties = {
                type: 'Sphere',
                parentID: _this.entityID,
                dimensions: {
                    x: 0.1,
                    y: 0.1,
                    z: 0.1,
                },
                color: INTERSECT_COLOR,
                position: position,
                collisionless: true
            }
            _this.debugSphere = Entities.addEntity(sphereProperties);
        },

        updateDebugSphere: function(position) {
            Entities.editEntity(_this.debugSphere, {
                visible: true,
                position: position
            })
        },

        debugSphereOff: function() {
            Entities.editEntity(_this.debugSphere, {
                visible: false
            })

        },

        overlayLineOn: function(closePoint, farPoint, color) {
            if (_this.overlayLine === null) {
                var lineProperties = {
                    lineWidth: 5,
                    start: closePoint,
                    end: farPoint,
                    color: color,
                    ignoreRayIntersection: true, // always ignore this
                    visible: true,
                    alpha: 1
                };
                this.overlayLine = Overlays.addOverlay("line3d", lineProperties);

            } else {
                var success = Overlays.editOverlay(_this.overlayLine, {
                    lineWidth: 5,
                    start: closePoint,
                    end: farPoint,
                    color: color,
                    visible: true,
                    ignoreRayIntersection: true, // always ignore this
                    alpha: 1
                });
            }
        },

        overlayLineOff: function() {
            if (_this.overlayLine !== null) {
                Overlays.deleteOverlay(this.overlayLine);
            }
            _this.overlayLine = null;
        },

        seeIfOwnerIsLookingAtTheTank: function() {
            var cameraPosition = Camera.getPosition();
            var cameraOrientation = Camera.getOrientation();

            var front = Quat.getFront(cameraOrientation);
            var pickRay = {
                origin: cameraPosition,
                direction: front
            };

            if (WANT_LOOK_DEBUG_LINE === true) {
                _this.overlayLineOn(pickRay.origin, Vec3.sum(pickRay.origin, Vec3.multiply(front, _this.overlayLineDistance)), INTERSECT_COLOR);
            };

            // var brn = _this.userData['hifi-home-fishtank']['corners'].brn;
            // var tfl = _this.userData['hifi-home-fishtank']['corners'].tfl;
            var innerContainer = _this.userData['hifi-home-fishtank'].innerContainer;

            var intersection = Entities.findRayIntersection(pickRay, true, [innerContainer], [_this.entityID]);

            if (intersection.intersects && intersection.entityID === innerContainer) {
                //print('intersecting a tank')
                if (WANT_LOOK_DEBUG_SPHERE === true) {
                    if (_this.debugSphere === null) {
                        _this.debugSphereOn(intersection.intersection);
                    } else {
                        _this.updateDebugSphere(intersection.intersection);
                    }
                }
                if (intersection.distance > LOOK_ATTRACTOR_DISTANCE) {
                    if (WANT_LOOK_DEBUG_SPHERE === true) {
                        _this.debugSphereOff();
                    }
                    return
                }
                // print('intersection:: ' + JSON.stringify(intersection));
                if (_this.hasLookAttractor === false) {
                    _this.createLookAttractor(intersection.intersection, intersection.distance);
                } else if (_this.hasLookAttractor === true) {
                    _this.updateLookAttractor(intersection.intersection, intersection.distance);
                }
            } else {
                if (_this.hasLookAttractor === true) {
                    _this.clearLookAttractor();
                }
            }
        },
        createLookAttractor: function(position, distance) {
            _this.lookAttractor = {
                position: position,
                distance: distance
            };
            _this.hasLookAttractor = true;
        },
        updateLookAttractor: function(position, distance) {
            _this.lookAttractor = {
                position: position,
                distance: distance
            };
        },
        clearLookAttractor: function() {
            _this.hasLookAttractor = false;
            _this.lookAttractor = null;
        },
        createLookAttractorEntity: function() {

        },
        findLookAttractorEntities: function() {

        },
        seeIfAnyoneIsLookingAtTheTank: function() {

            // get avatars
            // get their positions
            // get their gazes
            // check for intersection
            // add attractor for closest person (?)

        }

    };

    //
    //  flockOfFish.js
    //  examples
    //
    //  Philip Rosedale
    //  Copyright 2016 High Fidelity, Inc.   
    //  Fish smimming around in a space in front of you 
    //   
    //  Distributed under the Apache License, Version 2.0.
    //  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

    var FISHTANK_USERDATA_KEY = 'hifi-home-fishtank'

    var LIFETIME = 300; //  Fish live for 5 minutes 
    var NUM_FISH = 8;
    var TANK_DIMENSIONS = {
        x: 0.8212,
        y: 0.8116,
        z: 2.1404
    };

    var TANK_WIDTH = TANK_DIMENSIONS.z / 2;
    var TANK_HEIGHT = TANK_DIMENSIONS.y / 2;

    var FISH_DIMENSIONS = {
        x: 0.0149,
        y: 0.02546,
        z: 0.0823
    }

    var MAX_SIGHT_DISTANCE = 1.5;
    var MIN_SEPARATION = 0.15;
    var AVOIDANCE_FORCE = 0.032;
    var COHESION_FORCE = 0.025;
    var ALIGNMENT_FORCE = 0.025;
    var LOOK_ATTRACTOR_FORCE = 0.02;
    var LOOK_ATTRACTOR_DISTANCE = 1.75;
    var SWIMMING_FORCE = 0.025;
    var SWIMMING_SPEED = 0.5;
    var FISH_DAMPING = 0.55;
    var FISH_ANGULAR_DAMPING = 0.55;

    var THROTTLE = false;
    var THROTTLE_RATE = 100;
    var sinceLastUpdate = 0;


var LOWER_CORNER_VERTICAL_OFFSET = (-TANK_DIMENSIONS.y / 2) + 0.3;
var LOWER_CORNER_FORWARD_OFFSET = TANK_DIMENSIONS.x;
var LOWER_CORNER_LATERAL_OFFSET = -TANK_DIMENSIONS.z / 8;

var UPPER_CORNER_VERTICAL_OFFSET = (TANK_DIMENSIONS.y / 2)-0.3;
var UPPER_CORNER_FORWARD_OFFSET = -TANK_DIMENSIONS.x;
var UPPER_CORNER_LATERAL_OFFSET = TANK_DIMENSIONS.z / 8;


    // var FISH_MODEL_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/Fish-1.fbx";

    // var FISH_MODEL_TWO_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/Fish-2.fbx";
    var FISH_MODEL_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/goodfish5.fbx";
    var FISH_MODEL_TWO_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/goodfish5.fbx";
    var fishLoaded = false;

    function randomVector(scale) {
        return {
            x: Math.random() * scale - scale / 2.0,
            y: Math.random() * scale - scale / 2.0,
            z: Math.random() * scale - scale / 2.0
        };
    }

    function updateFish(deltaTime) {
        // print('update loop')

        if (_this.tankLocked === true) {
            return;
        }
        if (!Entities.serversExist() || !Entities.canRez()) {
            return;
        }


        if (THROTTLE === true) {
            sinceLastUpdate = sinceLastUpdate + deltaTime * 100;
            if (sinceLastUpdate > THROTTLE_RATE) {
                sinceLastUpdate = 0;
            } else {
                return;
            }
        }


        //  print('has userdata fish??' + _this.userData['hifi-home-fishtank'].fishLoaded)

        if (_this.userData['hifi-home-fishtank'].fishLoaded === false) {
            //no fish in the user data
            _this.tankLocked = true;
            print('NO FISH YET SO LOAD EM!!!')
            loadFish(NUM_FISH);
            var data = {
                fishLoaded: true,
                bubbleSystem: _this.userData['hifi-home-fishtank'].bubbleSystem,
                // bubbleSound: _this.userData['hifi-home-fishtank'].bubbleSound,
                // corners: {
                //     brn: _this.userData['hifi-home-fishtank'].lowerCorner,
                //     tfl: _this.userData['hifi-home-fishtank'].upperCorner
                // },
                innerContainer: _this.userData['hifi-home-fishtank'].innerContainer,

            }
            setEntityCustomData(FISHTANK_USERDATA_KEY, _this.entityID, data);
            _this.userData['hifi-home-fishtank'].fishLoaded = true;
            Script.setTimeout(function() {
                _this.fish = _this.findFishInTank();
            }, 2000)
            return;
        } else {

            //fish in userdata already
            if (_this.fish === null) {
                _this.fish = _this.findFishInTank();
            }

        }


        var fish = _this.fish;
        //   print('how many fish do i find?' + fish.length)

        if (fish.length === 0) {
            //    print('no fish...')
            return
        };

        var averageVelocity = {
            x: 0,
            y: 0,
            z: 0
        };

        var averagePosition = {
            x: 0,
            y: 0,
            z: 0
        };


        var userData = JSON.parse(_this.currentProperties.userData);
        var innerContainer = userData['hifi-home-fishtank']['innerContainer'];
        // var bounds = Entities.getEntityProperties(innerContainer, "boundingBox").boundingBox;

        lowerCorner = getOffsetFromTankCenter(LOWER_CORNER_VERTICAL_OFFSET, LOWER_CORNER_FORWARD_OFFSET, LOWER_CORNER_LATERAL_OFFSET);
        upperCorner = getOffsetFromTankCenter(UPPER_CORNER_VERTICAL_OFFSET, UPPER_CORNER_FORWARD_OFFSET, UPPER_CORNER_LATERAL_OFFSET);

        // First pre-load an array with properties  on all the other fish so our per-fish loop
        // isn't doing it. 
        var flockProperties = [];
        for (var i = 0; i < fish.length; i++) {
            var otherProps = Entities.getEntityProperties(fish[i], ["position", "velocity", "rotation"]);
            flockProperties.push(otherProps);
        }

        for (var i = 0; i < fish.length; i++) {
            if (fish[i]) {
                // Get only the properties we need, because that is faster
                var properties = flockProperties[i];
                //  If fish has been deleted, bail
                if (properties.id != fish[i]) {
                    fish[i] = false;
                    return;
                }

                // Store old values so we can check if they have changed enough to update
                var velocity = {
                    x: properties.velocity.x,
                    y: properties.velocity.y,
                    z: properties.velocity.z
                };
                var position = {
                    x: properties.position.x,
                    y: properties.position.y,
                    z: properties.position.z
                };
                averageVelocity = {
                    x: 0,
                    y: 0,
                    z: 0
                };
                averagePosition = {
                    x: 0,
                    y: 0,
                    z: 0
                };

                var othersCounted = 0;
                for (var j = 0; j < fish.length; j++) {
                    if (i != j) {
                        // Get only the properties we need, because that is faster
                        var otherProps = flockProperties[j];
                        var separation = Vec3.distance(properties.position, otherProps.position);
                        if (separation < MAX_SIGHT_DISTANCE) {
                            averageVelocity = Vec3.sum(averageVelocity, otherProps.velocity);
                            averagePosition = Vec3.sum(averagePosition, otherProps.position);
                            othersCounted++;
                        }
                        if (separation < MIN_SEPARATION) {
                            var pushAway = Vec3.multiply(Vec3.normalize(Vec3.subtract(properties.position, otherProps.position)), AVOIDANCE_FORCE);
                            velocity = Vec3.sum(velocity, pushAway);
                        }
                    }
                }

                if (othersCounted > 0) {
                    averageVelocity = Vec3.multiply(averageVelocity, 1.0 / othersCounted);
                    averagePosition = Vec3.multiply(averagePosition, 1.0 / othersCounted);
                    //  Alignment: Follow group's direction and speed
                    velocity = Vec3.mix(velocity, Vec3.multiply(Vec3.normalize(averageVelocity), Vec3.length(velocity)), ALIGNMENT_FORCE);
                    // Cohesion: Steer towards center of flock
                    var towardCenter = Vec3.subtract(averagePosition, position);
                    velocity = Vec3.mix(velocity, Vec3.multiply(Vec3.normalize(towardCenter), Vec3.length(velocity)), COHESION_FORCE);

                    //attractors
                    //[position, radius, force]



                }

                if (_this.hasLookAttractor === true) {
                    //print('has a look attractor, so use it')
                    var attractorPosition = _this.lookAttractor.position;
                    var towardAttractor = Vec3.subtract(attractorPosition, position);
                    velocity = Vec3.mix(velocity, Vec3.multiply(Vec3.normalize(towardAttractor), Vec3.length(velocity)), LOOK_ATTRACTOR_FORCE);
                }

                //  Try to swim at a constant speed
                velocity = Vec3.mix(velocity, Vec3.multiply(Vec3.normalize(velocity), SWIMMING_SPEED), SWIMMING_FORCE);

                //  Keep fish in their 'tank' 
                if (position.x < lowerCorner.x) {
                    position.x = lowerCorner.x;
                    velocity.x *= -0.15
                } else if (position.x > upperCorner.x) {
                    position.x = upperCorner.x;
                    velocity.x *= -0.15
                }
                if (position.y < lowerCorner.y) {
                    position.y = lowerCorner.y;
                    velocity.y *= -0.15
                } else if (position.y > upperCorner.y) {
                    position.y = upperCorner.y;
                    velocity.y *= -0.15
                }
                if (position.z < lowerCorner.z) {
                    position.z = lowerCorner.z;
                    velocity.z *= -0.15
                } else if (position.z > upperCorner.z) {
                    position.z = upperCorner.z;
                    velocity.z *= -0.15
                }

                //  Orient in direction of velocity 
                var rotation = Quat.rotationBetween(Vec3.UNIT_NEG_Z, velocity);

                var mixedRotation = Quat.mix(properties.rotation, rotation, VELOCITY_FOLLOW_RATE);
                var VELOCITY_FOLLOW_RATE = 0.30;

                var slerpedRotation = Quat.slerp(properties.rotation, rotation, VELOCITY_FOLLOW_RATE);

                var safeEuler = Quat.safeEulerAngles(rotation);
                safeEuler.z = safeEuler.z *= 0.925;

                //note: a we want the fish to both rotate toward its velocity and not roll over, and also not pitch more than 30 degrees positive or negative (not doing that last bit right quite yet)
                var newQuat = Quat.fromPitchYawRollDegrees(safeEuler.x, safeEuler.y, safeEuler.z);

                var finalQuat = Quat.slerp(slerpedRotation, newQuat, 0.5);


                //  Only update properties if they have changed, to save bandwidth
                var MIN_POSITION_CHANGE_FOR_UPDATE = 0.001;
                if (Vec3.distance(properties.position, position) < MIN_POSITION_CHANGE_FOR_UPDATE) {
                    Entities.editEntity(fish[i], {
                        velocity: velocity,
                        rotation: finalQuat
                    });
                } else {
                    Entities.editEntity(fish[i], {
                        position: position,
                        velocity: velocity,
                        rotation: finalQuat
                    });
                }
            }
        }
    }

    var STARTING_FRACTION = 0.25;

    function loadFish(howMany) {
        // print('LOADING FISH: ' + howMany)

        var center = _this.currentProperties.position;

        lowerCorner = {
            x: center.x - (_this.currentProperties.dimensions.z / 2),
            y: center.y,
            z: center.z - (_this.currentProperties.dimensions.z / 2)
        };
        upperCorner = {
            x: center.x + (_this.currentProperties.dimensions.z / 2),
            y: center.y + _this.currentProperties.dimensions.y,
            z: center.z + (_this.currentProperties.dimensions.z / 2)
        };

        var fish = [];

        for (var i = 0; i < howMany; i++) {
            var position = {
                x: lowerCorner.x + (upperCorner.x - lowerCorner.x) / 2.0 + (Math.random() - 0.5) * (upperCorner.x - lowerCorner.x) * STARTING_FRACTION,
                y: lowerCorner.y + (upperCorner.y - lowerCorner.y) / 2.0 + (Math.random() - 0.5) * (upperCorner.y - lowerCorner.y) * STARTING_FRACTION,
                z: lowerCorner.z + (upperCorner.z - lowerCorner.z) / 2.0 + (Math.random() - 0.5) * (upperCorner.z - lowerCorner.z) * STARTING_FRACTION
            };


            fish.push(
                Entities.addEntity({
                    name: 'hifi-fishtank-fish' + _this.entityID,
                    type: "Model",
                    modelURL: fish.length % 2 === 0 ? FISH_MODEL_URL : FISH_MODEL_TWO_URL,
                    position: position,
                    parentID: _this.entityID,
                    // rotation: {
                    //     x: 0,
                    //     y: 0,
                    //     z: 0,
                    //     w: 1
                    // },
                    // type: "Box",
                    dimensions: FISH_DIMENSIONS,
                    velocity: {
                        x: SWIMMING_SPEED,
                        y: SWIMMING_SPEED,
                        z: SWIMMING_SPEED
                    },
                    damping: FISH_DAMPING,
                    angularDamping: FISH_ANGULAR_DAMPING,
                    dynamic: false,
                    // lifetime: LIFETIME,
                    color: {
                        red: 0,
                        green: 255,
                        blue: 255
                    }
                })
            );


        }
        // print('initial fish::' + fish.length)
        _this.tankLocked = false;
    }

    Script.scriptEnding.connect(function() {
        Script.update.disconnect(_this.update);
    })


    function setEntityUserData(id, data) {
        var json = JSON.stringify(data)
        Entities.editEntity(id, {
            userData: json
        });
    }

    // FIXME do non-destructive modification of the existing user data
    function getEntityUserData(id) {
        var results = null;
        var properties = Entities.getEntityProperties(id, "userData");
        if (properties.userData) {
            try {
                results = JSON.parse(properties.userData);
            } catch (err) {
                //   print('error parsing json');
                //   print('properties are:'+ properties.userData);
            }
        }
        return results ? results : {};
    }


    // Non-destructively modify the user data of an entity.
    function setEntityCustomData(customKey, id, data) {
        var userData = getEntityUserData(id);
        if (data == null) {
            delete userData[customKey];
        } else {
            userData[customKey] = data;
        }
        setEntityUserData(id, userData);
    }

    function getEntityCustomData(customKey, id, defaultValue) {
        var userData = getEntityUserData(id);
        if (undefined != userData[customKey]) {
            return userData[customKey];
        } else {
            return defaultValue;
        }
    }

    return new FishTank();
});