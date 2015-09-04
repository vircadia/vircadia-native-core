
// FIXME make this editable through some script UI, so the user can customize the size of the structure built
SCALE = 0.5;
BALL_SIZE = 0.08 * SCALE;
STICK_LENGTH = 0.24 * SCALE;

DEBUG_MAGSTICKS = true;

BALL_NAME = "MagBall";
EDGE_NAME = "MagStick";

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
