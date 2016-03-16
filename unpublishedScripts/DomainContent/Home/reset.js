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

    function Reset() {
        _this = this;
    };

    var utilsPath = Script.resolvePath('utils.js');

    var fishTankPath = Script.resolvePath('fishTank/wrapper.js?' + Math.random());

    var tiltMazePath = Script.resolvePath("tiltMaze/wrapper.js?" + Math.random())

    var whiteboardPath = Script.resolvePath("whiteboard/wrapper.js?" + Math.random());

    var plantPath = Script.resolvePath("growingPlant/wrapper.js?" + Math.random());

    var cuckooClockPath = Script.resolvePath("cuckooClock/wrapper.js?" + Math.random());

    Script.include(utilsPath);
    Script.include(fishTankPath);
    Script.include(tiltMazePath);
    Script.include(whiteboardPath);
    Script.include(plantPath);
    Script.include(cuckooClockPath);
    
    Reset.prototype = {
        preload: function(entityID) {
            _this.entityID = entityID;
        },
        unload: function() {
            this.cleanupDynamicEntities();
        },
        tidying: false,
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
            var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
                x: 0,
                y: 0.5,
                z: 0
            }), Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));



            // var fishTank = new FishTank(center);
            // var tiltMaze = new TiltMaze(center);
            // var whiteboard = new Whiteboard(center);
            // var myPlant = new Plant(center);

            // dynamicEntities.push(fishTank);
            // dynamicEntities.push(tiltMaze);
            // dynamicEntities.push(whiteboard);
            // dynamicEntities.push(myPlant);

            //v2.0
            // var musicBox = new MusicBox();
            var cuckooClock = new MyCuckooClock(center);
            // var doppelganger = new Doppelganger();
            
            dynamicEntities.push(cuckooClock);
        },

        cleanupDynamicEntities: function() {
            if (dynamicEntities.length === 0) {
                return;
            }
            dynamicEntities.forEach(function(dynamicEntity) {
                print('dynamicEntity is:: '+JSON.stringify(dynamicEntity))
                dynamicEntity.cleanup();
            })
        },

    }
    return new Reset();
});