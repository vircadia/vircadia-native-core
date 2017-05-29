//
//  dynamicsTests.js
//  scripts/developer/tests/dynamics/
//
//  Created by Seth Alves 2017-4-30
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

"use strict";

/* global Entities, Script, Tablet, MyAvatar, Vec3 */

(function() { // BEGIN LOCAL_SCOPE

    var DYNAMICS_TESTS_URL = Script.resolvePath("dynamics-tests.html");
    var DEFAULT_LIFETIME = 120; // seconds

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

    var button = tablet.addButton({
        icon: Script.resolvePath("dynamicsTests.svg"),
        text: "Dynamics",
        sortOrder: 15
    });



    function coneTwistAndTractorLeverTest(params) {
        var pos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: -0.5, z: -2}));
        var lifetime = params.lifetime;

        var baseID = Entities.addEntity({
            name: "cone-twist test -- base",
            type: "Box",
            color: { blue: 128, green: 100, red: 20 },
            dimensions: { x: 0.5, y: 0.2, z: 0.5 },
            position: Vec3.sum(pos, { x: 0, y: 0, z:0 }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: 0, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        var leverID = Entities.addEntity({
            name: "cone-twist test -- lever",
            type: "Box",
            color: { blue: 128, green: 100, red: 200 },
            dimensions: { x: 0.05, y: 1, z: 0.05 },
            position: Vec3.sum(pos, { x: 0, y: 0.7, z:0 }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: 0, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addEntity({
            name: "cone-twist test -- handle",
            type: "Box",
            color: { blue: 30, green: 100, red: 200 },
            dimensions: { x: 0.1, y: 0.08, z: 0.08 },
            position: Vec3.sum(pos, { x: 0, y: 0.7 + 0.5, z:0 }),
            dynamic: false,
            collisionless: true,
            gravity: { x: 0, y: 0, z: 0 },
            lifetime: lifetime,
            parentID: leverID,
            userData: "{ \"grabbableKey\": { \"grabbable\": false } }"
        });

        Entities.addAction("cone-twist", baseID, {
            pivot: { x: 0, y: 0.2, z: 0 },
            axis: { x: 0, y: 1, z: 0 },
            otherEntityID: leverID,
            otherPivot: { x: 0, y: -0.55, z: 0 },
            otherAxis: { x: 0, y: 1, z: 0 },
            swingSpan1: Math.PI / 4,
            swingSpan2: Math.PI / 4,
            twistSpan: Math.PI / 2,
            tag: "cone-twist test"
        });

        Entities.addAction("tractor", leverID, {
            targetRotation: { x: 0, y: 0, z: 0, w: 1 },
            angularTimeScale: 0.2,
            tag: "cone-twist test tractor"
        });


        Entities.editEntity(baseID, { gravity: { x: 0, y: -5, z: 0 } });
    }

    function doorVSWorldTest(params) {
        var pos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.1, z: -2}));
        var lifetime = params.lifetime;

        var doorID = Entities.addEntity({
            name: "door test",
            type: "Box",
            color: { blue: 128, green: 20, red: 20 },
            dimensions: { x: 1.0, y: 2, z: 0.1 },
            position: pos,
            dynamic: true,
            collisionless: false,
            lifetime: lifetime,
            gravity: { x: 0, y: 0, z: 0 },
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("hinge", doorID, {
            pivot: { x: -0.5, y: 0, z: 0 },
            axis: { x: 0, y: 1, z: 0 },
            low: 0,
            high: Math.PI,
            tag: "door hinge test"
        });
    }

    function hingeChainTest(params) {
        var pos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.1, z: -2}));
        var lifetime = params.lifetime;

        var offset = 0.28;
        var prevEntityID = null;
        for (var i = 0; i < 5; i++) {
            var newID = Entities.addEntity({
                name: "hinge test " + i,
                type: "Box",
                color: { blue: 128, green: 40 * i, red: 20 },
                dimensions: { x: 0.2, y: 0.2, z: 0.1 },
                position: Vec3.sum(pos, {x: 0, y: offset * i, z:0}),
                dynamic: true,
                collisionless: false,
                lifetime: lifetime,
                gravity: { x: 0, y: 0, z: 0 },
                userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
            });
            if (prevEntityID) {
                Entities.addAction("hinge", prevEntityID, {
                    pivot: { x: 0, y: offset / 2, z: 0 },
                    axis: { x: 1, y: 0, z: 0 },
                    otherEntityID: newID,
                    otherPivot: { x: 0, y: -offset / 2, z: 0 },
                    otherAxis: { x: 1, y: 0, z: 0 },
                    tag: "A/B hinge test " + i
                });
            }
            prevEntityID = newID;
        }
    }

    function sliderVSWorldTest(params) {
        var pos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.1, z: -2}));
        var lifetime = params.lifetime;

        var sliderEntityID = Entities.addEntity({
            name: "slider test",
            type: "Box",
            color: { blue: 128, green: 20, red: 20 },
            dimensions: { x: 0.2, y: 0.2, z: 0.2 },
            position: pos,
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: 0, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("slider", sliderEntityID, {
            point: { x: -0.5, y: 0, z: 0 },
            axis: { x: 0, y: 1, z: 0 },
            linearLow: 0,
            linearHigh: 0.6,
            tag: "slider test"
        });
    }

    function sliderChainTest(params) {
        var pos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.1, z: -2}));
        var lifetime = params.lifetime;

        var offset = 0.28;
        var prevEntityID = null;
        for (var i = 0; i < 7; i++) {
            var newID = Entities.addEntity({
                name: "hinge test " + i,
                type: "Box",
                color: { blue: 128, green: 40 * i, red: 20 },
                dimensions: { x: 0.2, y: 0.1, z: 0.2 },
                position: Vec3.sum(pos, {x: 0, y: offset * i, z:0}),
                dynamic: true,
                collisionless: false,
                gravity: { x: 0, y: 0, z: 0 },
                lifetime: lifetime,
                userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
            });
            if (prevEntityID) {
                Entities.addAction("slider", prevEntityID, {
                    point: { x: 0, y: 0, z: 0 },
                    axis: { x: 0, y: 1, z: 0 },
                    otherEntityID: newID,
                    otherPoint: { x: 0, y: -offset / 2, z: 0 },
                    otherAxis: { x: 0, y: 1, z: 0 },
                    linearLow: 0,
                    linearHigh: 0.6,
                    tag: "A/B slider test " + i
                });
            }
            prevEntityID = newID;
        }
    }

    function ballSocketBetweenTest(params) {
        var pos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.1, z: -2}));
        var lifetime = params.lifetime;

        var offset = 0.2;
        var diameter = offset - 0.01;
        var prevEntityID = null;
        for (var i = 0; i < 7; i++) {
            var newID = Entities.addEntity({
                name: "ball and socket test " + i,
                type: "Sphere",
                color: { blue: 128, green: 40 * i, red: 20 },
                dimensions: { x: diameter, y: diameter, z: diameter },
                position: Vec3.sum(pos, {x: 0, y: offset * i, z:0}),
                dynamic: true,
                collisionless: false,
                lifetime: lifetime,
                gravity: { x: 0, y: 0, z: 0 },
                userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
            });
            if (prevEntityID) {
                Entities.addAction("ball-socket", prevEntityID, {
                    pivot: { x: 0, y: offset / 2, z: 0 },
                    otherEntityID: newID,
                    otherPivot: { x: 0, y: -offset / 2, z: 0 },
                    tag: "A/B ball-and-socket test " + i
                });
            }
            prevEntityID = newID;
        }
    }

    function ballSocketCoincidentTest(params) {
        var pos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.1, z: -2}));
        var lifetime = params.lifetime;

        var offset = 0.2;
        var diameter = offset - 0.01;
        var prevEntityID = null;
        for (var i = 0; i < 7; i++) {
            var newID = Entities.addEntity({
                name: "ball and socket test " + i,
                type: "Sphere",
                color: { blue: 128, green: 40 * i, red: 20 },
                dimensions: { x: diameter, y: diameter, z: diameter },
                position: Vec3.sum(pos, {x: 0, y: offset * i, z:0}),
                dynamic: true,
                collisionless: false,
                lifetime: lifetime,
                gravity: { x: 0, y: 0, z: 0 },
                userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
            });
            if (prevEntityID) {
                Entities.addAction("ball-socket", prevEntityID, {
                    pivot: { x: 0, y: 0, z: 0 },
                    otherEntityID: newID,
                    otherPivot: { x: 0, y: offset, z: 0 },
                    tag: "A/B ball-and-socket test " + i
                });
            }
            prevEntityID = newID;
        }
    }

    function ragdollTest(params) {
        var scale = 1.6;
        var lifetime = params.lifetime;
        var pos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 1.0, z: -2}));

        var neckLength = scale * 0.05;
        var shoulderGap = scale * 0.1;
        var elbowGap = scale * 0.06;
        var hipGap = scale * 0.07;
        var kneeGap = scale * 0.08;
        var ankleGap = scale * 0.06;
        var ankleMin = 0;
        var ankleMax = Math.PI / 4;

        var headSize = scale * 0.2;

        var bodyHeight = scale * 0.4;
        var bodyWidth = scale * 0.3;
        var bodyDepth = scale * 0.2;

        var upperArmThickness = scale * 0.05;
        var upperArmLength = scale * 0.2;

        var lowerArmThickness = scale * 0.05;
        var lowerArmLength = scale * 0.2;

        var legLength = scale * 0.3;
        var legThickness = scale * 0.08;

        var shinLength = scale * 0.2;
        var shinThickness = scale * 0.06;

        var footLength = scale * 0.2;
        var footThickness = scale * 0.03;
        var footWidth = scale * 0.08;


        //
        // body
        //

        var bodyID = Entities.addEntity({
            name: "ragdoll body",
            type: "Box",
            color: { blue: 128, green: 100, red: 20 },
            dimensions: { x: bodyDepth, y: bodyHeight, z: bodyWidth },
            position: Vec3.sum(pos, { x: 0, y: scale * 0.0, z:0 }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: 0, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        //
        // head
        //

        var headID = Entities.addEntity({
            name: "ragdoll head",
            type: "Box",
            color: { blue: 128, green: 100, red: 20 },
            dimensions: { x: headSize, y: headSize, z: headSize },
            position: Vec3.sum(pos, { x: 0, y: bodyHeight / 2 + headSize / 2 + neckLength, z:0 }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: 0.5, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("tractor", headID, {
            targetRotation: { x: 0, y: 0, z: 0, w: 1 },
            angularTimeScale: 2.0,
            otherID: bodyID,
            tag: "cone-twist test tractor"
        });


        var noseID = Entities.addEntity({
            name: "ragdoll nose",
            type: "Box",
            color: { blue: 128, green: 100, red: 100 },
            dimensions: { x: headSize / 5, y: headSize / 5, z: headSize / 5 },
            localPosition: { x: headSize / 2 + headSize / 10, y: 0, z: 0 },
            dynamic: false,
            collisionless: true,
            lifetime: lifetime,
            parentID: headID,
            userData: "{ \"grabbableKey\": { \"grabbable\": false } }"
        });

        Entities.addAction("cone-twist", headID, {
            pivot: { x: 0, y: -headSize / 2 - neckLength / 2, z: 0 },
            axis: { x: 0, y: 1, z: 0 },
            otherEntityID: bodyID,
            otherPivot: { x: 0, y: bodyHeight / 2 + neckLength / 2, z: 0 },
            otherAxis: { x: 0, y: 1, z: 0 },
            swingSpan1: Math.PI / 4,
            swingSpan2: Math.PI / 4,
            twistSpan: Math.PI / 2,
            tag: "ragdoll neck joint"
        });

        //
        // right upper arm
        //

        var rightUpperArmID = Entities.addEntity({
            name: "ragdoll right arm",
            type: "Box",
            color: { blue: 128, green: 100, red: 20 },
            dimensions: { x: upperArmThickness, y: upperArmThickness, z: upperArmLength },
            position: Vec3.sum(pos, { x: 0,
                                      y: bodyHeight / 2 + upperArmThickness / 2,
                                      z: bodyWidth / 2 + shoulderGap + upperArmLength / 2
                                    }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: 0, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("cone-twist", bodyID, {
            pivot: { x: 0, y: bodyHeight / 2 + upperArmThickness / 2, z: bodyWidth / 2 + shoulderGap / 2 },
            axis: { x: 0, y: 0, z: 1 },
            otherEntityID: rightUpperArmID,
            otherPivot: { x: 0, y: 0, z: -upperArmLength / 2 - shoulderGap / 2 },
            otherAxis: { x: 0, y: 0, z: 1 },
            swingSpan1: Math.PI / 2,
            swingSpan2: Math.PI / 2,
            twistSpan: 0,
            tag: "ragdoll right shoulder joint"
        });

        //
        // left upper arm
        //

        var leftUpperArmID = Entities.addEntity({
            name: "ragdoll left arm",
            type: "Box",
            color: { blue: 128, green: 100, red: 20 },
            dimensions: { x: upperArmThickness, y: upperArmThickness, z: upperArmLength },
            position: Vec3.sum(pos, { x: 0,
                                      y: bodyHeight / 2 + upperArmThickness / 2,
                                      z: -bodyWidth / 2 - shoulderGap - upperArmLength / 2
                                    }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: 0, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("cone-twist", bodyID, {
            pivot: { x: 0, y: bodyHeight / 2 + upperArmThickness / 2, z: -bodyWidth / 2 - shoulderGap / 2 },
            axis: { x: 0, y: 0, z: -1 },
            otherEntityID: leftUpperArmID,
            otherPivot: { x: 0, y: 0, z: upperArmLength / 2 + shoulderGap / 2 },
            otherAxis: { x: 0, y: 0, z: -1 },
            swingSpan1: Math.PI / 2,
            swingSpan2: Math.PI / 2,
            twistSpan: 0,
            tag: "ragdoll left shoulder joint"
        });

        //
        // right lower arm
        //

        var rightLowerArmID = Entities.addEntity({
            name: "ragdoll right lower arm",
            type: "Box",
            color: { blue: 128, green: 100, red: 20 },
            dimensions: { x: lowerArmThickness, y: lowerArmThickness, z: lowerArmLength },
            position: Vec3.sum(pos, { x: 0,
                                      y: bodyHeight / 2 - upperArmThickness / 2,
                                      z: bodyWidth / 2 + shoulderGap + upperArmLength + elbowGap + lowerArmLength / 2
                                    }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: -1, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("hinge", rightLowerArmID, {
            pivot: { x: 0, y: 0, z: -lowerArmLength / 2 - elbowGap / 2 },
            axis: { x: 0, y: 1, z: 0 },
            otherEntityID: rightUpperArmID,
            otherPivot: { x: 0, y: 0, z: upperArmLength / 2 + elbowGap / 2 },
            otherAxis: { x: 0, y: 1, z: 0 },
            low: Math.PI / -2,
            high: 0,
            tag: "ragdoll right elbow joint"
        });

        //
        // left lower arm
        //

        var leftLowerArmID = Entities.addEntity({
            name: "ragdoll left lower arm",
            type: "Box",
            color: { blue: 128, green: 100, red: 20 },
            dimensions: { x: lowerArmThickness, y: lowerArmThickness, z: lowerArmLength },
            position: Vec3.sum(pos, { x: 0,
                                      y: bodyHeight / 2 - upperArmThickness / 2,
                                      z: -bodyWidth / 2 - shoulderGap - upperArmLength - elbowGap - lowerArmLength / 2
                                    }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: -1, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("hinge", leftLowerArmID, {
            pivot: { x: 0, y: 0, z: lowerArmLength / 2 + elbowGap / 2 },
            axis: { x: 0, y: 1, z: 0 },
            otherEntityID: leftUpperArmID,
            otherPivot: { x: 0, y: 0, z: -upperArmLength / 2 - elbowGap / 2 },
            otherAxis: { x: 0, y: 1, z: 0 },
            low: 0,
            high: Math.PI / 2,
            tag: "ragdoll left elbow joint"
        });

        //
        // right leg
        //

        var rightLegID = Entities.addEntity({
            name: "ragdoll right arm",
            type: "Box",
            color: { blue: 20, green: 200, red: 20 },
            dimensions: { x: legThickness, y: legLength, z: legThickness },
            position: Vec3.sum(pos, { x: 0, y: -bodyHeight / 2 - hipGap - legLength / 2, z: bodyWidth / 2 - legThickness / 2 }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: 0, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("cone-twist", rightLegID, {
            pivot: { x: 0, y: legLength / 2 + hipGap / 2, z: 0 },
            axis: { x: 0, y: 1, z: 0 },
            otherEntityID: bodyID,
            otherPivot: { x: 0, y: -bodyHeight / 2 - hipGap / 2, z: bodyWidth / 2 - legThickness / 2 },
            otherAxis: Vec3.normalize({ x: -1, y: 1, z: 0 }),
            swingSpan1: Math.PI / 4,
            swingSpan2: Math.PI / 4,
            twistSpan: 0,
            tag: "ragdoll right hip joint"
        });

        //
        // left leg
        //

        var leftLegID = Entities.addEntity({
            name: "ragdoll left arm",
            type: "Box",
            color: { blue: 20, green: 200, red: 20 },
            dimensions: { x: legThickness, y: legLength, z: legThickness },
            position: Vec3.sum(pos, { x: 0, y: -bodyHeight / 2 - hipGap - legLength / 2, z: -bodyWidth / 2 + legThickness / 2 }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: 0, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("cone-twist", leftLegID, {
            pivot: { x: 0, y: legLength / 2 + hipGap / 2, z: 0 },
            axis: { x: 0, y: 1, z: 0 },
            otherEntityID: bodyID,
            otherPivot: { x: 0, y: -bodyHeight / 2 - hipGap / 2, z: -bodyWidth / 2 + legThickness / 2 },
            otherAxis: Vec3.normalize({ x: -1, y: 1, z: 0 }),
            swingSpan1: Math.PI / 4,
            swingSpan2: Math.PI / 4,
            twistSpan: 0,
            tag: "ragdoll left hip joint"
        });

        //
        // right shin
        //

        var rightShinID = Entities.addEntity({
            name: "ragdoll right shin",
            type: "Box",
            color: { blue: 20, green: 200, red: 20 },
            dimensions: { x: shinThickness, y: shinLength, z: shinThickness },
            position: Vec3.sum(pos, { x: 0,
                                      y: -bodyHeight / 2 - hipGap - legLength - kneeGap - shinLength / 2,
                                      z: bodyWidth / 2 - legThickness / 2
                                    }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: -2, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("hinge", rightShinID, {
            pivot: { x: 0, y: shinLength / 2 + kneeGap / 2, z: 0 },
            axis: { x: 0, y: 0, z: 1 },
            otherEntityID: rightLegID,
            otherPivot: { x: 0, y: -legLength / 2 - kneeGap / 2, z: 0 },
            otherAxis: { x: 0, y: 0, z: 1 },
            low: 0,
            high: Math.PI / 2,
            tag: "ragdoll right knee joint"
        });


        //
        // left shin
        //

        var leftShinID = Entities.addEntity({
            name: "ragdoll left shin",
            type: "Box",
            color: { blue: 20, green: 200, red: 20 },
            dimensions: { x: shinThickness, y: shinLength, z: shinThickness },
            position: Vec3.sum(pos, { x: 0,
                                      y: -bodyHeight / 2 - hipGap - legLength - kneeGap - shinLength / 2,
                                      z: -bodyWidth / 2 + legThickness / 2
                                    }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: -2, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("hinge", leftShinID, {
            pivot: { x: 0, y: shinLength / 2 + kneeGap / 2, z: 0 },
            axis: { x: 0, y: 0, z: 1 },
            otherEntityID: leftLegID,
            otherPivot: { x: 0, y: -legLength / 2 - kneeGap / 2, z: 0 },
            otherAxis: { x: 0, y: 0, z: 1 },
            low: 0,
            high: Math.PI / 2,
            tag: "ragdoll left knee joint"
        });

        //
        // right foot
        //

        var rightFootID = Entities.addEntity({
            name: "ragdoll right foot",
            type: "Box",
            color: { blue: 128, green: 100, red: 20 },
            dimensions: { x: footLength, y: footThickness, z: footWidth },
            position: Vec3.sum(pos, { x: -shinThickness / 2 + footLength / 2,
                                      y: -bodyHeight / 2 - hipGap - legLength - kneeGap - shinLength - ankleGap - footThickness / 2,
                                      z: bodyWidth / 2 - legThickness / 2
                                    }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: -5, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("hinge", rightFootID, {
            pivot: { x: -footLength / 2 + shinThickness / 2, y: ankleGap / 2, z: 0 },
            axis: { x: 0, y: 0, z: 1 },
            otherEntityID: rightShinID,
            otherPivot: { x: 0, y: -shinLength / 2 - ankleGap / 2, z: 0 },
            otherAxis: { x: 0, y: 0, z: 1 },
            low: ankleMin,
            high: ankleMax,
            tag: "ragdoll right ankle joint"
        });

        //
        // left foot
        //

        var leftFootID = Entities.addEntity({
            name: "ragdoll left foot",
            type: "Box",
            color: { blue: 128, green: 100, red: 20 },
            dimensions: { x: footLength, y: footThickness, z: footWidth },
            position: Vec3.sum(pos, { x: -shinThickness / 2 + footLength / 2,
                                      y: -bodyHeight / 2 - hipGap - legLength - kneeGap - shinLength - ankleGap - footThickness / 2,
                                      z: bodyWidth / 2 - legThickness / 2
                                    }),
            dynamic: true,
            collisionless: false,
            gravity: { x: 0, y: -5, z: 0 },
            lifetime: lifetime,
            userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }"
        });

        Entities.addAction("hinge", leftFootID, {
            pivot: { x: -footLength / 2 + shinThickness / 2, y: ankleGap / 2, z: 0 },
            axis: { x: 0, y: 0, z: 1 },
            otherEntityID: leftShinID,
            otherPivot: { x: 0, y: -shinLength / 2 - ankleGap / 2, z: 0 },
            otherAxis: { x: 0, y: 0, z: 1 },
            low: ankleMin,
            high: ankleMax,
            tag: "ragdoll left ankle joint"
        });

    }

    function onWebEventReceived(eventString) {
        print("received web event: " + JSON.stringify(eventString));
        if (typeof eventString === "string") {
            var event;
            try {
                event = JSON.parse(eventString);
            } catch(e) {
                return;
            }

            if (event["dynamics-tests-command"]) {
                var commandToFunctionMap = {
                    "cone-twist-and-tractor-lever-test": coneTwistAndTractorLeverTest,
                    "door-vs-world-test": doorVSWorldTest,
                    "hinge-chain-test": hingeChainTest,
                    "slider-vs-world-test": sliderVSWorldTest,
                    "slider-chain-test": sliderChainTest,
                    "ball-socket-between-test": ballSocketBetweenTest,
                    "ball-socket-coincident-test": ballSocketCoincidentTest,
                    "ragdoll-test": ragdollTest
                };

                var cmd = event["dynamics-tests-command"];
                if (commandToFunctionMap.hasOwnProperty(cmd)) {
                    var func = commandToFunctionMap[cmd];
                    func(event);
                }
            }
        }
    }


    var onDynamicsTestsScreen = false;
    var shouldActivateButton = false;

    function onClicked() {
        if (onDynamicsTestsScreen) {
            tablet.gotoHomeScreen();
        } else {
            shouldActivateButton = true;
            tablet.gotoWebScreen(DYNAMICS_TESTS_URL +
                                 "?lifetime=" + DEFAULT_LIFETIME.toString()
                                );
            onDynamicsTestsScreen = true;
        }
    }

    function onScreenChanged() {
        // for toolbar mode: change button to active when window is first openend, false otherwise.
        button.editProperties({isActive: shouldActivateButton});
        shouldActivateButton = false;
        onDynamicsTestsScreen = shouldActivateButton;
    }

    function cleanup() {
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
    }

    button.clicked.connect(onClicked);
    tablet.webEventReceived.connect(onWebEventReceived);
    tablet.screenChanged.connect(onScreenChanged);
    Script.scriptEnding.connect(cleanup);
}()); // END LOCAL_SCOPE
