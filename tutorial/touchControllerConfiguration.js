var CONTROLLER_LENGTH_OFFSET = 0.0762; 
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

