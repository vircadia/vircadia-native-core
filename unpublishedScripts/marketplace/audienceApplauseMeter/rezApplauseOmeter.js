//
// Created by Alan-Michael Moody on 5/2/2017
//

'use strict';

(function () {
    var pos = Vec3.sum(MyAvatar.position, Quat.getFront(MyAvatar.orientation));

    var meter = {
        stand: {
            type: 'Model',
            modelURL: 'https://binaryrelay.com/files/public-docs/hifi/meter/applauseOmeter.fbx',
            lifetime: '3600',
            script: 'https://binaryrelay.com/files/public-docs/hifi/meter/applauseOmeter.js',
            position: Vec3.sum(pos, {x: 0, y: 2.0, z: 0})
        }

    };


    Entities.addEntity(meter.stand);

})();
