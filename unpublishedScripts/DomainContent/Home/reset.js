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

    var plantPath = Script.resolvePath("atp:/growingPlant/wrapper.js");

    var cuckooClockPath = Script.resolvePath("atp:/cuckooClock/wrapper.js");

    var pingPongGunPath = Script.resolvePath("atp:/pingPongGun/wrapper.js");

    var musicBoxPath = Script.resolvePath("musicBox/wrapper.js?" + Math.random());

    var transformerPath = Script.resolvePath("atp:/dressingRoom/wrapper.js");

    Script.include(utilsPath);

    Script.include(kineticPath);

    Script.include(fishTankPath);
    Script.include(tiltMazePath);
    Script.include(whiteboardPath);
    Script.include(plantPath);
    Script.include(cuckooClockPath);
    Script.include(pingPongGunPath);
    // Script.include(musicBoxPath);
    Script.include(transformerPath);

    var TRANSFORMER_URL_ROBOT = 'atp:/dressingRoom/simple_robot.fbx';

    var TRANSFORMER_URL_BEING_OF_LIGHT = 'atp:/dressingRoom/being_of_light.fbx';

    var TRANSFORMER_URL_WILL = 'atp:/dressingRoom/will_T.fbx';

    var TRANSFORMER_URL_STYLIZED_FEMALE = 'atp:/dressingRoom/stylized_female.fbx';

    var TRANSFORMER_URL_REALISTIC_MALE = '';

    var TRANSFORMER_URL_REALISTIC_FEMALE = '';

    Reset.prototype = {
        tidying: false,

        preload: function(entityID) {
            _this.entityID = entityID;
        },

        showTidyingButton: function() {
            var data = {
                "Texture.001": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Head-Housing-Texture.png",
                "button.tidy": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Tidy-Up-Button-Orange.png",
                "button.tidy-active": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Tidy-Up-Button-Orange.png",
                "button.tidy-active.emit": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Tidy-Up-Button-Orange-Emit.png",
                "button.tidy.emit": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Tidy-Up-Button-Orange-Emit.png",
                "tex.button.blanks": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Button-Blanks.png",
                "tex.button.blanks.normal": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Button-Blanks-Normal.png",
                "tex.face.sceen": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/tidy-guy-face.png",
                "tex.face.screen.emit": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/tidy-guy-face-Emit.png"
            }

            Entities.editEntity(_this.entityID, {
                textures: JSON.stringify(data)
            });
        },

        showTidyButton: function() {
            var data = {
                "Texture.001": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Head-Housing-Texture.png",
                "button.tidy": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Tidy-Up-Button-Green.png",
                "button.tidy.emit": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Tidy-Up-Button-Green-Emit.png",
                "tex.button.blanks": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Button-Blanks.png",
                "tex.button.blanks.normal": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/Button-Blanks-Normal.png",
                "tex.face.sceen": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/tidy-guy-face.png",
                "tex.face.screen.emit": "atp:/Tidyguy-7.fbx/Tidyguy-7.fbm/tidy-guy-face-Emit.png"
            }


            Entities.editEntity(_this.entityID, {
                textures: JSON.stringify(data)
            });
        },

        playTidyingSound: function() {

        },

        toggleButton: function() {
            if (_this.tidying === true) {
                return;
            } else {
                _this.tidying = true;
                _this.showTidyingButton();
                _this.playTidyingSound();

                _this.findAndDeleteHomeEntities();
                Script.setTimeout(function() {
                    _this.showTidyButton();
                    _this.tidying = false;
                }, 2500);

                Script.setTimeout(function() {
                    _this.createKineticEntities();
                    _this.createScriptedEntities();
                    _this.setupDressingRoom();
                }, 750)


            }
        },

        clickReleaseOnEntity: function(entityID, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            _this.toggleButton();

        },

        startNearTrigger: function() {
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
            print('HOME after deleting home entities')
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
                x: 1104,
                y: 460.5,
                z: -77
            }, {
                x: 0,
                y: -133,
                z: 0
            });

            var myPlant = new Plant({
                x: 1099.8785,
                y: 460.3115,
                z: -84.7736
            }, {
                x: 0,
                y: 0,
                z: 0
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
                x: 1105.267,
                y: 461.44,
                z: -81.9495
            }, {
                x: 0,
                y: -57,
                z: 0
            });

            // var musicBox = new MusicBox();
            print('HOME after creating scripted entities')

        },

        createKineticEntities: function() {

            var blocks = new Blocks({
                x: 1097.1383,
                y: 460.3790,
                z: -66.4895
            });

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

            var blueChair = new BlueChair({
                x: 1100.4821,
                y: 459.9147,
                z: -75.9071
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
            print('HOME after creating kinetic entities')
        },

        setupDressingRoom: function() {
            print('HOME setup dressing room')
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
            print('HOME created rotator block')
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
            print('HOME created dais : ' + dais)
        },

        createTransformers: function() {
            var firstDollPosition = {
                x: 1107.61,
                y: 460.6,
                z: -77.34
            }

            var dollRotation = {
                x: 0,
                y: -55.86,
                z: 0,
            }

            var rotationAsQuat = Quat.fromPitchYawRollDegrees(dollRotation.x, dollRotation.y, dollRotation.z);

            var dolls = [
                TRANSFORMER_URL_STYLIZED_FEMALE,
                TRANSFORMER_URL_ROBOT,
                TRANSFORMER_URL_BEING_OF_LIGHT,
                TRANSFORMER_URL_WILL
            ];

            var dollDimensions = [{
                //stylized female artemis
                x: 1.6323,
                y: 1.7705,
                z: 0.2851
            }, {
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
                //will
                x: 1.6326,
                y: 1.6764,
                z: 0.2606
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
                var dollPosition = Vec3.sum(firstDollPosition, distanceToLeft)
                var transformer = new TransformerDoll(doll, dollPosition, dollRotation,
                    dollDimensions[index]);
            });

        },

        unload: function() {
            // this.findAndDeleteHomeEntities();
        }

    }
    return new Reset();
});