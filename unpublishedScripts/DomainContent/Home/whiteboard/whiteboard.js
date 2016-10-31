//  Created by James B. Pollack @imgntn 6/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//

(function() {

    var _this;
    var MARKER_SCRIPT_URL = "atp:/whiteboard/markerEntityScript.js";
    var ERASER_SCRIPT_URL = "atp:/whiteboard/eraserEntityScript.js";

    Whiteboard = function() {
        _this = this;
    }

    Whiteboard.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            this.setup();
        },
        unload: function() {

        },
        setup: function() {

            var props = Entities.getEntityProperties(_this.entityID);
            this.spawnRotation = Quat.safeEulerAngles(props.rotation);
            this.spawnPosition = props.position;
            this.orientation = Quat.fromPitchYawRollDegrees(this.spawnRotation.x, this.spawnRotation.y, this.spawnRotation.z);
            this.markerRotation = Quat.fromVec3Degrees({
                x: this.spawnRotation.x + 10,
                y: this.spawnRotation.y - 90,
                z: this.spawnRotation.z
            });

        },
        createMarkers: function() {
            _this.setup();
            var modelURLS = [
                "atp:/whiteboard/marker-blue.fbx",
                "atp:/whiteboard/marker-red.fbx",
                "atp:/whiteboard/marker-black.fbx",
            ];

            var markerPosition = Vec3.sum(_this.spawnPosition, Vec3.multiply(Quat.getFront(_this.orientation), -0.1));

            _this.createMarker(modelURLS[0], markerPosition, {
                red: 10,
                green: 10,
                blue: 200
            });

            _this.createMarker(modelURLS[1], markerPosition, {
                red: 200,
                green: 10,
                blue: 10
            });

            _this.createMarker(modelURLS[2], markerPosition, {
                red: 10,
                green: 10,
                blue: 10
            });
        },
        createMarker: function(modelURL, markerPosition, markerColor) {

            var markerProperties = {
                type: "Model",
                modelURL: modelURL,
                rotation: _this.markerRotation,
                shapeType: "box",
                name: "hifi_model_marker",
                dynamic: true,
                gravity: {
                    x: 0,
                    y: -5,
                    z: 0
                },
                velocity: {
                    x: 0,
                    y: -0.1,
                    z: 0
                },
                position: markerPosition,
                dimensions: {
                    x: 0.027,
                    y: 0.027,
                    z: 0.164
                },
                lifetime: 86400,
                script: MARKER_SCRIPT_URL,
                userData: JSON.stringify({
                    'grabbableKey': {
                        'grabbable': true
                    },
                    'hifiHomeKey': {
                        'reset': true
                    },
                    originalPosition: markerPosition,
                    originalRotation: _this.markerRotation,
                    markerColor: markerColor,
                    wearable: {
                        joints: {
                            RightHand: [{
                                x: 0.001,
                                y: 0.139,
                                z: 0.050
                            }, {
                                x: -0.73,
                                y: -0.043,
                                z: -0.108,
                                w: -0.666
                            }],
                            LeftHand: [{
                                x: 0.007,
                                y: 0.151,
                                z: 0.061
                            }, {
                                x: -0.417,
                                y: 0.631,
                                z: -0.389,
                                w: -0.525
                            }]
                        }
                    }
                })
            }

            var marker = Entities.addEntity(markerProperties);

        },
        createEraser: function() {
            _this.setup();
            var ERASER_MODEL_URL = "atp:/whiteboard/eraser-2.fbx";

            var eraserPosition = Vec3.sum(_this.spawnPosition, Vec3.multiply(Quat.getFront(_this.orientation), -0.1));
            eraserPosition = Vec3.sum(eraserPosition, Vec3.multiply(-0.5, Quat.getRight(_this.orientation)));
            var eraserRotation = _this.markerRotation;

            var eraserProps = {
                type: "Model",
                name: "hifi_model_whiteboardEraser",
                modelURL: ERASER_MODEL_URL,
                position: eraserPosition,
                script: ERASER_SCRIPT_URL,
                shapeType: "box",
                lifetime: 86400,
                dimensions: {
                    x: 0.0858,
                    y: 0.0393,
                    z: 0.2083
                },
                rotation: eraserRotation,
                dynamic: true,
                gravity: {
                    x: 0,
                    y: -10,
                    z: 0
                },
                velocity: {
                    x: 0,
                    y: -0.1,
                    z: 0
                },
                userData: JSON.stringify({
                    'hifiHomeKey': {
                        'reset': true
                    },
                    'grabbableKey': {
                        'grabbable': true
                    },
                    originalPosition: eraserPosition,
                    originalRotation: eraserRotation,
                    wearable: {
                        joints: {
                            RightHand: [{
                                x: 0.020,
                                y: 0.120,
                                z: 0.049
                            }, {
                                x: 0.1004,
                                y: 0.6424,
                                z: 0.717,
                                w: 0.250
                            }],
                            LeftHand: [{
                                x: -0.005,
                                y: 0.1101,
                                z: 0.053
                            }, {
                                x: 0.723,
                                y: 0.289,
                                z: 0.142,
                                w: 0.610
                            }]
                        }
                    }
                })
            }

            var eraser = Entities.addEntity(eraserProps);
        }

    }


    return new Whiteboard();
})