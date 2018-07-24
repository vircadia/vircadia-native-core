// JavaScript source code

/* Vec3, MyAvatar, Camera, Quat, Mat4, Script        */


var controllerMappingName;
var controllerMapping;

controllerMappingName = 'Hifi-AnimationTools-Mapping';
controllerMapping = Controller.newMapping(controllerMappingName);

// Camera.mode = "independent";
Camera.orientation = Quat.lookAtSimple(Camera.position, Vec3.ZERO);
var moveUp = false;
var moveDown = false;
var moveRight = false;
var moveLeft = false;
var moveForward = false;
var moveBack = false;
var rotateRight = false;
var rotateLeft = false;
var rotateUp = false;
var rotateDown = false;
var rollLeft = false;
var rollRight = false;
var orbitLeft = false;
var orbitRight = false;
var orbitUp = false;
var orbitDown = false;

controllerMapping.from(Controller.Hardware.Keyboard.I).to(function (value) {
    if (value !== 0) {
        Camera.mode = "third person";
        print("camera mode is independent");
    }
});


controllerMapping.from(Controller.Hardware.Keyboard.T).to(function (value) {
    if (value !== 0) {
        orbitUp = true;
        print("orbit the camera up");
    } else {
        orbitUp = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.G).to(function (value) {
    if (value !== 0) {
        orbitDown = true;
        print("orbit the camera down");
    } else {
        orbitDown = false;
    }
});


controllerMapping.from(Controller.Hardware.Keyboard.V).to(function (value) {
    if (value !== 0) {
        orbitLeft = true;
        print("orbit the camera left");
    } else {
        orbitLeft = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.B).to(function (value) {
    if (value !== 0) {
        orbitRight = true;
        print("orbit the camera right");
    } else {
        orbitRight = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.F).to(function (value) {
    if (value !== 0) {
        rollRight = true;
        print("roll the camera right");
    } else {
        rollRight = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.R).to(function (value) {
    if (value !== 0) {
        rollLeft = true;
        print("roll the camera left");
    } else {
        rollLeft = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.D).to(function (value) {
    if (value !== 0) {
        rotateRight = true;
        print("rotate the camera right");
    } else {
        rotateRight = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.E).to(function (value) {
    if (value !== 0) {
        rotateLeft = true;
        print("rotate the camera left");
    } else {
        rotateLeft = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.W).to(function (value) {
    if (value !== 0) {
        rotateUp = true;
        print("rotate the camera up");
    } else {
        rotateUp = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.S).to(function (value) {
    if (value !== 0) {
        rotateDown = true;
        print("rotate the camera down");
    } else {
        rotateDown = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.U).to(function (value) {
    if (value !== 0) {
        moveForward = true;
        print("move the camera forward");
    } else {
        moveForward = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.J).to(function (value) {
    if (value !== 0) {
        moveRight = true;
        print("move the camera right");
    } else {
        moveRight = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.H).to(function (value) {
    if (value !== 0) {
        moveLeft = true;
        print("move the camera left");
    } else {
        moveLeft = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.N).to(function (value) {
    if (value !== 0) {
        moveBack = true;
        print("move the camera back");
    } else {
        moveBack = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.O).to(function (value) {
    if (value !== 0) {
        moveUp = true;
        print("move the camera up");
    } else {
        moveUp = false;
    }
});

controllerMapping.from(Controller.Hardware.Keyboard.L).to(function (value) {
    if (value !== 0) {
        moveDown = true;
        print("move the camera down");
    } else {
        moveDown = false;
    }
});

Controller.enableMapping(controllerMappingName);
Camera.mode = "first person";

Script.update.connect(function (deltaTime) {

    if (Camera.mode === "third person") {
        print("the camera position is now " + Camera.position.x + " " + Camera.position.y + " " + Camera.position.z);

        var METERS_PER_SECOND = 1.0;
        var DEGREES_PER_SECOND = 20.0;
        var metersTraveledThisFrame = deltaTime * METERS_PER_SECOND;
        var degreesTraveledThisFrame = deltaTime * DEGREES_PER_SECOND;
        var newPosition = { x: 0, y: 0, z: 0 };
        var newOrientation = { x: 0, y: 0, z: 0, w: 1 };
        var distanceToMyAvatar = Vec3.length(Vec3.subtract(Camera.position, MyAvatar.position));

        if (orbitLeft) {
            Camera.position = MyAvatar.position;
            newOrientation = Quat.angleAxis(degreesTraveledThisFrame, { x: 0, y: -1, z: 0 });
            Camera.orientation = Quat.multiply(newOrientation, Camera.orientation);
            newPosition = Vec3.multiply(distanceToMyAvatar, Vec3.multiplyQbyV(Camera.orientation, { x: 0, y: 0, z: 1 }));
            Camera.position = Vec3.sum(Camera.position, newPosition);
        }
        if (orbitRight) {
            Camera.position = MyAvatar.position;
            newOrientation = Quat.angleAxis(degreesTraveledThisFrame, { x: 0, y: 1, z: 0 });
            Camera.orientation = Quat.multiply(newOrientation, Camera.orientation);
            newPosition = Vec3.multiply(distanceToMyAvatar, Vec3.multiplyQbyV(Camera.orientation, { x: 0, y: 0, z: 1 }));
            Camera.position = Vec3.sum(Camera.position, newPosition);
        }

        if (orbitUp) {
            Camera.position = MyAvatar.position;
            newOrientation = Quat.angleAxis(degreesTraveledThisFrame, { x: -1, y: 0, z: 0 });
            Camera.orientation = Quat.multiply(Camera.orientation, newOrientation);
            newPosition = Vec3.multiply(distanceToMyAvatar, Vec3.multiplyQbyV(Camera.orientation, { x: 0, y: 0, z: 1 }));
            Camera.position = Vec3.sum(Camera.position, newPosition);
        }
        if (orbitDown) {
            Camera.position = MyAvatar.position;
            newOrientation = Quat.angleAxis(degreesTraveledThisFrame, { x: 1, y: 0, z: 0 });
            Camera.orientation = Quat.multiply(Camera.orientation, newOrientation);
            newPosition = Vec3.multiply(distanceToMyAvatar, Vec3.multiplyQbyV(Camera.orientation, { x: 0, y: 0, z: 1 }));
            Camera.position = Vec3.sum(Camera.position, newPosition);
        }

        if (rotateLeft) {
            newOrientation = Quat.angleAxis(degreesTraveledThisFrame, { x: 0, y: 1, z: 0 });
            Camera.orientation = Quat.multiply(Camera.orientation, newOrientation);
        }
        if (rotateRight) {
            newOrientation = Quat.angleAxis(degreesTraveledThisFrame, { x: 0, y: -1, z: 0 });
            Camera.orientation = Quat.multiply(Camera.orientation, newOrientation);
        }
        if (rollLeft) {
            newOrientation = Quat.angleAxis(degreesTraveledThisFrame, { x: 0, y: 0, z: 1 });
            Camera.orientation = Quat.multiply(Camera.orientation, newOrientation);
        }
        if (rollRight) {
            newOrientation = Quat.angleAxis(degreesTraveledThisFrame, { x: 0, y: 0, z: -1 });
            Camera.orientation = Quat.multiply(Camera.orientation, newOrientation);
        }

        if (rotateUp) {
            newOrientation = Quat.angleAxis(degreesTraveledThisFrame, { x: 1, y: 0, z: 0 });
            Camera.orientation = Quat.multiply(Camera.orientation, newOrientation);
        }
        if (rotateDown) {
            newOrientation = Quat.angleAxis(degreesTraveledThisFrame, { x: -1, y: 0, z: 0 });
            Camera.orientation = Quat.multiply(Camera.orientation, newOrientation);
        }
        if (moveForward) {
            newPosition = Vec3.multiply(metersTraveledThisFrame, Vec3.multiplyQbyV(Camera.orientation, { x: 0, y: 0, z: -1 }));
            Camera.position = Vec3.sum(Camera.position, newPosition);
        }
        if (moveBack) {
            newPosition = Vec3.multiply(metersTraveledThisFrame, Vec3.multiplyQbyV(Camera.orientation, { x: 0, y: 0, z: 1 }));
            Camera.position = Vec3.sum(Camera.position, newPosition);
        }
        if (moveLeft) {
            newPosition = Vec3.multiply(metersTraveledThisFrame, Vec3.multiplyQbyV(Camera.orientation, { x: -1, y: 0, z: 0 }));
            Camera.position = Vec3.sum(Camera.position, newPosition);
        }
        if (moveRight) {
            newPosition = Vec3.multiply(metersTraveledThisFrame, Vec3.multiplyQbyV(Camera.orientation, { x: 1, y: 0, z: 0 }));
            Camera.position = Vec3.sum(Camera.position, newPosition);
        }
        if (moveUp) {
            newPosition = Vec3.multiply(metersTraveledThisFrame, Vec3.multiplyQbyV(Camera.orientation, { x: 0, y: 1, z: 0 }));
            Camera.position = Vec3.sum(Camera.position, newPosition);
        }
        if (moveDown) {
            newPosition = Vec3.multiply(metersTraveledThisFrame, Vec3.multiplyQbyV(Camera.orientation, { x: 0, y: -1, z: 0 }));
            Camera.position = Vec3.sum(Camera.position, newPosition);
        }
    }
});

Script.scriptEnding.connect(function () {
    Controller.disableMapping(controllerMappingName);
});// JavaScript source code
