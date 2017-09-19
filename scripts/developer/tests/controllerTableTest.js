"use strict";

/* jslint bitwise: true */
/* global Script, Entities, MyAvatar, Vec3, Quat, Mat4 */

(function() { // BEGIN LOCAL_SCOPE

    // var lifetime = -1;
    var lifetime = 600;
    var tableSections = 32;
    var tableRadius = 9;

    var sectionRelativeRotation = 0;
    var sectionRotation = 0;
    var sectionRelativeCenterA = 0;
    var sectionRelativeCenterB = 0;
    var sectionRelativeCenterSign = 0;
    var sectionCenterA = 0;
    var sectionCenterB = 0;
    var sectionCenterSign = 0;
    var yFlip = 0;
    
    var objects = [];
    var overlays = [];

    var testNames = [
        "FarActionGrab",
        "NearParentGrabEntity",
        "NearParentGrabOverlay",
        "Clone Entity (dynamic)",
        "Clone Entity (non-dynamic"
    ];

    function createCloneDynamicEntity(index) {
        createPropsCube(index, false, false, true, true);
        createPropsModel(index, false, false, true, true);
        createSign(index, "Clone Dynamic Entity");
    };

    function createCloneEntity(index) {
        createPropsCube(index, false, false, true, false);
        createPropsModel(index, false, false, true, false);
        createSign(index, "Clone Non-Dynamic Entity");
    };

    function createNearGrabOverlay(index) {
        createPropsCubeOverlay(index, false, false, true, true);
        createPropsModelOverlay(index, false, false, true, true);
        createSign(index, "Near Grab Overlay");
    };

    function createNearGrabEntity(index) {
        createPropsCube(index, false, false, false, false);
        createPropsModel(index, false, false, false, false);
        createSign(index, "Near Grab Entity");
    };

    function createFarGrabEntity(index) {
        createPropsCube(index, true, false, false, false);
        createPropsModel(index, true, false, false, false);
        createSign(index, "Far Grab Entity");
    };

    function createPropsModel(i, dynamic, collisionless, clone, cloneDynamic) {
        var propsModel = {
            name: "controller-tests model object " + i,
            type: "Model",
            modelURL: "http://headache.hungry.com/~seth/hifi/controller-tests/color-cube.obj",
            
            position: sectionCenterA,
            rotation: sectionRotation,
            
            gravity: (dynamic && !collisionless) ? { x: 0, y: -1, z: 0 } : { x: 0, y: 0, z: 0 },
            dimensions: { x: 0.2, y: 0.2, z: 0.2 },
            userData: JSON.stringify({
                grabbableKey: {
                    grabbable: true,
                    cloneLimit: 10,
                    cloneable: clone,
                    cloneDynamic: cloneDynamic
                },
                controllerTestEntity: true
            }),
            lifetime: lifetime,
            shapeType: "box",
            dynamic: dynamic,
            collisionless: collisionless
        };
        objects.push(Entities.addEntity(propsModel));
    }

    function createPropsModelOverlay(i, dynamic, collisionless, clone, cloneDynamic) {
        var propsModel = {
            name: "controller-tests model object " + i,
            type: "Model",
            modelURL: "http://headache.hungry.com/~seth/hifi/controller-tests/color-cube.obj",
            url: "http://headache.hungry.com/~seth/hifi/controller-tests/color-cube.obj",
            grabbable: true,
            position: sectionCenterA,
            rotation: sectionRotation,
            dimensions: { x: 0.2, y: 0.2, z: 0.2 },
            userData: JSON.stringify({
                grabbableKey: {
                    grabbable: true,
                },
                controllerTestEntity: true
            }),
            lifetime: lifetime,
            visible: true,
        };
        overlays.push(Overlays.addOverlay("model", propsModel));
    }


    function createPropsCubeOverlay(i, dynamic, collisionless, clone, cloneDynamic) {
        var propsCube = {
            name: "controller-tests cube object " + i,
            type: "Box",
            color: { "blue": 200, "green": 10, "red": 20 },
            position: sectionCenterB,
            rotation: sectionRotation,
            grabbable: true,
            dimensions: { x: 0.2, y: 0.2, z: 0.2 },
            userData: JSON.stringify({
                grabbableKey: {
                    grabbable: true,
                },
                controllerTestEntity: true
            }),
            lifetime: lifetime,
            solid: true,
            visible: true,
        };
        overlays.push(Overlays.addOverlay("cube", propsCube));
    }

    function createPropsCube(i, dynamic, collisionless, clone, cloneDynamic) {
        var propsCube = {
            name: "controller-tests cube object " + i,
            type: "Box",
            shape: "Cube",
            color: { "blue": 200, "green": 10, "red": 20 },
            position: sectionCenterB,
            rotation: sectionRotation,
            gravity: dynamic ? { x: 0, y: -1, z: 0 } : { x: 0, y: 0, z: 0 },
            dimensions: { x: 0.2, y: 0.2, z: 0.2 },
            userData: JSON.stringify({
                grabbableKey: {
                    grabbable: true,
                    cloneLimit: 10,
                    cloneable: clone,
                    cloneDynamic: cloneDynamic
                },
                controllerTestEntity: true
            }),
            lifetime: lifetime,
            shapeType: "box",
            dynamic: dynamic,
            collisionless: collisionless
        };
        objects.push(Entities.addEntity(propsCube));
    }

    function createSign(i, signText) {
        var propsLabel = {
            name: "controller-tests sign " + i,
            type: "Text",
            lineHeight: 0.125,
            position: sectionCenterSign,
            rotation: Quat.multiply(sectionRotation, yFlip),
            text: signText,
            dimensions: { x: 1, y: 1, z: 0.01 },
            lifetime: lifetime,
            userData: JSON.stringify({
                grabbableKey: {
                    grabbable: false,
                },
                controllerTestEntity: true
            })
        };
        objects.push(Entities.addEntity(propsLabel));
    }

    function chooseType(index) {
        switch (index) {
        case 0:
            createFarGrabEntity(index);
            break;
        case 1:
            createNearGrabEntity(index);
            break;
        case 2:
            createNearGrabOverlay(index);
            break;
        case 3:
            createCloneDynamicEntity();
            break;
        case 4:
            createCloneEntity(index);
            break;
        }
    }

    function setupControllerTests(testBaseTransform) {
        // var tableID =
        objects.push(Entities.addEntity({
            name: "controller-tests table",
            type: "Model",
            modelURL: "http://headache.hungry.com/~seth/hifi/controller-tests/controller-tests-table.obj.gz",
            position: Mat4.transformPoint(testBaseTransform, { x: 0, y: 1, z: 0 }),
            rotation: Mat4.extractRotation(testBaseTransform),
            userData: JSON.stringify({
                grabbableKey: { grabbable: false },
                soundKey: {
                    url: "http://headache.hungry.com/~seth/hifi/sound/clock-ticking-3.wav",
                    volume: 0.4,
                    loop: true,
                    playbackGap: 0,
                    playbackGapRange: 0
                },
                controllerTestEntity: true
            }),
            shapeType: "static-mesh",
            lifetime: lifetime
        }));

        var Xdynamic = 1;
        var Xcollisionless = 2;
        var Xkinematic = 4;
        var XignoreIK = 8;

        yFlip = Quat.fromPitchYawRollDegrees(0, 180, 0);

        for (var i = 0; i < 16; i++) {
            sectionRelativeRotation = Quat.fromPitchYawRollDegrees(0, -360 * i / tableSections, 0);
            sectionRotation = Quat.multiply(Mat4.extractRotation(testBaseTransform), sectionRelativeRotation);
            sectionRelativeCenterA = Vec3.multiplyQbyV(sectionRotation, { x: -0.2, y: 1.25, z: tableRadius - 0.8 });
            sectionRelativeCenterB = Vec3.multiplyQbyV(sectionRotation, { x: 0.2, y: 1.25, z: tableRadius - 0.8 });
            sectionRelativeCenterSign = Vec3.multiplyQbyV(sectionRotation, { x: 0, y: 1.5, z: tableRadius + 1.0 });
            sectionCenterA = Mat4.transformPoint(testBaseTransform, sectionRelativeCenterA);
            sectionCenterB = Mat4.transformPoint(testBaseTransform, sectionRelativeCenterB);
            sectionCenterSign = Mat4.transformPoint(testBaseTransform, sectionRelativeCenterSign);

            var dynamic = (i & Xdynamic) ? true : false;
            var collisionless = (i & Xcollisionless) ? true : false;
            var kinematic = (i & Xkinematic) ? true : false;
            var ignoreIK = (i & XignoreIK) ? true : false;

            chooseType(i);
        }
    }

    // This assumes the avatar is standing on a flat floor with plenty of space.
    // Find the floor:
    var pickRay = {
        origin: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 2, z: -1 })),
        direction: { x: 0, y: -1, z: 0 },
        length: 20
    };
    var intersection = Entities.findRayIntersection(pickRay, true, [], [], true);

    if (intersection.intersects) {
        var testBaseTransform = Mat4.createFromRotAndTrans(MyAvatar.rotation, intersection.intersection);
        setupControllerTests(testBaseTransform);
    }

    Script.scriptEnding.connect(function () {
        for (var i = 0; i < objects.length; i++) {
            var nearbyID = objects[i];
            Entities.deleteEntity(nearbyID);
        }

        for (var i = 0; i < overlays.length; i++) {
            var overlayID = overlays[i];
            Overlays.deleteOverlay(overlayID);
        }
    });
}()); // END LOCAL_SCOPE
