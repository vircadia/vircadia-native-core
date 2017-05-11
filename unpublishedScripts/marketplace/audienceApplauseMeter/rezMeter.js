//
// Created by Alan-Michael Moody on 4/17/2017
//

"use strict";

(function () { // BEGIN LOCAL_SCOPE
    var pos = Vec3.sum(MyAvatar.position, Quat.getFront(MyAvatar.orientation));

    var graph = {
        background: {
            type: "Box",
            dimensions: {x: 1, y: 1, z: .1},
            color: {
                red: 128,
                green: 128,
                blue: 128
            },
            lifetime: "3600",
            script: "https://binaryrelay.com/files/public-docs/hifi/meter/basic/meter.js",
            position: pos
        },
        bar: {
            type: "Box",
            parentID: "",
            userData: '{"name":"bar"}',
            dimensions: {x: .05, y: .25, z: .1},
            color: {
                red: 0,
                green: 0,
                blue: 0
            },
            lifetime: "3600",
            position: Vec3.sum(pos, {x: -0.495, y: 0, z: 0.1})
        },
        displayText: {
            type: "Text",
            parentID: "",
            userData: '{"name":"displayText"}',
            text: "Loudness: % ",
            textColor: {
                red: 0,
                green: 0,
                blue: 0
            },
            backgroundColor: {
                red: 128,
                green: 128,
                blue: 128
            },
            visible: 0.5,
            dimensions: {x: 0.70, y: 0.15, z: 0.1},
            lifetime: "3600",
            position: Vec3.sum(pos, {x: 0, y: 0.4, z: 0.06})
        }
    };

    var background = Entities.addEntity(graph.background);

    graph.bar.parentID = background;
    graph.displayText.parentID = background;

    var bar = Entities.addEntity(graph.bar);
    var displayText = Entities.addEntity(graph.displayText);


})(); // END LOCAL_SCOPE
