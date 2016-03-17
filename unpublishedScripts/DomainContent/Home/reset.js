//
//  reset.js
//
//  Created by James B. Pollack @imgntn on 3/14/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This cleanups up and creates content for the home.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this;

    var dynamicEntities = [];
    var kineticEntities = [];

    function Reset() {
        _this = this;
    }

    var utilsPath = Script.resolvePath('utils.js');

    var fishTankPath = Script.resolvePath('fishTank/wrapper.js?' + Math.random());

    var tiltMazePath = Script.resolvePath("tiltMaze/wrapper.js?" + Math.random())

    var whiteboardPath = Script.resolvePath("whiteboard/wrapper.js?" + Math.random());

    var plantPath = Script.resolvePath("growingPlant/wrapper.js?" + Math.random());

    var pingPongGunPath = Script.resolvePath("pingPongGun/wrapper.js?" + Math.random());

    var kineticPath = Script.resolvePath("kineticObjects/wrapper.js?" + Math.random());

    Script.include(kineticPath);

    var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
        x: 0,
        y: 0.5,
        z: 0
    }), Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));

    Reset.prototype = {
        tidying: false,

        preload: function(entityID) {
            _this.entityID = entityID;
        },

        showTidyingButton: function() {
            var textureString =
                'Texture.001:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/Head-Housing-Texture.png",\ntex.face.screen.emit:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/tidy-guy-face-Emit.png",\ntex.face.sceen:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/tidy-guy-face.png",\ntex.button.blanks:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/Button-Blanks.png",\ntex.button.blanks.normal:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/Button-Blanks-Normal.png",\nbutton.tidy.emit:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidy-Up-Button-Orange-Emit.png",\nbutton.tidy:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidy-Up-Button-Orange.png"'

            Entities.editEntity(_this.entityID, {
                textures: textureString
            });
        },

        showTidyButton: function() {
            var textureString =
                'Texture.001:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/Head-Housing-Texture.png",\ntex.face.screen.emit:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/tidy-guy-face-Emit.png",\ntex.face.sceen:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/tidy-guy-face.png",\ntex.button.blanks:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/Button-Blanks.png",\ntex.button.blanks.normal:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/Button-Blanks-Normal.png",\nbutton.tidy.emit:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/Tidy-Up-Button-Green-Emit.png",\nbutton.tidy:"http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx/Tidyguy-6.fbm/Tidy-Up-Button-Green.png"'

            Entities.editEntity(_this.entityID, {
                textures: textureString
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
                Script.setTimeout(function() {
                    _this.showTidyButton();
                    _this.tidying = false;
                }, 2500);
                _this.cleanupDynamicEntities();
                _this.cleanupKineticEntities();
                _this.createKineticEntities();
                _this.createDynamicEntities();

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

        createDynamicEntities: function() {
 
            Script.include(utilsPath);
            Script.include(fishTankPath);
            Script.include(tiltMazePath);
            Script.include(whiteboardPath);
            Script.include(plantPath);
            Script.include(pingPongGunPath);

            var fishTank = new FishTank({
                x: 1098.9254,
                y: 460.5814,
                z: -79.1103
            },{
                x:0,
                y:152,
                z:0
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
            },{
                x:0,
                y:-133,
                z:0
            });

            var myPlant = new Plant({
                x: 1099.8785,
                y: 460.3115,
                z: -84.7736
            });

            var pingPongGun = new _PingPongGun({
                x: 1101.2123,
                y: 460.2328,
                z: -65.8513
            }, {
                x: 97.3683,
                y: 179.0293,
                z: 89.9698
            });



            dynamicEntities.push(fishTank);
            dynamicEntities.push(tiltMaze);
            dynamicEntities.push(whiteboard);
            dynamicEntities.push(myPlant);
            dynamicEntities.push(pingPongGun);
            
            //v2.0
        
            // var musicBox = new MusicBox();
            // var cuckooClock = new CuckooClock();


            // var doppelganger = new Doppelganger();


        },

        cleanupDynamicEntities: function() {
            if (dynamicEntities.length === 0) {
                return;
            }
            dynamicEntities.forEach(function(dynamicEntity) {
                dynamicEntity.cleanup();
            })
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

            var rightDeskDrawer = new RightDeskDrawer({
                x: 1105.1735,
                y: 460.0446,
                z: -81.3612
            });

            var leftDeskDrawer = new LeftDeskDrawer({
                x: 1104.6478,
                y: 460.0463,
                z: -82.1095
            });

            var chair = new Chair({
                x: 1105.2716,
                y: 459.7251,
                z: -79.8097
            });

            var trashcan = new Trashcan({
                x: 1106.2725,
                y: 459.4355,
                z: -82.7294
            });

            var books = new Books({
                x: 1106.1553,
                y: 461.0825,
                z: -80.4890
            });

            kineticEntities.push(fruitBowl);
            kineticEntities.push(livingRoomLamp);
            kineticEntities.push(upperBookShelf);
            kineticEntities.push(lowerBookShelf);
            kineticEntities.push(rightDeskDrawer);
            kineticEntities.push(leftDeskDrawer);
            kineticEntities.push(chair);
            kineticEntities.push(trashcan);
            kineticEntities.push(books);
        },

        cleanupKineticEntities: function() {
            if (kineticEntities.length === 0) {
                return;
            }
            kineticEntities.forEach(function(kineticEntity) {
                kineticEntity.cleanup();
            })
        },
        unload: function() {
            this.cleanupDynamicEntities();
            this.cleanupKineticEntities();
        }

    }
    return new Reset();
});