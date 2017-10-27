//
// Created by Alan-Michael Moody on 4/17/2017
//

"use strict";

(function () {
    var pos = Vec3.sum(MyAvatar.position, Quat.getFront(MyAvatar.orientation));

    var graph = {
        background: {
            type: "Model",
            modelURL: "https://binaryrelay.com/files/public-docs/hifi/meter/wood/meter-wood.fbx",
            color: {
                red: 128,
                green: 128,
                blue: 128
            },
            lifetime: "3600",
            script: "https://binaryrelay.com/files/public-docs/hifi/meter/wood/meter.js",
            position: pos
        },
        bar: {
            type: "Box",
            parentID: "",
            userData: '{"name":"bar"}',
            dimensions: {x: .05, y: .245, z: .07},
            color: {
                red: 0,
                green: 0,
                blue: 0
            },
            lifetime: "3600",
            position: Vec3.sum(pos, {x: -0.88, y: 0, z: -0.15})
        }
    };

    graph.bar.parentID = Entities.addEntity(graph.background);
    Entities.addEntity(graph.bar);


})();
