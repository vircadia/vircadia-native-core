
//
//  viveControllerConfiguration.js
//
//  Created by Ryan Huffman on 12/06/16
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* globals TOUCH_CONTROLLER_CONFIGURATION_LEFT:true TOUCH_CONTROLLER_CONFIGURATION_RIGHT:true */
/* eslint camelcase: ["error", { "properties": "never" }] */

var leftBaseRotation = Quat.multiply(
    Quat.fromPitchYawRollDegrees(0, 0, 0),
    Quat.multiply(
        Quat.fromPitchYawRollDegrees(-90, 0, 0),
        Quat.fromPitchYawRollDegrees(0, 0, 90)
    )
);
//var leftBaseRotation = Quat.fromPitchYawRollDegrees(0, 0, 0);

var rightBaseRotation = Quat.multiply(
    Quat.fromPitchYawRollDegrees(0, 0, 0),
    Quat.multiply(
        Quat.fromPitchYawRollDegrees(-90, 0, 0),
        Quat.fromPitchYawRollDegrees(0, 0, -90)
    )
);

// keep these in sync with the values from SteamVRHelpers.cpp
var CONTROLLER_LENGTH_OFFSET = 0.0762;
var CONTROLLER_LATERAL_OFFSET = 0.0381;
var CONTROLLER_VERTICAL_OFFSET = 0.0381;
var CONTROLLER_FORWARD_OFFSET = 0.1524;

var leftBasePosition = Vec3.multiplyQbyV(leftBaseRotation, {
    x: -CONTROLLER_LENGTH_OFFSET / 2.0,
    y: CONTROLLER_LENGTH_OFFSET / 2.0,
    z: CONTROLLER_LENGTH_OFFSET * 1.5
});
var rightBasePosition = Vec3.multiplyQbyV(rightBaseRotation, {
    x: CONTROLLER_LENGTH_OFFSET / 2.0,
    y: CONTROLLER_LENGTH_OFFSET / 2.0,
    z: CONTROLLER_LENGTH_OFFSET * 1.5
});

var BASE_URL = Script.resourcesPath() + "meshes/controller/touch/";

