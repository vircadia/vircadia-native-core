//
//  reset.js
//
//  Created by James B. Pollack @imgntn on 3/14/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This cleanups up and creates content for the home.
//  To begin, it finds any entity with the home reset key in its user data and deletes it.
//  Next, it creates 'scripted entities', or objects that have scripts attached to them.
//  Then it creates 'kinetic entities', or objects that need to be reset but have no scripts attached.
//  Finally it sets up the 'dressing room' - an area that turns small models into big models on collision with a platform.
//
//  The code is organized into a folder for each scripted entity containing a 'wrapper' file that instantiates the entity at a given location and rotation;
//  The rest of the folder contains the scripts needed for that entity.
//  Additionally, there is a folder for the kinetic entities that contains JSON files describing their content, and a single 'wrapper' file that contains constructors for each the various objects.
//  A utilities file and the virtualBaton are included at the root level for all scripts to access as necessary.
//  
// 
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this;

    function Reset() {
        _this = this;
    }

    var utilsPath = Script.resolvePath('utils.js');

    var kineticPath = Script.resolvePath("atp:/kineticObjects/wrapper.js");

    var fishTankPath = Script.resolvePath('atp:/fishTank/wrapper.js');

    var tiltMazePath = Script.resolvePath("atp:/tiltMaze/wrapper.js")

    var whiteboardPath = Script.resolvePath("atp:/whiteboard/wrapper.js");

    var cuckooClockPath = Script.resolvePath("atp:/cuckooClock/wrapper.js");

    var pingPongGunPath = Script.resolvePath("atp:/pingPongGun/wrapper.js");

    var transformerPath = Script.resolvePath("atp:/dressingRoom/wrapper.js");

    var blockyPath = Script.resolvePath("atp:/blocky/wrapper.js");

    Script.include(utilsPath);

    Script.include(kineticPath);

    Script.include(fishTankPath);
    Script.include(tiltMazePath);
    Script.include(whiteboardPath);
    Script.include(cuckooClockPath);
    Script.include(pingPongGunPath);
    Script.include(transformerPath);
    Script.include(blockyPath);

    var TRANSFORMER_URL_ROBOT = 'atp:/dressingRoom/simple_robot.fbx';

    var TRANSFORMER_URL_BEING_OF_LIGHT = 'atp:/dressingRoom/being_of_light.fbx';

    var TRANSFORMER_URL_WILL = 'atp:/dressingRoom/will_T.fbx';

    var TRANSFORMER_URL_STYLIZED_FEMALE = 'atp:/dressingRoom/ArtemisJacketOn.fbx';

    var TRANSFORMER_URL_PRISCILLA = 'atp:/dressingRoom/priscilla.fbx';

    var TRANSFORMER_URL_MATTHEW = 'atp:/dressingRoom/matthew.fbx';

    var BEAM_TRIGGER_THRESHOLD = 0.075;

    var BEAM_POSITION = {
        x: 1103.3876,
        y: 460.4591,
        z: -74.2998
    };

    Reset.prototype = {
        tidying: false,
        eyesOn: false,
        flickerInterval: null,
        preload: function(entityID) {
            _this.entityID = entityID;
            _this.tidySound = SoundCache.getSound("atp:/sounds/tidy.wav");
            Script.update.connect(_this.update);
        },

        showTidyingButton: function() {
            var data = {
                "Texture": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Tidy-Swipe-Sticker.jpg",
                "Texture.001": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Head-Housing-Texture.png",
                "Texture.003": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Tidy-Swipe-Sticker-Emit.jpg",
                "tex.button.blanks": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Button-Blanks.png",
                "tex.button.blanks.normal": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Button-Blanks-Normal.png",
                "tex.face.sceen.default": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/face-default.jpg",
                "tex.face.screen-active": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/face-active.jpg",
                "tex.glow.active": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/glow-active.png",
                "tex.glow.default": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/glow-active.png"
            };


            Entities.editEntity(_this.entityID, {
                textures: JSON.stringify(data)
            });
        },

        tidyGlowActive: function() {
            var res = Entities.findEntities(MyAvatar.position, 30);
            var glow = null;

            res.forEach(function(item) {
                var props = Entities.getEntityProperties(item);
                if (props.name === "Tidyguy-Glow") {
                    glow = item;
                }
            });

            if (glow !== null) {
                Entities.editEntity(glow, {
                    color: {
                        red: 255,
                        green: 140,
                        blue: 20
                    }
                });
            }
        },

        tidyGlowDefault: function() {
            var res = Entities.findEntities(MyAvatar.position, 30);
            var glow = null;

            res.forEach(function(item) {
                var props = Entities.getEntityProperties(item);
                if (props.name === "Tidyguy-Glow") {
                    glow = item;
                }

            });

            if (glow !== null) {
                Entities.editEntity(glow, {
                    color: {
                        red: 92,
                        green: 255,
                        blue: 209
                    }
                });
            }
        },

        showTidyButton: function() {
            var data = {
                "Texture": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Tidy-Swipe-Sticker.jpg",
                "Texture.001": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Head-Housing-Texture.png",
                "Texture.003": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Tidy-Swipe-Sticker-Emit.jpg",
                "tex.button.blanks": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Button-Blanks.png",
                "tex.button.blanks.normal": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Button-Blanks-Normal.png",
                "tex.face.sceen.default": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/face-default.jpg",
                "tex.face.screen-active": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/face-active.jpg",
                "tex.glow.active": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/glow-active.png",
                "tex.glow.default": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/glow-default.png"
            };

            Entities.editEntity(_this.entityID, {
                textures: JSON.stringify(data)
            });
        },

        playTidyingSound: function() {
            var location = {
                x: 1102.7660,
                y: 460.9814,
                z: -78.6451
            };

            var injector = Audio.playSound(_this.tidySound, {
                volume: 1,
                position: location
            });
        },

        toggleButton: function() {
            if (_this.tidying === true) {
                return;
            } else {
                _this.tidying = true;
                _this.showTidyingButton();
                _this.tidyGlowActive();
                _this.playTidyingSound();
                _this.flickerEyes();
                _this.findAndDeleteHomeEntities();
                Script.setTimeout(function() {
                    _this.showTidyButton();
                    _this.tidyGlowDefault();
                    _this.tidying = false;
                }, 2500);

                Script.setTimeout(function() {
                    _this.createKineticEntities();
                    _this.createScriptedEntities();
                    _this.setupDressingRoom();
                    _this.createMilkPailBalls();
                    _this.createTarget();
                }, 750);
            }
        },

        flickerEyes: function() {
            this.flickerInterval = Script.setInterval(function() {
                if (_this.eyesOn === true) {
                    _this.turnEyesOff();
                    _this.eyesOn = false;
                } else if (_this.eyesOn === false) {
                    _this.turnEyesOn();
                    _this.eyesOn = true
                }
            }, 100);

            Script.setTimeout(function() {
                Script.clearInterval(_this.flickerInterval);
            }, 2500);
        },

        turnEyesOn: function() {
            var data = {
                "Texture": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Tidy-Swipe-Sticker.jpg",
                "Texture.001": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Head-Housing-Texture.png",
                "Texture.003": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Tidy-Swipe-Sticker-Emit.jpg",
                "tex.button.blanks": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Button-Blanks.png",
                "tex.button.blanks.normal": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Button-Blanks-Normal.png",
                "tex.face.sceen.default": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/face-active.jpg",
                "tex.face.screen-active": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/face-active.jpg",
                "tex.glow.active": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/glow-active.png",
                "tex.glow.default": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/glow-active.png"
            };

            Entities.editEntity(_this.entityID, {
                textures: JSON.stringify(data)
            });
        },

        turnEyesOff: function() {
            var data = {
                "Texture": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Tidy-Swipe-Sticker.jpg",
                "Texture.001": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Head-Housing-Texture.png",
                "Texture.003": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Tidy-Swipe-Sticker-Emit.jpg",
                "tex.button.blanks": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Button-Blanks.png",
                "tex.button.blanks.normal": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/Button-Blanks-Normal.png",
                "tex.face.sceen.default": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/face-default.jpg",
                "tex.face.screen-active": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/face-active.jpg",
                "tex.glow.active": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/glow-active.png",
                "tex.glow.default": "atp:/Tidyguy-4d.fbx/Tidyguy-4d.fbm/glow-active.png"
            };

            Entities.editEntity(_this.entityID, {
                textures: JSON.stringify(data)
            });
        },

        clickReleaseOnEntity: function(entityID, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            _this.toggleButton();
        },

        findAndDeleteHomeEntities: function() {
            print('HOME trying to find home entities to delete')
            var resetProperties = Entities.getEntityProperties(_this.entityID);
            var results = Entities.findEntities(resetProperties.position, 1000);
            var found = [];
            results.forEach(function(result) {
                var properties = Entities.getEntityProperties(result);
                if (properties.userData === "" || properties.userData === undefined) {
                    print('no userdata -- its blank or undefined')
                    return;
                }
                var userData = null;
                try {
                    userData = JSON.parse(properties.userData);
                } catch (err) {
                    print('error parsing json in resetscript for: ' + properties.name);
                    //print('properties are:' + properties.userData);
                    return;
                }
                if (userData.hasOwnProperty('hifiHomeKey')) {
                    if (userData.hifiHomeKey.reset === true) {
                        Entities.deleteEntity(result);
                    }
                }
            })

            print('HOME after deleting home entities');
        },

        createScriptedEntities: function() {
            var fishTank = new FishTank({
                x: 1099.2200,
                y: 460.5460,
                z: -78.2363
            }, {
                x: 0,
                y: 0,
                z: 0
            });

            var tiltMaze = new TiltMaze({
                x: 1105.5768,
                y: 460.3298,
                z: -80.4891
            });

            var whiteboard = new Whiteboard({
                x: 1105.0955,
                y: 460.5000,
                z: -77.4409
            }, {
                x: -0.0013,
                y: -133.0056,
                z: -0.0013
            });

            var pingPongGun = new HomePingPongGun({
                x: 1101.2123,
                y: 460.2328,
                z: -65.8513
            }, {
                x: 97.3683,
                y: 179.0293,
                z: 89.9698
            });

            var cuckooClock = new MyCuckooClock({
                x: 1105.5237,
                y: 461.4826,
                z: -81.7524
            }, {
                x: -0.0013,
                y: -57.0089,
                z: -0.0013
            });

            var blocky = new BlockyGame({
                x: 1098.4424,
                y: 460.3090,
                z: -66.2190
            })

            print('HOME after creating scripted entities')

        },

        createKineticEntities: function() {

            var fruitBowl = new FruitBowl({
                x: 1105.3185,
                y: 460.3221,
                z: -81.2452
            });

            var livingRoomLamp = new LivingRoomLamp({
                x: 1104.6732,
                y: 460.3326,
                z: -81.9710
            });

            var upperBookShelf = new UpperBookShelf({
                x: 1106.2649,
                y: 461.5352,
                z: -80.3018
            });

            var lowerBookShelf = new LowerBookShelf({
                x: 1106.2725,
                y: 460.9600,
                z: -80.2837
            });

            var deskChair = new Chair({
                x: 1105.2716,
                y: 459.7251,
                z: -79.8097
            });


            var stuffOnShelves = new StuffOnShelves({
                x: 1105.9432,
                y: 461.095,
                z: -80.7894
            });

            var junk = new HomeJunk({
                x: 1102.5861,
                y: 460.1812,
                z: -69.5005
            });

            var trashcan = new Trashcan({
                x: 1103.9034,
                y: 459.4355,
                z: -82.3619
            });

            var books = new Books({
                x: 1106.1553,
                y: 461.1,
                z: -80.4890
            });

            var cellPoster = new PosterCell({
                x: 1103.78,
                y: 461,
                z: -70.3
            });

            var playaPoster = new PosterPlaya({
                x: 1101.8,
                y: 461,
                z: -73.3
            });

            // var dressingRoomBricabrac = new Bricabrac({
            //     x: 1106.8,
            //     y: 460.3909,
            //     z: -72.6
            // });

            var bench = new Bench({
                x: 1100.1210,
                y: 459.4552,
                z: -75.4537
            });

            var umbrella = new Umbrella({
                x: 1097.5510,
                y: 459.5230,
                z: -84.3897
            });

            print('HOME after creating kinetic entities');
        },

        setupDressingRoom: function() {
            print('HOME setup dressing room');
            this.createRotatorBlock();
            this.createTransformingDais();
            this.createTransformers();
        },

        createRotatorBlock: function() {
            var rotatorBlockProps = {
                name: 'hifi-home-dressing-room-rotator-block',
                type: 'Box',
                visible: false,
                color: {
                    red: 0,
                    green: 255,
                    blue: 0
                },
                dimensions: {
                    x: 1.0000,
                    y: 0.0367,
                    z: 1.0000
                },
                collisionless: true,
                angularDamping: 0,
                angularVelocity: {
                    x: 0,
                    y: 0.10472,
                    z: 0
                },
                dynamic: false,
                userData: JSON.stringify({
                    'hifiHomeKey': {
                        'reset': true
                    }
                }),
                position: {
                    x: 1106.9778,
                    y: 460.2765,
                    z: -74.5842
                },
                userData: JSON.stringify({
                    'hifiHomeKey': {
                        'reset': true
                    }
                })
            }

            var rotatorBlock = Entities.addEntity(rotatorBlockProps);
            print('HOME created rotator block');
        },

        createTransformingDais: function() {
            var DAIS_MODEL_URL = 'atp:/dressingRoom/Dressing-Dais.fbx';
            var COLLISION_HULL_URL = 'atp:/dressingRoom/Dressing-Dais.obj';

            var DAIS_DIMENSIONS = {
                x: 1.0654,
                y: 0.1695,
                z: 1.0654
            };

            var DAIS_POSITION = {
                x: 1107.0330,
                y: 459.2782,
                z: -74.5704
            };

            var daisProperties = {
                name: 'hifi-home-dressing-room-transformer-collider',
                type: 'Model',
                modelURL: DAIS_MODEL_URL,
                dimensions: DAIS_DIMENSIONS,
                compoundShapeURL: COLLISION_HULL_URL,
                position: DAIS_POSITION,
                dynamic: false,
                userData: JSON.stringify({
                    'hifiHomeKey': {
                        'reset': true
                    }
                }),
            };

            var dais = Entities.addEntity(daisProperties);
        },

        createTarget: function() {
            var targetProperties = {
                type: 'Model',
                modelURL: 'atp:/pingPongGun/Target.fbx',
                shapeType: 'Compound',
                compoundShapeURL: 'atp:/pingPongGun/Target.obj',
                dimensions: {
                    x: 0.4937,
                    y: 0.6816,
                    z: 0.0778
                },
                rotation: Quat.fromPitchYawRollDegrees(3.1471, -170.4121, -0.0060),
                gravity: {
                    x: 0,
                    y: -9.8,
                    z: 0
                },
                velocity: {
                    x: 0,
                    y: -0.1,
                    z: 0
                },
                position: {
                    x: 1100.6343,
                    y: 460.5366,
                    z: -65.2142
                },
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: true
                    },
                    hifiHomeKey: {
                        reset: true
                    }
                }),
                script: 'atp:/pingPongGun/target.js',
                density: 100,
                dynamic: true
            }
            var target = Entities.addEntity(targetProperties);
        },

        createMilkPailBalls: function() {
            var locations = [{
                x: 1099.0795,
                y: 459.4186,
                z: -70.8603
            }, {
                x: 1099.2826,
                y: 459.4186,
                z: -70.9094
            }, {
                x: 1099.5012,
                y: 459.4186,
                z: -71.1000
            }];

            var ballProperties = {
                type: 'Model',
                modelURL: 'atp:/static_objects/StarBall.fbx',
                shapeType: 'Sphere',
                dimensions: {
                    x: 0.1646,
                    y: 0.1646,
                    z: 0.1646
                },
                gravity: {
                    x: 0,
                    y: -9.8,
                    z: 0
                },
                velocity: {
                    x: 0,
                    y: -0.1,
                    z: 0
                },
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: true
                    },
                    hifiHomeKey: {
                        reset: true
                    }
                }),
                dynamic: true
            };

            locations.forEach(function(location) {
                ballProperties.position = location;
                var ball = Entities.addEntity(ballProperties);
            });
            print('HOME made milk pail balls')
        },

        createTransformers: function() {
            var firstDollPosition = {
                x: 1107.6,
                y: 460.575,
                z: -77.37
            }

            var dollRotation = {
                x: 0,
                y: -55.86,
                z: 0,
            }

            var rotationAsQuat = Quat.fromPitchYawRollDegrees(dollRotation.x, dollRotation.y, dollRotation.z);

            var dolls = [
                TRANSFORMER_URL_ROBOT,
                TRANSFORMER_URL_BEING_OF_LIGHT,
                TRANSFORMER_URL_STYLIZED_FEMALE,
                TRANSFORMER_URL_WILL,
                TRANSFORMER_URL_PRISCILLA,
                TRANSFORMER_URL_MATTHEW
            ];

            var dollDimensions = [{
                //robot
                x: 1.4439,
                y: 0.6224,
                z: 0.4998
            }, {
                //being of light
                x: 1.8838,
                y: 1.7865,
                z: 0.2955
            }, {
                //stylized female artemis
                x: 1.6323,
                y: 1.7705,
                z: 0.2851
            }, {
                //will
                x: 1.6326,
                y: 1.6764,
                z: 0.2606
            }, {
                //priscilla
                x: 1.6448,
                y: 1.6657,
                z: 0.3078
            }, {
                //matthew
                x: 1.8722,
                y: 1.8197,
                z: 0.3666
            }];

            var TRANSFORMER_SCALE = 0.25;

            dollDimensions.forEach(function(vector, index) {
                var scaled = Vec3.multiply(vector, TRANSFORMER_SCALE);
                dollDimensions[index] = scaled;
            })

            var dollLateralSeparation = 0.8;

            dolls.forEach(function(doll, index) {
                var separation = index * dollLateralSeparation;
                var left = Quat.getRight(rotationAsQuat);
                var distanceToLeft = Vec3.multiply(separation, left);
                var dollPosition = Vec3.sum(firstDollPosition, distanceToLeft);
                if (index === 0) {
                    //special case for robot
                    dollPosition.y += 0.15;
                }
                var transformer = new TransformerDoll(doll, dollPosition, dollRotation,
                    dollDimensions[index]);
            });

        },

        update: function() {
            if (_this.tidying === true) {
                return;
            }

            var leftHandPosition = MyAvatar.getLeftPalmPosition();
            var rightHandPosition = MyAvatar.getRightPalmPosition();

            var rightDistance = Vec3.distance(leftHandPosition, BEAM_POSITION)
            var leftDistance = Vec3.distance(rightHandPosition, BEAM_POSITION)

            if (rightDistance < BEAM_TRIGGER_THRESHOLD || leftDistance < BEAM_TRIGGER_THRESHOLD) {
                _this.toggleButton();
            }
        },

        unload: function() {
            Script.update.disconnect(_this.update);
            Script.clearInterval(_this.flickerInterval);
        }

    }
    return new Reset();
});