(function() {

    Script.include('../../../../examples/libraries/virtualBaton.js?' + Math.random());
    Script.include('../../../../examples/libraries/utils.js?' + Math.random());

    //only one person should simulate the tank at a time -- we pass around a virtual baton
    var baton;
    var iOwn = false;
    var _entityID;
    var _this;
    var connected = false;

    var TANK_SEARCH_RADIUS = 5;

    var INTERSECT_COLOR = {
        red: 255,
        green: 0,
        blue: 255
    }

    var NO_INTERSECT_COLOR = {
        red: 0,
        green: 255,
        blue: 0
    }

    function FishTank() {
        _this = this;
    }

    function startUpdate() {
        //when the baton is claimed;
        iOwn = true;
        connected = true;
        Script.update.connect(_this.update);
    }

    function stopUpdateAndReclaim() {
        //when the baton is released;
        print('i released the object ' + _entityID)
        iOwn = false;
        if (connected === true) {
            connected = false;
            Script.update.disconnect(_this.update);
        }
        //hook up callbacks to the baton
        baton.claim(startUpdate, stopUpdateAndReclaim);

    }

    FishTank.prototype = {
        fish: null,
        tankLocked: false,

        combinedEntitySearch: function() {
            var results = Entities.findEntities(_this.currentProperties.position, TANK_SEARCH_RADIUS);
            var attractorList = [];
            var fishList = [];
            results.forEach(function(entity) {
                var properties = Entities.getEntityProperties(entity, 'name');
                if (properties.name.indexOf('hifi-fishtank-attractor' + _this.entityID) > -1) {
                    attractorList.push(entity);
                }
                if (properties.name.indexOf('hifi-fishtank-fish' + _this.entityID) > -1) {
                    fishList.push(entity);
                }
            });

        },

        findAttractors: function() {
            var results = Entities.findEntities(_this.currentProperties.position, TANK_SEARCH_RADIUS);
            var attractorList = [];

            results.forEach(function(attractor) {
                var properties = Entities.getEntityProperties(attractor, 'name');
                if (properties.name.indexOf('hifi-fishtank-attractor' + _this.entityID) > -1) {
                    fishList.push(attractor);
                }
            })
        },

        findFishInTank: function() {
            //  print('looking for a fish in the tank')
            var results = Entities.findEntities(_this.currentProperties.position, TANK_SEARCH_RADIUS);
            var fishList = [];

            results.forEach(function(fish) {
                var properties = Entities.getEntityProperties(fish, 'name');
                if (properties.name.indexOf('hifi-fishtank-fish' + _this.entityID) > -1) {
                    fishList.push(fish);
                }
            })

            print('fish? ' + fishList.length)
            return fishList;
        },

        initialize: function(entityID) {
            print('JBP fishtank initialize' + entityID)
            var properties = Entities.getEntityProperties(entityID);
            if (properties.userData.length === 0 || properties.hasOwnProperty('userData') === false) {
                _this.initTimeout = Script.setTimeout(function() {
                    print('JBP no user data yet, try again in one second')
                    _this.initialize(entityID);
                }, 1000)

            } else {

                print('JBP userdata before parse attempt' + properties.userData)
                _this.userData = null;
                try {
                    _this.userData = JSON.parse(properties.userData);
                } catch (err) {
                    print('JBP error parsing json');
                    print('JBP properties are:' + properties.userData);
                    return;
                }
                print('after parse')
                _this.currentProperties = Entities.getEntityProperties(entityID);
                baton = virtualBaton({
                    batonName: 'io.highfidelity.fishtank:' + entityID, // One winner for each entity
                });

                stopUpdateAndReclaim();

            }
        },

        preload: function(entityID) {
            print("preload");
            this.entityID = entityID;
            _entityID = entityID;
            this.initialize(entityID);
            this.initTimeout = null;

        },

        unload: function() {
            print(' UNLOAD')
            Script.update.disconnect(_this.update);
            _this.overlayLineOff();
            if (baton) {
                print('BATON RELEASE ')
                baton.release(function() {});
            }

        },

        update: function() {
            //print('AM I THE OWNER??' + iOwn);
            if (iOwn === false) {
                return
            }
            // print('i am the owner!')
            //do stuff
            updateFish();
            _this.seeIfOwnerIsLookingAtTheTank();
        },

        overlayLine: null,

        overlayLineOn: function(closePoint, farPoint, color) {
            print('closePoint:  ' + JSON.stringify(closePoint));
            print('farPoint:  ' + JSON.stringify(farPoint));
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

        overlayLineDistance: 3,

        seeIfOwnerIsLookingAtTheTank: function() {
            var cameraPosition = Camera.getPosition();
            var cameraOrientation = Camera.getOrientation();

            var front = Quat.getFront(cameraOrientation);
            var pickRay = {
                origin: cameraPosition,
                direction: front
            };

            _this.overlayLineOn(pickRay.origin, Vec3.sum(pickRay.origin, Vec3.multiply(front, _this.overlayLineDistance)), INTERSECT_COLOR);

            var intersection = Entities.findRayIntersection(pickRay, true, [_this.entityID]);

            // print('INTERSCTION::: ' + JSON.stringify(intersection));

            if (intersection.intersects && intersection.entityID === _this.entityID) {
                print('intersecting a tank')
            }
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
        x: 1.3393,
        y: 1.3515,
        z: 3.5914
    };

    var TANK_WIDTH = TANK_DIMENSIONS.z / 3;
    var TANK_HEIGHT = TANK_DIMENSIONS.y / 3;
    var FISH_WIDTH = 0.03;
    var FISH_LENGTH = 0.15;
    var MAX_SIGHT_DISTANCE = 0.8;
    var MIN_SEPARATION = 0.15;
    var AVOIDANCE_FORCE = 0.3;
    var COHESION_FORCE = 0.025;
    var ALIGNMENT_FORCE = 0.025;
    var SWIMMING_FORCE = 0.05;
    var SWIMMING_SPEED = 0.5;

    var THROTTLE = false;
    var THROTTLE_RATE = 100;
    var sinceLastUpdate = 0;

    var FISH_MODEL_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/Fish-1.fbx";

    var FISH_MODEL_TWO_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/Fish-2.fbx";

    var fishLoaded = false;

    function randomVector(scale) {
        return {
            x: Math.random() * scale - scale / 2.0,
            y: Math.random() * scale - scale / 2.0,
            z: Math.random() * scale - scale / 2.0
        };
    }

    function updateFish(deltaTime) {

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


        // print('has userdata fish??' + _this.userData['hifi-home-fishtank'].fishLoaded)


        if (_this.userData['hifi-home-fishtank'].fishLoaded === false) {
            //no fish in the user data
            _this.tankLocked = true;
            loadFish(NUM_FISH);
            setEntityCustomData(FISHTANK_USERDATA_KEY, _this.entityID, {
                fishLoaded: true
            });
            _this.userData['hifi-home-fishtank'].fishLoaded = true;
            _this.fish = _this.findFishInTank();
            return;
        } else {

            //fish in userdata already
            if (_this.fish === null) {
                _this.fish = _this.findFishInTank();
            }

        }


        var fish = _this.fish;
        // print('how many fish do i find?' + fish.length)

        if (fish.length === 0) {
            print('no fish...')
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

        var birdPositionsCounted = 0;
        var birdVelocitiesCounted = 0;
        var center = _this.currentProperties.position;

        lowerCorner = {
            x: center.x - (TANK_WIDTH / 2),
            y: center.y,
            z: center.z - (TANK_WIDTH / 2)
        };
        upperCorner = {
            x: center.x + (TANK_WIDTH / 2),
            y: center.y + TANK_HEIGHT,
            z: center.z + (TANK_WIDTH / 2)
        };

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

                //  Try to swim at a constant speed
                velocity = Vec3.mix(velocity, Vec3.multiply(Vec3.normalize(velocity), SWIMMING_SPEED), SWIMMING_FORCE);

                //  Keep fish in their 'tank' 
                if (position.x < lowerCorner.x) {
                    position.x = lowerCorner.x;
                    velocity.x *= -1.0;
                } else if (position.x > upperCorner.x) {
                    position.x = upperCorner.x;
                    velocity.x *= -1.0;
                }
                if (position.y < lowerCorner.y) {
                    position.y = lowerCorner.y;
                    velocity.y *= -1.0;
                } else if (position.y > upperCorner.y) {
                    position.y = upperCorner.y;
                    velocity.y *= -1.0;
                }
                if (position.z < lowerCorner.z) {
                    position.z = lowerCorner.z;
                    velocity.z *= -1.0;
                } else if (position.z > upperCorner.z) {
                    position.z = upperCorner.z;
                    velocity.z *= -1.0;
                }

                //  Orient in direction of velocity 
                var rotation = Quat.rotationBetween(Vec3.UNIT_NEG_Z, velocity);
                var VELOCITY_FOLLOW_RATE = 0.30;

                //  Only update properties if they have changed, to save bandwidth
                var MIN_POSITION_CHANGE_FOR_UPDATE = 0.001;
                if (Vec3.distance(properties.position, position) < MIN_POSITION_CHANGE_FOR_UPDATE) {
                    Entities.editEntity(fish[i], {
                        velocity: velocity,
                        rotation: Quat.mix(properties.rotation, rotation, VELOCITY_FOLLOW_RATE)
                    });
                } else {
                    Entities.editEntity(fish[i], {
                        position: position,
                        velocity: velocity,
                        rotation: Quat.slerp(properties.rotation, rotation, VELOCITY_FOLLOW_RATE)
                    });
                }
            }
        }
    }

    var STARTING_FRACTION = 0.25;

    function loadFish(howMany) {
        print('LOADING FISH: ' + howMany)

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
                    rotation: {
                        x: 0,
                        y: 0,
                        z: 0,
                        w: 1
                    },
                    // dimensions: {
                    //     x: FISH_WIDTH,
                    //     y: FISH_WIDTH,
                    //     z: FISH_LENGTH
                    // },
                    // velocity: {
                    //     x: SWIMMING_SPEED,
                    //     y: SWIMMING_SPEED,
                    //     z: SWIMMING_SPEED
                    // },
                    damping: 0.1,
                    dynamic: false,
                    lifetime: LIFETIME,
                    color: {
                        red: 0,
                        green: 255,
                        blue: 255
                    }
                })
            );


        }
        print('initial fish::' + fish.length)
        _this.tankLocked = false;
    }

    Script.scriptEnding.connect(function() {
        Script.update.disconnect(_this.update);
    })

    return new FishTank();
});