TOUCH_CONTROLLER_CONFIGURATION_LEFT = {
    name: "Touch",
    controllers: [
        {
            modelURL: BASE_URL + "touch_l_body.fbx",
            jointIndex: MyAvatar.getJointIndex("_CONTROLLER_LEFTHAND"),
            naturalPosition: { x: 0.01648625358939171, y: -0.03551870584487915, z: -0.018527675420045853 },
            dimensions: { x: 0.11053799837827682, y: 0.0995776429772377, z: 0.10139888525009155 },
            rotation: leftBaseRotation,
            position: leftBasePosition,

            parts: {
                tips: {
                    type: "static",
                    modelURL: BASE_URL + "Oculus-Labels-L.fbx",
                    naturalPosition: { x: -0.022335469722747803, y: 0.00022516027092933655, z: 0.020340695977211 },

                    textureName: "blank",
                    defaultTextureLayer: "blank",
                    textureLayers: {
                        blank: {
                            defaultTextureURL: BASE_URL + "Oculus-Labels-L.fbx/Oculus-Labels-L.fbm/Blank.png"
                        },
                        trigger: {
                            defaultTextureURL: BASE_URL + "Oculus-Labels-L.fbx/Oculus-Labels-L.fbm/Trigger.png"
                        },
                        arrows: {
                            defaultTextureURL: BASE_URL + "Oculus-Labels-L.fbx/Oculus-Labels-L.fbm/Rotate.png"
                        },
                        grip: {
                            defaultTextureURL: BASE_URL + "Oculus-Labels-L.fbx/Oculus-Labels-L.fbm/Grip-oculus.png"
                        },
                        teleport: {
                            defaultTextureURL: BASE_URL + "Oculus-Labels-L.fbx/Oculus-Labels-L.fbm/Teleport.png"
                        },
                    }
                },

                trigger: {
                    type: "rotational",
                    modelURL: BASE_URL + "touch_l_trigger.fbx",
                    naturalPosition: { x: 0.0008544912561774254, y: -0.019867943599820137, z: 0.018800459802150726 },

                    // rotational 
                    input: Controller.Standard.LT,
                    origin: { x: 0, y: -0.015, z: -0.00 },
                    minValue: 0.0,
                    maxValue: 1.0,
                    axis: { x: 1, y: 0, z: 0 },
                    maxAngle: 17,

                    textureName: "tex-highlight",
                    defaultTextureLayer: "normal",
                    textureLayers: {
                        normal: {
                            defaultTextureURL: BASE_URL + "touch_l_trigger.fbx/touch_l_trigger.fbm/L_controller_DIF.jpg",
                        },
                        highlight: {
                            defaultTextureURL: BASE_URL + "touch_l_trigger.fbx/touch_l_trigger.fbm/L_controller-highlight_DIF.jpg",
                        }
                    }
                },

                grip: {
                    type: "linear",
                    modelURL: BASE_URL + "touch_l_bumper.fbx",
                    naturalPosition: { x: 0.00008066371083259583, y: -0.02715788595378399, z: -0.02448512241244316 },

                    // linear properties
                    // Offset from origin = 0.36470, 0.11048, 0.11066
                    input: "OculusTouch.LeftGrip",
                    axis: { x: 1, y: 0.302933918, z: 0.302933918 },
                    maxTranslation: 0.003967,

                    textureName: "tex-highlight",
                    defaultTextureLayer: "normal",
                    textureLayers: {
                        normal: {
                            defaultTextureURL: BASE_URL + "touch_l_bumper.fbx/touch_l_bumper.fbm/L_controller_DIF.jpg",
                        },
                        highlight: {
                            defaultTextureURL: BASE_URL + "touch_l_bumper.fbx/touch_l_bumper.fbm/L_controller-highlight_DIF.jpg",
                        }
                    }
                },

                joystick: {
                    type: "joystick",
                    modelURL: BASE_URL + "touch_l_joystick.fbx",
                    naturalPosition: { x: 0.0075613949447870255, y: -0.008225866593420506, z: 0.004792703315615654 },

                    // joystick
                    xInput: "OculusTouch.LX",
                    yInput: "OculusTouch.LY",
                    originOffset: { x: 0, y: -0.0028564, z: -0.00 },
                    xHalfAngle: 20,
                    yHalfAngle: 20,

                    textureName: "tex-highlight",
                    defaultTextureLayer: "normal",
                    textureLayers: {
                        normal: {
                            defaultTextureURL: BASE_URL + "touch_l_joystick.fbx/touch_l_joystick.fbm/L_controller_DIF.jpg",
                        },
                        highlight: {
                            defaultTextureURL: BASE_URL + "touch_l_joystick.fbx/touch_l_joystick.fbm/L_controller-highlight_DIF.jpg",
                        }
                    }
                },

                button_a: {
                    type: "linear",
                    modelURL: BASE_URL + "touch_l_button_x.fbx",
                    naturalPosition: { x: -0.009307309985160828, y: -0.00005015172064304352, z: -0.012594521045684814 },

                    input: "OculusTouch.X",
                    axis: { x: 0, y: -1, z: 0 },
                    maxTranslation: 0.001,

                    textureName: "tex-highlight",
                    defaultTextureLayer: "normal",
                    textureLayers: {
                        normal: {
                            defaultTextureURL: BASE_URL + "touch_l_button_x.fbx/touch_l_button_x.fbm/L_controller_DIF.jpg",
                        },
                        highlight: {
                            defaultTextureURL: BASE_URL + "touch_l_button_x.fbx/touch_l_button_x.fbm/L_controller-highlight_DIF.jpg",
                        }
                    }
                },

                button_b: {
                    type: "linear",
                    modelURL: BASE_URL + "touch_l_button_y.fbx",
                    naturalPosition: { x: -0.01616849936544895, y: -0.000050364527851343155, z: 0.0017703399062156677 },

                    input: "OculusTouch.Y",
                    axis: { x: 0, y: -1, z: 0 },
                    maxTranslation: 0.001,

                    textureName: "tex-highlight",
                    defaultTextureLayer: "normal",
                    textureLayers: {
                        normal: {
                            defaultTextureURL: BASE_URL + "touch_l_button_y.fbx/touch_l_button_y.fbm/L_controller_DIF.jpg",
                        },
                        highlight: {
                            defaultTextureURL: BASE_URL + "touch_l_button_y.fbx/touch_l_button_y.fbm/L_controller-highlight_DIF.jpg",
                        }
                    }
                },
            }
        }
    ]
};

