//
//  testEars.js
//
//  Positions and orients your listening position at a virtual head created in front of you.
//
//  Created by David Rowe on 7 Oct 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html.
//

(function () {

    "use strict";

    var overlays = [
            {
                // Head
                type: "sphere",
                properties: {
                    dimensions: { x: 0.2, y: 0.3, z: 0.25 },
                    position: { x: 0, y: 0, z: -2 },
                    rotation: Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),
                    color: { red: 128, green: 128, blue: 128 },
                    alpha: 1.0,
                    solid: true
                }
            },
            {
                // Left ear
                type: "sphere",
                properties: {
                    dimensions: { x: 0.04, y: 0.10, z: 0.08 },
                    localPosition: { x: -0.1, y: 0, z: 0.05 },
                    localRotation: Quat.fromVec3Degrees({ x: 0, y: 0, z: 0 }),
                    color: { red: 255, green: 0, blue: 0 }, // Red for "port".
                    alpha: 1.0,
                    solid: true
                }
            },
            {
                // Right ear
                type: "sphere",
                properties: {
                    dimensions: { x: 0.04, y: 0.10, z: 0.08 },
                    localPosition: { x: 0.1, y: 0, z: 0.05 },
                    localRotation: Quat.fromVec3Degrees({ x: 0, y: 0, z: 0 }),
                    color: { red: 0, green: 255, blue: 0 }, // Green for "starboard".
                    alpha: 1.0,
                    solid: true
                }
            },
            {
                // Nose
                type: "sphere",
                properties: {
                    dimensions: { x: 0.04, y: 0.04, z: 0.04 },
                    localPosition: { x: 0, y: 0, z: -0.125 },
                    localRotation: Quat.fromVec3Degrees({ x: 0, y: -0.08, z: 0 }),
                    color: { red: 160, green: 160, blue: 160 },
                    alpha: 1.0,
                    solid: true
                }
            }
        ],
        originalListenerMode;

    function setUp() {
        var i, length;

        originalListenerMode = MyAvatar.audioListenerMode;

        for (i = 0, length = overlays.length; i < length; i++) {
            if (i === 0) {
                overlays[i].properties.position = Vec3.sum(MyAvatar.getHeadPosition(),
                    Vec3.multiplyQbyV(MyAvatar.orientation, overlays[i].properties.position));
                overlays[i].properties.rotation = Quat.multiply(MyAvatar.orientation, overlays[i].properties.rotation);
            } else {
                overlays[i].properties.parentID = overlays[0].id;
            }
            overlays[i].id = Overlays.addOverlay(overlays[i].type, overlays[i].properties);
        }

        MyAvatar.audioListenerMode = MyAvatar.audioListenerModeCustom;
        MyAvatar.customListenPosition = overlays[0].properties.position;
        MyAvatar.customListenOrientation = overlays[0].properties.orientation;
    }

    function tearDown() {
        var i, length;
        for (i = 0, length = overlays.length; i < length; i++) {
            Overlays.deleteOverlay(overlays[i].id);
        }

        MyAvatar.audioListenerMode = originalListenerMode;
    }

    Script.setTimeout(setUp, 2000); // Delay so that overlays display if script runs at Interface start.
    Script.scriptEnding.connect(tearDown);
}());
