var LEFT_JOINT_INDEX = MyAvatar.getJointIndex("_CONTROLLER_LEFTHAND");
var RIGHT_JOINT_INDEX = MyAvatar.getJointIndex("_CONTROLLER_RIGHTHAND");

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

// keep these in sync with the values from plugins/openvr/src/OpenVrHelpers.cpp:303
var CONTROLLER_LATERAL_OFFSET = 0.0381;
var CONTROLLER_VERTICAL_OFFSET = 0.0495;
var CONTROLLER_FORWARD_OFFSET = 0.1371;
var leftBasePosition = {
    x: CONTROLLER_VERTICAL_OFFSET,
    y: CONTROLLER_FORWARD_OFFSET,
    z: CONTROLLER_LATERAL_OFFSET
};
var rightBasePosition = {
    x: -CONTROLLER_VERTICAL_OFFSET,
    y: CONTROLLER_FORWARD_OFFSET,
    z: CONTROLLER_LATERAL_OFFSET
};

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

var viveModelURL = "atp:/controller/vive_body.fbx";
var viveTipsModelURL = "atp:/controller/vive_tips.fbx"

VIVE_CONTROLLER_CONFIGURATION_LEFT = {
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
                tips: {
                    type: "static",
                    modelURL: viveTipsModelURL,
                    naturalPosition: {"x":-0.004377640783786774,"y":-0.034371938556432724,"z":0.06769277155399323},

                    textureName: "Tex.Blank",

                    defaultTextureLayer: "blank",
                    textureLayers: {
                        blank: {
                            defaultTextureURL: viveTipsModelURL + "/Controller-Tips.fbm/Blank.png",
                        },
                        trigger: {
                            defaultTextureURL: viveTipsModelURL + "/Controller-Tips.fbm/Trigger.png",
                        },
                        arrows: {
                            defaultTextureURL: viveTipsModelURL + "/Controller-Tips.fbm/Rotate.png",
                        },
                        grip: {
                            defaultTextureURL: viveTipsModelURL + "/Controller-Tips.fbm/Grip.png",
                        },
                        teleport: {
                            defaultTextureURL: viveTipsModelURL + "/Controller-Tips.fbm/Teleport.png",
                        },
                    }
                },

                // The touchpad type draws a dot indicating the current touch/thumb position
                // and swaps in textures based on the thumb position.
                touchpad: {
                    type: "touchpad",
                    modelURL: "atp:/controller/vive_trackpad.fbx",
                    visibleInput: "Vive.RSTouch",
                    xInput: "Vive.LX",
                    yInput: "Vive.LY",
                    naturalPosition: {"x":0,"y":0.000979491975158453,"z":0.04872849956154823},
                    minValue: 0.0,
                    maxValue: 1.0,
                    minPosition: { x: -0.035, y: 0.004, z: -0.005 },
                    maxPosition: { x: -0.035, y: 0.004, z: -0.005 },
                    disable_textureName: "Tex.touchpad-blank",

                    disable_defaultTextureLayer: "blank",
                    disable_textureLayers: {
                        blank: {
                            defaultTextureURL: "atp:/controller/vive_trackpad.fbx/Touchpad.fbm/touchpad-blank.jpg",
                        },
                        teleport: {
                            defaultTextureURL: "atp:/controller/vive_trackpad.fbx/Touchpad.fbm/touchpad-teleport-active-LG.jpg",
                        },
                        arrows: {
                            defaultTextureURL: "atp:/controller/vive_trackpad.fbx/Touchpad.fbm/touchpad-look-arrows.jpg",
                        }
                    }
                },

                trigger: {
                    type: "rotational",
                    modelURL: "atp:/controller/vive_trigger.fbx",
                    input: Controller.Standard.LT,
                    naturalPosition: {"x":0.000004500150680541992,"y":-0.027690507471561432,"z":0.04830199480056763},
                    origin: { x: 0, y: -0.015, z: -0.00 },
                    minValue: 0.0,
                    maxValue: 1.0,
                    axis: { x: -1, y: 0, z: 0 },
                    maxAngle: 20,
                },

                l_grip: {
                    type: "static",
                    modelURL: "atp:/controller/vive_l_grip.fbx",
                    naturalPosition: {"x":-0.01720449887216091,"y":-0.014324013143777847,"z":0.08714400231838226},
                },

                r_grip: {
                    type: "static",
                    modelURL: "atp:/controller/vive_r_grip.fbx",
                    naturalPosition: {"x":0.01720449887216091,"y":-0.014324013143777847,"z":0.08714400231838226},
                },

                sys_button: {
                    type: "static",
                    modelURL: "atp:/controller/vive_sys_button.fbx",
                    naturalPosition: {"x":0,"y":0.0020399854984134436,"z":0.08825899660587311},
                },

                button: {
                    type: "static",
                    modelURL: "atp:/controller/vive_button.fbx",
                    naturalPosition: {"x":0,"y":0.005480996798723936,"z":0.019918499514460564}
                },
                button2: {
                    type: "static",
                    modelURL: "atp:/controller/vive_button.fbx",
                    naturalPosition: {"x":0,"y":0.005480996798723936,"z":0.019918499514460564}
                },
            },
        },
    ]
};



