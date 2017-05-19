//
// Created by Alan-Michael Moody on 4/17/2017
//

"use strict";

(function () {
    var pos = Vec3.sum(MyAvatar.position, Quat.getFront(MyAvatar.orientation));

    var graph = {
        background: {
            type: "Model",
            modelURL: "https://binaryrelay.com/files/public-docs/hifi/meter/text-entity/meter-text-entity.fbx",
            color: {
                red: 128,
                green: 128,
                blue: 128
            },
            lifetime: "3600",
            script: "https://binaryrelay.com/files/public-docs/hifi/meter/text-entity/meter.js",
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
        },
        displayText: {
            type: "Text",
            parentID: "",
            userData: '{"name":"displayText"}',
            text: "Make Some Noise:",
            textColor: {
                red: 0,
                green: 0,
                blue: 0
            },
            backgroundColor: {
                red: 255,
                green: 255,
                blue: 255
            },
            dimensions: {x: .82, y: 0.115, z: 0.15},
            lifetime: "3600",
            lineHeight: .08,
            position: Vec3.sum(pos, {x: -0.2, y: 0.175, z: -0.035})
        }
    };

    var background = Entities.addEntity(graph.background);

    graph.bar.parentID = background;
    graph.displayText.parentID = background;

    var bar = Entities.addEntity(graph.bar);
    var displayText = Entities.addEntity(graph.displayText);


})();
