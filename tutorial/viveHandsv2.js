var PARENT_ID = MyAvatar.sessionUUID;
var LEFT_JOINT_INDEX = MyAvatar.getJointIndex("_CONTROLLER_LEFTHAND");
var RIGHT_JOINT_INDEX = MyAvatar.getJointIndex("_CONTROLLER_RIGHTHAND");
//var LEFT_JOINT_INDEX = MyAvatar.getJointIndex("LeftHand");
//var RIGHT_JOINT_INDEX = MyAvatar.getJointIndex("RightHand");

var zeroPosition = { x: 0, y: 0, z: 0 };
var zeroRotation = { x: 0, y: 0, z: 0, w: 1 };

var CONTROLLER_LENGTH_OFFSET = 0.0762; 

var naturalPosition = {
    x: 0,
    y: -0.034076502197422087,
    z: 0.06380049744620919
};
var naturalPositionL = {
    x: 0,
    y: 0.034076502197422087,
    z: 0.06380049744620919
};
var naturalPositionR = {
    x: 0.0,
    y: 0.034076502197422087,
    z: 0.06380049744620919
};

var leftBasePosition = {
    x: CONTROLLER_LENGTH_OFFSET / 2,
    y: CONTROLLER_LENGTH_OFFSET * 2,
    z: CONTROLLER_LENGTH_OFFSET / 2
};
var rightBasePosition = {
    x: -CONTROLLER_LENGTH_OFFSET / 2,
    y: CONTROLLER_LENGTH_OFFSET * 2,
    z: CONTROLLER_LENGTH_OFFSET / 2
};

var leftBasePositionVive = Vec3.sum(leftBasePosition, { x: 0.005, y: 0.03, z: 0 });
var rightBasePositionVive = Vec3.sum(rightBasePosition, { x: -0.005, y: 0.03, z: 0 });

Vec3.print("left offset: ", leftBasePosition);

var leftBaseRotation = Quat.multiply(
    Quat.fromPitchYawRollDegrees(0, 0, 45),
    Quat.multiply(
        Quat.fromPitchYawRollDegrees(90, 0, 0),
        Quat.fromPitchYawRollDegrees(0, 0, 90)
    )
);

var rightBaseRotation = Quat.multiply(
    Quat.fromPitchYawRollDegrees(0, 0, -45),
    Quat.multiply(
        Quat.fromPitchYawRollDegrees(90, 0, 0),
        Quat.fromPitchYawRollDegrees(0, 0, -90)
    )
);


var touchLeftBaseRotation = Quat.multiply(
    Quat.fromPitchYawRollDegrees(0, 0, 0),
    Quat.multiply(
        Quat.fromPitchYawRollDegrees(0, 0, -45),
        Quat.multiply(
            Quat.fromPitchYawRollDegrees(180, 0, 0),
            Quat.fromPitchYawRollDegrees(0, -90, 0)
        )
    )
);

var touchRightBaseRotation = Quat.multiply(
    Quat.fromPitchYawRollDegrees(0, 0, 45),
    Quat.multiply(
        Quat.fromPitchYawRollDegrees(180, 0, 0),
        Quat.fromPitchYawRollDegrees(0, 90, 0)
    )
);

