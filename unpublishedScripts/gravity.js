//
//  gravity.js
//
//  Created by Alan-Michael Moody on 7/24/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    var _entityID;

    this.preload = function (entityID) {
        _entityID = entityID;
    };

    function update(deltatime) {
        var planet = Entities.getEntityProperties(_entityID);

        //normalization happens in rotationBetween.
        var direction = Vec3.subtract(MyAvatar.position, planet.position);
        var localUp = Quat.getUp(MyAvatar.orientation);

        MyAvatar.orientation = Quat.normalize(Quat.multiply(Quat.rotationBetween(localUp, direction), MyAvatar.orientation));
    }

    function init() {
        Script.update.connect(update);
    }

    function clean() {
        Script.update.disconnect(update);
        MyAvatar.orientation = Quat.fromVec3Degrees({
            x: 0,
            y: 0,
            z: 0
        });
    }

    var _switch = true;

    this.clickDownOnEntity = function(uuid, mouseEvent) {

        if (_switch) {
            init();
        } else {
            clean();
        }
        _switch = !_switch;
    };

    Script.scriptEnding.connect(clean);

});