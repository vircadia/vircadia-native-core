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

    Reset.prototype = {
        preload: function() {

        },
        unload: function() {

        },
        tidying: false,
        showTidyingButton: function() {

        },
        showTidyButton: function() {

        },
        toggleButton: function() {

        },

        clickReleaseOnEntity: function(entityID, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            _this.prepareScene();
            print('CLICK ON TIDY GUY!!!')

        },
        prepareScene: function() {
            _this.cleanupDynamicEntities();
            _this.createDynamicEntities();
        },
        startNearTrigger: function() {
            _this.prepareScene();
        },
        createDynamicEntities: function() {
            var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
                x: 0,
                y: 0.5,
                z: 0
            }), Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));

            Script.include(utilsPath);
            Script.include(fishTankPath);
            Script.include(tiltMazePath);
            Script.include(whiteboardPath);
            Script.include(plantPath);

            // var fishTank = new FishTank(center);
            // var tiltMaze = new TiltMaze(center);
            // var whiteboard = new Whiteboard(center);
            var myPlant = new Plant(center);


            // dynamicEntities.push(fishTank);
            // dynamicEntities.push(tiltMaze);
            // dynamicEntities.push(whiteboard);
            dynamicEntities.push(myPlant);

            //v2.0
            // var musicBox = new MusicBox();
            // var cuckooClock = new CuckooClock();
            // var doppelganger = new Doppelganger();
        },
        cleanupDynamicEntities: function() {
            if (dynamicEntities.length === 0) {
                return;
            }
            print('DYNAMIC ENTITIES:: ' + JSON.stringify(dynamicEntities))
            dynamicEntities.forEach(function(dynamicEntity) {
                dynamicEntity.cleanup();
            })
        },

    }
    return new Reset();
});