var TOUCH_CONTROLLER_CONFIGURATION = {
    name: "Touch",
    controllers: [
        {
            modelURL: "C:/Users/Ryan/Assets/controller/oculus_touch_l.fbx",
            naturalPosition: {
                x: 0.016486000269651413,
                y: -0.035518500953912735,
                z: -0.018527504056692123
            },
            jointIndex: MyAvatar.getJointIndex("_CONTROLLER_LEFTHAND"),
            rotation: touchLeftBaseRotation,
            position: leftBasePosition,

            annotationTextRotation: Quat.fromPitchYawRollDegrees(20, -90, 0),
            annotations: {

                buttonX: {
                    position: {
                        x: -0.00931,
                        y: 0.00212,
                        z: -0.01259,
                    },
                    direction: "left",
                    color: { red: 100, green: 100, blue: 100 },
                },
                buttonY: {
                    position: {
                        x: -0.01617,
                        y: 0.00216,
                        z: 0.00177,
                    },
                    direction: "left",
                    color: { red: 100, green: 255, blue: 100 },
                },
                bumper: {
                    position: {
                        x: 0.00678,
                        y: -0.02740,
                        z: -0.02537,
                    },
                    direction: "left",
                    color: { red: 100, green: 100, blue: 255 },
                },
                trigger: {
                    position: {
                        x: -0.01275,
                        y: -0.01992,
                        z: 0.02314,
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                }
            },
        },
        {
            modelURL: "C:/Users/Ryan/Assets/controller/oculus_touch_r.fbx",
            naturalPosition: {
                x: -0.016486000269651413,
                y: -0.035518500953912735,
                z: -0.018527504056692123
            },
            jointIndex: MyAvatar.getJointIndex("_CONTROLLER_RIGHTHAND"),
            rotation: touchRightBaseRotation,
            position: rightBasePosition,

            annotationTextRotation: Quat.fromPitchYawRollDegrees(20, 90, 0),
            annotations: {

                buttonA: {
                    position: {
                        x: 0.00931,
                        y: 0.00212,
                        z: -0.01259,
                    },
                    direction: "right",
                    color: { red: 100, green: 100, blue: 100 },
                },
                buttonB: {
                    position: {
                        x: 0.01617,
                        y: 0.00216,
                        z: 0.00177,
                    },
                    direction: "right",
                    color: { red: 100, green: 255, blue: 100 },
                },
                bumper: {
                    position: {
                        x: 0.00678,
                        y: -0.02740,
                        z: -0.02537,
                    },
                    direction: "right",
                    color: { red: 100, green: 100, blue: 255 },
                },
                trigger: {
                    position: {
                        x: 0.01275,
                        y: -0.01992,
                        z: 0.02314,
                    },
                    direction: "right",
                    color: { red: 255, green: 100, blue: 100 },
                }
            },
        }
    ]
}


var viveNaturalDimensions = {
    x: 0.1174320001155138,
    y: 0.08361100335605443,
    z: 0.21942697931081057
};
var viveNaturalPosition = {
    x: 0,
    y: -0.034076502197422087,
    z: 0.06380049744620919
};
var viveModelURL = "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive2.fbx";
var viveModelURL = "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_body.fbx";

var VIVE_CONTROLLER_CONFIGURATION = {
    name: "Vive",
    controllers: [
        {
            modelURL: viveModelURL,
            jointIndex: MyAvatar.getJointIndex("_CONTROLLER_LEFTHAND"),
            naturalPosition: viveNaturalPosition,
            rotation: leftBaseRotation,
            position: Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, 0, 45), leftBasePosition),

            dimensions: viveNaturalDimensions,

            parts: {
                //{
                //    type: "linear",
                //    modelURL: "",
                //    input: "Controller.Hardware.Vive.RT",
                //    minValue: 0.0,
                //    maxValue: 1.0,
                //    textOffset: { x: -0.035, y: 0.004, z: -0.005 },
                //    minPosition: { x: -0.035, y: 0.004, z: -0.005 },
                //    maxPosition: { x: -0.035, y: 0.004, z: -0.005 },
                //},

                // The touchpad type draws a dot indicating the current touch/thumb position
                // and swaps in textures based on the thumb position.
                touchpad: {
                    type: "touchpad",
                    //modelURL: "file:///C:\\Users\\Ryan\\Assets\\controller\\vive_trackpad.fbx",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_trackpad.fbx",
                    visibleInput: "Vive.RSTouch",
                    xInput: "Vive.LX",
                    yInput: "Vive.LY",
                    naturalPosition: {"x":0,"y":0.000979491975158453,"z":0.04872849956154823},
                    minValue: 0.0,
                    maxValue: 1.0,
                    minPosition: { x: -0.035, y: 0.004, z: -0.005 },
                    maxPosition: { x: -0.035, y: 0.004, z: -0.005 },
                    textureName: "Tex.touchpad-blank",

                    defaultTextureLayer: "blank",
                    textureLayers: {
                        blank: {
                            defaultTextureURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_trackpad.fbx/Touchpad.fbm/touchpad-blank.jpg",
                        },
                        teleport: {
                            defaultTextureURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_trackpad.fbx/Touchpad.fbm/touchpad-teleport.jpg",
                        },
                        arrows: {
                            defaultTextureURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_trackpad.fbx/Touchpad.fbm/touchpad-look-arrows.jpg",
                        }
                    }
                },

                trigger: {
                    type: "rotational",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_trigger.fbx",
                    input: Controller.Standard.LT,
                    naturalPosition: {"x":0.000004500150680541992,"y":-0.027690507471561432,"z":0.04830199480056763},
                    origin: { x: 0, y: -0.015, z: -0.00 },
                    minValue: 0.0,
                    maxValue: 1.0,
                    axis: { x: -1, y: 0, z: 0 },
                    maxAngle: 25,
                },

                l_grip: {
                    type: "ignore",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_l_grip.fbx",
                    naturalPosition: {"x":-0.01720449887216091,"y":-0.014324013143777847,"z":0.08714400231838226},
                },

                r_grip: {
                    type: "ignore",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_r_grip.fbx",
                    naturalPosition: {"x":0.01720449887216091,"y":-0.014324013143777847,"z":0.08714400231838226},
                },

                sys_button: {
                    type: "ignore",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_sys_button.fbx",
                    naturalPosition: {"x":0,"y":0.0020399854984134436,"z":0.08825899660587311},
                },

                button: {
                    type: "ignore",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_button.fbx",
                    naturalPosition: {"x":0,"y":0.005480996798723936,"z":0.019918499514460564}
                },
                button2: {
                    type: "ignore",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_button.fbx",
                    naturalPosition: {"x":0,"y":0.005480996798723936,"z":0.019918499514460564}
                },
            },
            annotationTextRotation: Quat.fromPitchYawRollDegrees(45, -90, 0),
            annotations: {
//                red: {
//                    debug: true,
//                    position: {
//                        x: 0.1,
//                        y: 0.0,
//                        z: 0.0
//                    },
//                    direction: "right",
//                    color: { red: 255, green: 0, blue: 0 },
//                },
//                green: {
//                    debug: true,
//                    position: {
//                        x: 0.0,
//                        y: 0.1,
//                        z: 0.0
//                    },
//                    direction: "right",
//                    color: { red: 0, green: 255, blue: 0 },
//                },
//                blue: {
//                    debug: true,
//                    position: {
//                        x: 0.0,
//                        y: 0.0,
//                        z: 0.1
//                    },
//                    direction: "right",
//                    color: { red: 0, green: 0, blue: 255 },
//                },
//                white: {
//                    debug: true,
//                    position: {
//                        x: 0.0,
//                        y: 0.0,
//                        z: 0.0
//                    },
//                    direction: "right",
//                    color: { red: 255, green: 255, blue: 255 },
//                },

                // center: {
                //     position: zeroPosition,
                //     direction: "center",
                //     color: { red: 100, green: 255, blue: 255 },
                // },

                left: {
                    textOffset: { x: -0.035, y: 0.004, z: -0.005 },
                    position: {
                        x: 0,
                        y: 0.00378,
                        z: 0.04920
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
                right: {
                    textOffset: { x: 0.023, y: 0.004, z: -0.005 },
                    position: {
                        x: 0,
                        y: 0.00378,
                        z: 0.04920
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },


                trigger: {
                    position: {
                        x: 0.0055,
                        y: -0.032978,
                        z: 0.04546
                    },
                    lineHeight: 0.013,
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
                menu: {
                    position: {
                        x: 0,
                        y: 0.00770,
                        z: 0.01979
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
                grip: {
                    position: {
                        x: 0.01980,
                        y: -0.01561,
                        z: 0.08721
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
                teleport: {
                    textOffset: { x: -0.015, y: 0.004, z: -0.005 },
                    position: {
                        x: 0,
                        y: 0.00378,
                        z: 0.04920
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
                steam: {
                    position: {
                        x: 0,
                        y: 0.00303,
                        z: 0.08838
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
            },
        },
        {
            modelURL: viveModelURL,
            jointIndex: MyAvatar.getJointIndex("_CONTROLLER_RIGHTHAND"),

            rotation: rightBaseRotation,
            position: Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, 0, -45), rightBasePosition),

            dimensions: viveNaturalDimensions,

            naturalPosition: {
                x: 0,
                y: -0.034076502197422087,
                z: 0.06380049744620919
            },

            parts: {
                //{
                //    type: "linear",
                //    modelURL: "",
                //    input: "Controller.Hardware.Vive.RT",
                //    minValue: 0.0,
                //    maxValue: 1.0,
                //    textOffset: { x: -0.035, y: 0.004, z: -0.005 },
                //    minPosition: { x: -0.035, y: 0.004, z: -0.005 },
                //    maxPosition: { x: -0.035, y: 0.004, z: -0.005 },
                //},

                // The touchpad type draws a dot indicating the current touch/thumb position
                // and swaps in textures based on the thumb position.
                touchpad: {
                    type: "touchpad",
                    //modelURL: "file:///C:\\Users\\Ryan\\Assets\\controller\\vive_trackpad.fbx",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_trackpad.fbx",
                    visibleInput: "Vive.RSTouch",
                    xInput: "Vive.RX",
                    yInput: "Vive.RY",
                    naturalPosition: {"x":0,"y":0.000979491975158453,"z":0.04872849956154823},
                    minValue: 0.0,
                    maxValue: 1.0,
                    minPosition: { x: -0.035, y: 0.004, z: -0.005 },
                    maxPosition: { x: -0.035, y: 0.004, z: -0.005 },
                    textureName: "Tex.touchpad-blank",

                    defaultTextureLayer: "blank",
                    textureLayers: {
                        blank: {
                            defaultTextureURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_trackpad.fbx/Touchpad.fbm/touchpad-blank.jpg",
                        },
                        teleport: {
                            defaultTextureURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_trackpad.fbx/Touchpad.fbm/touchpad-teleport.jpg",
                        },
                        arrows: {
                            defaultTextureURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_trackpad.fbx/Touchpad.fbm/touchpad-look-arrows.jpg",
                        }
                    }
                },

                trigger: {
                    type: "rotational",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_trigger.fbx",
                    input: Controller.Standard.RT,
                    naturalPosition: {"x":0.000004500150680541992,"y":-0.027690507471561432,"z":0.04830199480056763},
                    origin: { x: 0, y: -0.015, z: -0.00 },
                    minValue: 0.0,
                    maxValue: 1.0,
                    axis: { x: -1, y: 0, z: 0 },
                    maxAngle: 25,
                },

                l_grip: {
                    type: "ignore",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_l_grip.fbx",
                    naturalPosition: {"x":-0.01720449887216091,"y":-0.014324013143777847,"z":0.08714400231838226},
                },

                r_grip: {
                    type: "ignore",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_r_grip.fbx",
                    naturalPosition: {"x":0.01720449887216091,"y":-0.014324013143777847,"z":0.08714400231838226},
                },

                sys_button: {
                    type: "ignore",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_sys_button.fbx",
                    naturalPosition: {"x":0,"y":0.0020399854984134436,"z":0.08825899660587311},
                },

                button: {
                    type: "ignore",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_button.fbx",
                    naturalPosition: {"x":0,"y":0.005480996798723936,"z":0.019918499514460564}
                },
                button2: {
                    type: "ignore",
                    modelURL: "https://hifi-public.s3.amazonaws.com/huffman/controllers/vive_button.fbx",
                    naturalPosition: {"x":0,"y":0.005480996798723936,"z":0.019918499514460564}
                },
            },

            annotationTextRotation: Quat.fromPitchYawRollDegrees(180 + 45, 90, 180),
            annotations: {

                left: {
                    textOffset: { x: -0.035, y: 0.004, z: -0.005 },
                    position: {
                        x: 0,
                        y: 0.00378,
                        z: 0.04920
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
                right: {
                    textOffset: { x: 0.023, y: 0.004, z: -0.005 },
                    position: {
                        x: 0,
                        y: 0.00378,
                        z: 0.04920
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },

                trigger: {
                    position: {
                        x: -0.075,
                        y: -0.032978,
                        z: 0.04546
                    },
                    lineHeight: 0.013,
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
                menu: {
                    position: {
                        x: 0,
                        y: 0.00770,
                        z: 0.01979
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
                grip: {
                    position: {
                        x: 0.01980,
                        y: -0.01561,
                        z: 0.08721
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
                teleport: {
                    textOffset: { x: -0.015, y: 0.004, z: -0.005 },
                    position: {
                        x: 0,
                        y: 0.00378,
                        z: 0.04920
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
                steam: {
                    position: {
                        x: 0,
                        y: 0.00303,
                        z: 0.08838
                    },
                    direction: "left",
                    color: { red: 255, green: 100, blue: 100 },
                },
            }
        }
    ]
}

var DEBUG = false;
var VISIBLE_BY_DEFAULT = false;

function setupController(config) {
    var controllerDisplay = {
        overlays: [],
        partOverlays: {
        },
        parts: {
        },
        annotations: {
        },
        mappingName: "mapping-display",

        setPartVisible: function(partName, visible) {
            print("Setting part visible", partName, visible);
            if (partName in this.partOverlays) {
                print("FOUND");
                for (var i = 0; i < this.partOverlays[partName].length; ++i) {
                    Overlays.editOverlay(this.partOverlays[partName][i], {
                        visible: visible 
                    });
                }
            }
        },

        setLayerForPart: function(partName, layerName) {
            print("Setting layer...", partName, layerName);
            if (partName in this.parts) {
                var part = this.parts[partName];
                print("FOnd", JSON.stringify(part));
                if (layerName in part.textureLayers) {
                    print("got it", layerName);
                    var layer = part.textureLayers[layerName];
                    var textures = {};
                    if (layer.defaultTextureURL) {
                        print("default texture");
                        textures[part.textureName] = layer.defaultTextureURL;
                    }
                    for (var i = 0; i < this.partOverlays[partName].length; ++i) {
                        print("updating", JSON.stringify(textures));
                        Overlays.editOverlay(this.partOverlays[partName][i], {
                            textures: textures
                        });
                    }
                }
            }
        }
    };
    var mapping = Controller.newMapping(controllerDisplay.mappingName);
    for (var i = 0; i < config.controllers.length; ++i) {
        var controller = config.controllers[i];
        var position = controller.position;
        if (controller.naturalPosition) {
            position = Vec3.sum(Vec3.multiplyQbyV(
                        controller.rotation, controller.naturalPosition), position);
        }
        var overlayID = Overlays.addOverlay("model", {
            url: controller.modelURL,
            dimensions: controller.dimensions,
            localRotation: controller.rotation,
            localPosition: position,
            parentID: PARENT_ID,
            parentJointIndex: controller.jointIndex,
            ignoreRayIntersection: true,
        });

        controllerDisplay.overlays.push(overlayID);

        if (controller.annotations) {
            for (var key in controller.annotations) {
                var annotation = controller.annotations[key];
                var annotationPosition = Vec3.sum(controller.position, Vec3.multiplyQbyV(controller.rotation, annotation.position));
                if (DEBUG) {
                    overlayID = Overlays.addOverlay("sphere", {
                        localPosition: annotationPosition,
                        //localPosition: Vec3.sum(controller.position, annotation.position),
                        //localPosition: Vec3.sum(position, annotation.position),
                        color: annotation.color || { red: 255, green: 100, blue: 100 },
                        dimensions: {
                            x: 0.01,
                            y: 0.01,
                            z: 0.01
                        },
                        parentID: PARENT_ID,
                        parentJointIndex: controller.jointIndex,
                    });
                    controllerDisplay.overlays.push(overlayID);

                }

                var ANNOTATION_TEXT_OFFSET = 0.1;
                var sign = annotation.direction == "right" ? 1 : -1;
                var textOffset = annotation.direction == "right" ? 0.08 : 0.02;
                if (annotation.textOffset) {
                    var pos = Vec3.sum(annotationPosition, Vec3.multiplyQbyV(controller.rotation, annotation.textOffset));
                } else {
                    var pos = Vec3.sum(annotationPosition, Vec3.multiplyQbyV(controller.rotation, { x: textOffset, y: 0, z: -0.005 }));
                }
                var textOverlayID = Overlays.addOverlay("text3d", {
                    visible: VISIBLE_BY_DEFAULT, 
                    text: key,
                    localPosition: pos,
                    localRotation: controller.annotationTextRotation,
                    lineHeight: annotation.lineHeight ? annotation.lineHeight : 0.01,
                    leftMargin: 0,
                    rightMargin: 0,
                    topMargin: 0,
                    bottomMargin: 0,
                    backgroundAlpha: 0,
                    dimensions: { x: 0.003, y: 0.003, z: 0.003 },
                    //localPosition: Vec3.sum(controller.position, annotation.position),
                    //localPosition: Vec3.sum(position, annotation.position),
                    color: annotation.textColor || { red: 255, green: 255, blue: 255 },
                    parentID: PARENT_ID,
                    parentJointIndex: controller.jointIndex,
                });

                controllerDisplay.overlays.push(textOverlayID);
                if (key in controllerDisplay.annotations) {
                    controllerDisplay.annotations[key].push(textOverlayID);
                } else {
                    controllerDisplay.annotations[key] = [textOverlayID];
                }

                var ANNOTATION_OFFSET = 0.5;
                var offset = { x: 0, y: 0, z: annotation.direction == "right" ? -1 * ANNOTATION_OFFSET : ANNOTATION_OFFSET };
                var lineOverlayID = Overlays.addOverlay("line3d", {
                    visible: false,
                    localPosition: annotationPosition,
                    localStart: { x: 0, y: 0, z: 0 },
                    localEnd: offset,
                    //localPosition: Vec3.sum(controller.position, annotation.position),
                    //localPosition: Vec3.sum(position, annotation.position),
                    color: annotation.color || { red: 255, green: 100, blue: 100 },
                    parentID: PARENT_ID,
                    parentJointIndex: controller.jointIndex,
                });
                controllerDisplay.overlays.push(lineOverlayID);
            }
        }

        function clamp(value, min, max) {
            if (value < min) {
                return min;
            } else if (value > max) {
                return max
            }
            return value;
        }

        if (controller.parts) {
            for (var partName in controller.parts) {
                var part = controller.parts[partName];
                var partPosition = Vec3.sum(controller.position, Vec3.multiplyQbyV(controller.rotation, part.naturalPosition));
                var innerRotation = controller.rotation

                Vec3.print("controller", controller.position);
                Vec3.print("part", partPosition);

                controllerDisplay.parts[partName] = controller.parts[partName];

                var overlayID = Overlays.addOverlay("model", {
                    url: part.modelURL,
                    localPosition: partPosition,
                    localRotation: innerRotation,
                    parentID: PARENT_ID,
                    parentJointIndex: controller.jointIndex,
                    ignoreRayIntersection: true,
                    //visible: false
                });

                if (part.type == "rotational") {
                    var range = part.maxValue - part.minValue;
                    mapping.from([part.input]).peek().to(function(controller, overlayID, part) {
                        return function(value) {
                            //print(value);
                            //print(JSON.stringify(part));

                            value = clamp(value, part.minValue, part.maxValue);

                            var pct = (value - part.minValue) / part.maxValue;
                            var angle = pct * part.maxAngle;
                            var rotation = Quat.angleAxis(angle, part.axis);
                            print(value, pct, angle);

                            var offset = { x: 0, y: 0, z: 0 };
                            if (part.origin) {
                                //print(rotation.x, rotation.y, rotation.z, rotation.w);
                                var offset = Vec3.multiplyQbyV(rotation, part.origin);
                                offset = Vec3.subtract(offset, part.origin);
                                Vec3.print('offset', offset);
                                //partPosition = Vec3.sum(partPosition, offset);
                            }

                            var partPosition = Vec3.sum(controller.position,
                                    Vec3.multiplyQbyV(controller.rotation, Vec3.sum(offset, part.naturalPosition)));

                            Overlays.editOverlay(overlayID, {
                                localPosition: partPosition,
                                localRotation: Quat.multiply(controller.rotation, rotation)
                            });
                        }
                    }(controller, overlayID, part));
                } else if (part.type == "touchpad") {
                    function resolveHardware(path) {
                        var parts = path.split(".");
                        function resolveInner(base, path, i) {
                            print(path[i]);
                            if (i >= path.length) {
                                return base;
                            }
                            return resolveInner(base[path[i]], path, ++i);
                        }
                        return resolveInner(Controller.Hardware, parts, 0);
                    }

                    var visibleInput = resolveHardware(part.visibleInput);
                    var xinput = resolveHardware(part.xInput);
                    var yinput = resolveHardware(part.yInput);

                    print("visible:", visibleInput);

                    mapping.from([visibleInput]).peek().to(function(value) {
                        print("visible", value);
                    });
                    mapping.from([xinput]).peek().to(function(value) {
                        print("X", value);
                    });
                    mapping.from([yinput]).peek().invert().to(function(value) {
                        print("Y", value);
                    });
                    if (part.defaultTextureURL) {
                        var textures = {};
                        textures[part.textureName] = part.defaultTextureURL;
                        Overlays.editOverlay(overlayID, {
                            textures: textures
                        });
                    }
                } else {
                    print("TYPE NOT SUPPORTED: ", part.type);
                }

                controllerDisplay.overlays.push(overlayID);
                if (!(partName in controllerDisplay.partOverlays)) {
                    controllerDisplay.partOverlays[partName] = [];
                }
                controllerDisplay.partOverlays[partName].push(overlayID);
            }
        }
    }
    Controller.enableMapping(controllerDisplay.mappingName);
    return controllerDisplay;
}

ControllerDisplay = function() {
};

function deleteControllerDisplay(controllerDisplay) {
    for (var i = 0; i < controllerDisplay.overlays.length; ++i) {
        Overlays.deleteOverlay(controllerDisplay.overlays[i]);
    }
    Controller.disableMapping(controllerDisplay.mappingName);
}

var overlays = [
];


Messages.subscribe('Controller-Display');
var handleMessages = function(channel, message, sender) {
    print("MESSASGE>>>>", channel, message, sender);
    if (sender === MyAvatar.sessionUUID) {
        if (channel === 'Controller-Display') {
            print('here');
            var data = JSON.parse(message);
            var name = data.name;
            var visible = data.visible;
            //c.setDisplayAnnotation(name, visible);
            if (name in c.annotations) {
                print("hiding");
                for (var i = 0; i < c.annotations[name].length; ++i) {
                    print("hiding", i);
                    Overlays.editOverlay(c.annotations[name][i], { visible: visible });
                }
            }
        } else if (channel === 'Controller-Display-Parts') {
            print('here part');
            var data = JSON.parse(message);
            for (var name in data) {
                var visible = data[name];
                c.setPartVisible(name, visible);
            }
        } else if (channel === 'Controller-Set-Part-Layer') {
            var data = JSON.parse(message);
            for (var name in data) {
                var layer = data[name];
                c.setLayerForPart(name, layer);
            }
        }
    }
}


Messages.messageReceived.connect(handleMessages);

var MAPPING_NAME = "com.highfidelity.handControllerGrab.disable";
var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from([Controller.Standard.LT]).to(function(value) {
    // print(value);
    // Overlays.editOverlay(leftTriggerOverlayID, {
    //     localRotation: Quat.multiply(Quat.fromPitchYawRollDegrees(0, 0, value * -45), leftBaseRotation)
    // });
});
mapping.from([Controller.Standard.RT]).to(function(value) {
    // print(value);
    // Overlays.editOverlay(rightTriggerOverlayID, {
    //     localRotation: Quat.multiply(Quat.fromPitchYawRollDegrees(0, 0, value * 45), rightBaseRotation)
    // });
});
Controller.enableMapping(MAPPING_NAME);

//var c = setupController(TOUCH_CONTROLLER_CONFIGURATION);
var c = setupController(VIVE_CONTROLLER_CONFIGURATION);
//c.setPartVisible("touchpad", false);
//c.setPartVisible("touchpad_teleport", false);
//layers = ["blank", "teleport", 'arrows'];
//num = 0;
//Script.setInterval(function() {
//    print('num', num);
//    num = (num + 1) % layers.length;
//    c.setLayerForPart("touchpad", layers[num]);
//}, 2000);

Script.scriptEnding.connect(function() {
    deleteControllerDisplay(c);
    MyAvatar.shouldRenderLocally = true;
    for (var i = 0; i < overlays.length; ++i) {
        Overlays.deleteOverlay(overlays[i]);
    }
    Controller.disableMapping(MAPPING_NAME);
});
