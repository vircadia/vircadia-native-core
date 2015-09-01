//
//  Created by Bradley Austin Davis on 2015/08/27
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
STICK_URL = HIFI_PUBLIC_BUCKET + "models/props/geo_stick.fbx";

// FIXME make this editable through some script UI, so the user can customize the size of the structure built
SCALE = 0.5;
BALL_SIZE = 0.08 * SCALE;
STICK_LENGTH = 0.24 * SCALE;

DEBUG_MAGSTICKS = true;

CUSTOM_DATA_NAME = "magBalls";
BALL_NAME = "MagBall";
EDGE_NAME = "MagStick";

ZERO_VECTOR = { x: 0, y: 0, z: 0 };

COLORS = {
    WHITE: {
        red: 255,
        green: 255,
        blue: 255,
    },
    BLACK: {
        red: 0,
        green: 0,
        blue: 0,
    },
    GREY: {
        red: 128,
        green: 128,
        blue: 128,
    },
    RED: {
        red: 255,
        green: 0,
        blue: 0
    },
    BLUE: {
        red: 0,
        green: 0,
        blue: 255
    },
    GREEN: {
        red: 0,
        green: 255,
        blue: 0
    },
    CYAN: {
        red: 0,
        green: 255,
        blue: 255
    },
    YELLOW: {
        red: 255,
        green: 255,
        blue: 0
    },
    MAGENTA: {
        red: 255,
        green: 0,
        blue: 255
    }
}

BALL_RADIUS = BALL_SIZE / 2.0;

BALL_SELECTION_RADIUS = BALL_RADIUS * 1.5;

BALL_DIMENSIONS = {
    x: BALL_SIZE,
    y: BALL_SIZE,
    z: BALL_SIZE
};

BALL_COLOR = {
    red: 128,
    green: 128,
    blue: 128
};

STICK_DIMENSIONS = {
    x: STICK_LENGTH / 6,
    y: STICK_LENGTH / 6,
    z: STICK_LENGTH
};

BALL_DISTANCE = STICK_LENGTH + BALL_SIZE;

BALL_PROTOTYPE = {
    type: "Sphere",
    name: BALL_NAME,
    dimensions: BALL_DIMENSIONS,
    color: BALL_COLOR,
    ignoreCollisions: true,
    collisionsWillMove: false
};

// 2 millimeters
BALL_EPSILON = (.002) / BALL_DISTANCE;

LINE_DIMENSIONS = {
    x: 5,
    y: 5,
    z: 5
}

LINE_PROTOTYPE = {
    type: "Line",
    name: EDGE_NAME,
    color: COLORS.CYAN,
    dimensions: LINE_DIMENSIONS,
    lineWidth: 5,
    visible: true,
    ignoreCollisions: true,
    collisionsWillMove: false,
}

EDGE_PROTOTYPE = LINE_PROTOTYPE;

// var EDGE_PROTOTYPE = {
// type: "Sphere",
// name: EDGE_NAME,
// color: { red: 0, green: 255, blue: 255 },
// //dimensions: STICK_DIMENSIONS,
// dimensions: { x: 0.02, y: 0.02, z: 0.02 },
// rotation: rotation,
// visible: true,
// ignoreCollisions: true,
// collisionsWillMove: false
// }