TOUCH_CONTROLLER_CONFIGURATION_RIGHT = {
    name: "Touch",
    controllers: [
        {
            modelURL: BASE_URL + "touch_r_body.fbx",
            jointIndex: MyAvatar.getJointIndex("_CONTROLLER_RIGHTHAND"),
            naturalPosition: { x: -0.016486231237649918, y: -0.03551865369081497, z: -0.018527653068304062 },
            dimensions: { x: 0.11053784191608429, y: 0.09957750141620636, z: 0.10139875113964081 },
            rotation: rightBaseRotation,
            position: rightBasePosition,

            parts: {
                tips: {
                    type: "static",
                    modelURL: BASE_URL + "Oculus-Labels-R.fbx",
                    naturalPosition: { x: 0.009739525616168976, y: -0.0017818436026573181, z: 0.016794726252555847 },

                    textureName: "Texture",
                    defaultTextureLayer: "blank",
                    textureLayers: {
                        blank: {
                            defaultTextureURL: BASE_URL + "Oculus-Labels-R.fbx/Oculus-Labels-R.fbm/Blank.png"
                        },
                        trigger: {
                            defaultTextureURL: BASE_URL + "Oculus-Labels-R.fbx/Oculus-Labels-R.fbm/Trigger.png"
                        },
                        arrows: {
                            defaultTextureURL: BASE_URL + "Oculus-Labels-R.fbx/Oculus-Labels-R.fbm/Rotate.png"
                        },
                        grip: {
                            defaultTextureURL: BASE_URL + "Oculus-Labels-R.fbx/Oculus-Labels-R.fbm/Grip-oculus.png"
                        },
                        teleport: {
                            defaultTextureURL: BASE_URL + "Oculus-Labels-R.fbx/Oculus-Labels-R.fbm/Teleport.png"
                        },
                    }
                },

                trigger: {
                    type: "rotational",
                    modelURL: BASE_URL + "touch_r_trigger.fbx",
                    naturalPosition: { x: -0.0008544912561774254, y: -0.019867943599820137, z: 0.018800459802150726 },

                    // rotational 
                    input: "OculusTouch.RT",
                    origin: { x: 0, y: -0.015, z: 0 },
                    minValue: 0.0,
                    maxValue: 1.0,
                    axis: { x: 1, y: 0, z: 0 },
                    maxAngle: 17,

                    textureName: "tex-highlight",
                    defaultTextureLayer: "normal",
                    textureLayers: {
                        normal: {
                            defaultTextureURL: BASE_URL + "touch_r_trigger.fbx/touch_r_trigger.fbm/R_controller_DIF.jpg",
                        },
                        highlight: {
                            defaultTextureURL: BASE_URL + "touch_r_trigger.fbx/touch_r_trigger.fbm/R_controller-highlight_DIF.jpg",
                        }
                    }
                },

                grip: {
                    type: "linear",
                    modelURL: BASE_URL + "touch_r_bumper.fbx",
                    naturalPosition: { x: -0.0000806618481874466, y: -0.027157839387655258, z: -0.024485092610120773 },

                    // linear properties
                    // Offset from origin = 0.36470, 0.11048, 0.11066
                    input: "OculusTouch.RightGrip",
                    axis: { x: -1, y: 0.302933918, z: 0.302933918 },
                    maxTranslation: 0.003967,


                    textureName: "tex-highlight",
                    defaultTextureLayer: "normal",
                    textureLayers: {
                        normal: {
                            defaultTextureURL: BASE_URL + "touch_r_bumper.fbx/touch_r_bumper.fbm/R_controller_DIF.jpg",
                        },
                        highlight: {
                            defaultTextureURL: BASE_URL + "touch_r_bumper.fbx/touch_r_bumper.fbm/R_controller-highlight_DIF.jpg",
                        }
                    }
                },

                joystick: {
                    type: "joystick",
                    modelURL: BASE_URL + "touch_r_joystick.fbx",
                    naturalPosition: { x: -0.007561382371932268, y: -0.008225853554904461, z: 0.00479268841445446 },

                    // joystick
                    xInput: "OculusTouch.RX",
                    yInput: "OculusTouch.RY",
                    originOffset: { x: 0, y: -0.0028564, z: 0 },
                    xHalfAngle: 20,
                    yHalfAngle: 20,

                    textureName: "tex-highlight",
                    defaultTextureLayer: "normal",
                    textureLayers: {
                        normal: {
                            defaultTextureURL: BASE_URL + "touch_r_joystick.fbx/touch_r_joystick.fbm/R_controller_DIF.jpg",
                        },
                        highlight: {
                            defaultTextureURL: BASE_URL + "touch_r_joystick.fbx/touch_r_joystick.fbm/R_controller-highlight_DIF.jpg",
                        }
                    }
                },

                button_a: {
                    type: "linear",
                    modelURL: BASE_URL + "touch_r_button_a.fbx",
                    naturalPosition: { x: 0.009307296946644783, y: -0.00005015172064304352, z: -0.012594504281878471 },

                    input: "OculusTouch.A",
                    axis: { x: 0, y: -1, z: 0 },
                    maxTranslation: 0.001,

                    textureName: "tex-highlight",
                    defaultTextureLayer: "normal",
                    textureLayers: {
                        normal: {
                            defaultTextureURL: BASE_URL + "touch_r_button_a.fbx/touch_r_button_a.fbm/R_controller_DIF.jpg",
                        },
                        highlight: {
                            defaultTextureURL: BASE_URL + "touch_r_button_a.fbx/touch_r_button_a.fbm/R_controller-highlight_DIF.jpg",
                        }
                    }
                },

                button_b: {
                    type: "linear",
                    modelURL: BASE_URL + "touch_r_button_b.fbx",
                    naturalPosition: { x: 0.01616847701370716, y: -0.000050364527851343155, z: 0.0017703361809253693 },

                    input: "OculusTouch.B",
                    axis: { x: 0, y: -1, z: 0 },
                    maxTranslation: 0.001,

                    textureName: "tex-highlight",
                    defaultTextureLayer: "normal",
                    textureLayers: {
                        normal: {
                            defaultTextureURL: BASE_URL + "touch_r_button_b.fbx/touch_r_button_b.fbm/R_controller_DIF.jpg",
                        },
                        highlight: {
                            defaultTextureURL: BASE_URL + "touch_r_button_b.fbx/touch_r_button_b.fbm/R_controller-highlight_DIF.jpg",
                        }
                    }
                },
            }
        }
    ]
};
