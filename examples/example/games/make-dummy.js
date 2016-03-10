//
//  make-dummy.js
//  examples
//
//  Created by Seth Alves on 2015-6-10
//  Copyright 2015 High Fidelity, Inc.
//
//  Makes a boxing-dummy that responds to collisions.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//
"use strict";
/*jslint vars: true*/
var Overlays, Entities, Controller, Script, MyAvatar, Vec3; // Referenced globals provided by High Fidelity.

var HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var rezButton = Overlays.addOverlay("image", {
    x: 100,
    y: 350,
    width: 32,
    height: 32,
    imageURL: HIFI_PUBLIC_BUCKET + "images/close.png",
    color: {
        red: 255,
        green: 255,
        blue: 255
    },
    alpha: 1
});


function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({
        x: event.x,
        y: event.y
    });

    if (clickedOverlay === rezButton) {
        var boxId;

        var position = Vec3.sum(MyAvatar.position, {x: 1.0, y: 0.4, z: 0.0});
        boxId = Entities.addEntity({
            type: "Box",
            name: "dummy",
            position: position,
            dimensions: {x: 0.3, y: 0.7, z: 0.3},
            gravity: {x: 0.0, y: -3.0, z: 0.0},
            damping: 0.2,
            dynamic: true
        });

        var pointToOffsetFrom = Vec3.sum(position, {x: 0.0, y: 2.0, z: 0.0});
        Entities.addAction("offset", boxId, {pointToOffsetFrom: pointToOffsetFrom,
                                             linearDistance: 2.0,
                                             // linearTimeScale: 0.005
                                             linearTimeScale: 0.1
                                            });
    }
}


function scriptEnding() {
    Overlays.deleteOverlay(rezButton);
}

Controller.mousePressEvent.connect(mousePressEvent);
Script.scriptEnding.connect(scriptEnding);
