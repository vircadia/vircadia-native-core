//
// Created by Alan-Michael Moody on 5/2/2017
//

'use strict';

(function () {
    var pos = Vec3.sum(MyAvatar.position, Quat.getFront(MyAvatar.orientation));

    var meter = {
        stand: {
            type: 'Model',
            modelURL: 'https://binaryrelay.com/files/public-docs/hifi/meter/applauseOmeterTextured.fbx',
            lifetime: '3600',
            script: 'https://binaryrelay.com/files/public-docs/hifi/meter/applauseOmeter.js',
            position: Vec3.sum(pos, {x: 0, y: 2.0, z: 0})
        },
        pivot:{
            type:'Sphere',
            parentID: '',
            userData: '{"name":"pivot"}',
            dimensions: {x: .05, y: .05, z: .05},
            lifetime: '3600',
            position: Vec3.sum(pos, {x: 0, y: 3.385, z: 0.2})
        },
        needle: {
            type: 'Model',
            modelURL:'https://binaryrelay.com/files/public-docs/hifi/meter/applauseOmeterNeedleTextured.fbx',
            parentID: '',
            userData: '{"name":"needle"}',
            dimensions: {x: .286, y: 1.12, z: .07},
            lifetime: '3600',
            position: Vec3.sum(pos, {x: 0, y: 3.85, z: 0.2})
        }
    };


    meter.pivot.parentID = Entities.addEntity(meter.stand);
    meter.needle.parentID = Entities.addEntity(meter.pivot);
    Entities.addEntity(meter.needle);


})();