VIVE_CONTROLLER_CONFIGURATION_RIGHT = {
    name: "Vive Right",
    controllers: [
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
                tips: {
                    type: "static",
                    modelURL: viveTipsModelURL,
                    naturalPosition: {"x":-0.004377640783786774,"y":-0.034371938556432724,"z":0.06769277155399323},

                    textureName: "Tex.Blank",

                    defaultTextureLayer: "blank",
                    textureLayers: {
                        blank: {
                            defaultTextureURL: viveTipsModelURL + "/Controller-Tips.fbm/Blank.png",
                        },
                        trigger: {
                            defaultTextureURL: viveTipsModelURL + "/Controller-Tips.fbm/Trigger.png",
                        },
                        arrows: {
                            defaultTextureURL: viveTipsModelURL + "/Controller-Tips.fbm/Rotate.png",
                        },
                        grip: {
                            defaultTextureURL: viveTipsModelURL + "/Controller-Tips.fbm/Grip.png",
                        },
                        teleport: {
                            defaultTextureURL: viveTipsModelURL + "/Controller-Tips.fbm/Teleport.png",
                        },
                    }
                },

                // The touchpad type draws a dot indicating the current touch/thumb position
                // and swaps in textures based on the thumb position.
                touchpad: {
                    type: "touchpad",
                    modelURL: "atp:/controller/vive_trackpad.fbx",
                    visibleInput: "Vive.RSTouch",
                    xInput: "Vive.RX",
                    yInput: "Vive.RY",
                    naturalPosition: { x: 0, y: 0.000979491975158453, z: 0.04872849956154823 },
                    minValue: 0.0,
                    maxValue: 1.0,
                    minPosition: { x: -0.035, y: 0.004, z: -0.005 },
                    maxPosition: { x: -0.035, y: 0.004, z: -0.005 },
                    disable_textureName: "Tex.touchpad-blank",

                    disable_defaultTextureLayer: "blank",
                    disable_textureLayers: {
                        blank: {
                            defaultTextureURL: "atp:/controller/vive_trackpad.fbx/Touchpad.fbm/touchpad-blank.jpg",
                        },
                        teleport: {
                            defaultTextureURL: "atp:/controller/vive_trackpad.fbx/Touchpad.fbm/touchpad-teleport-active-LG.jpg",
                        },
                        arrows: {
                            defaultTextureURL: "atp:/controller/vive_trackpad.fbx/Touchpad.fbm/touchpad-look-arrows-active.jpg",
                        }
                    }
                },

                trigger: {
                    type: "rotational",
                    modelURL: "atp:/controller/vive_trigger.fbx",
                    input: Controller.Standard.RT,
                    naturalPosition: {"x":0.000004500150680541992,"y":-0.027690507471561432,"z":0.04830199480056763},
                    origin: { x: 0, y: -0.015, z: -0.00 },
                    minValue: 0.0,
                    maxValue: 1.0,
                    axis: { x: -1, y: 0, z: 0 },
                    maxAngle: 25,
                },

                l_grip: {
                    type: "static",
                    modelURL: "atp:/controller/vive_l_grip.fbx",
                    naturalPosition: {"x":-0.01720449887216091,"y":-0.014324013143777847,"z":0.08714400231838226},
                },

                r_grip: {
                    type: "static",
                    modelURL: "atp:/controller/vive_r_grip.fbx",
                    naturalPosition: {"x":0.01720449887216091,"y":-0.014324013143777847,"z":0.08714400231838226},
                },

                sys_button: {
                    type: "static",
                    modelURL: "atp:/controller/vive_sys_button.fbx",
                    naturalPosition: {"x":0,"y":0.0020399854984134436,"z":0.08825899660587311},
                },

                button: {
                    type: "static",
                    modelURL: "atp:/controller/vive_button.fbx",
                    naturalPosition: {"x":0,"y":0.005480996798723936,"z":0.019918499514460564}
                },
                button2: {
                    type: "static",
                    modelURL: "atp:/controller/vive_button.fbx",
                    naturalPosition: {"x":0,"y":0.005480996798723936,"z":0.019918499514460564}
                },
            },
        }
    ]
};

