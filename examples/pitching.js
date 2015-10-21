//var PITCH_THUNK_SOUND_URL = "file:///C:/Users/Ryan/Downloads/323725__reitanna__thunk.wav";
var PITCH_THUNK_SOUND_URL = "file:///C:/Users/Ryan/Downloads/thunk.wav";
var PITCH_THUNK_SOUND_URL = "http://hifi-public.s3.amazonaws.com/sounds/ping_pong_gun/pong_sound.wav";
var pitchSound = SoundCache.getSound(PITCH_THUNK_SOUND_URL, false);

function info(message) {
    print("[INFO] " + message);
}

function error(message) {
    print("[ERROR] " + message);
}

var PITCHING_MACHINE_URL = "atp:87d4879530b698741ecc45f6f31789aac11f7865a2c3bec5fe9b061a182c80d4.fbx";
var PITCHING_MACHINE_OUTPUT_OFFSET_PCT = {
    x: 0.0,
    y: 0.25,
    z: -1.05,
};
var PITCHING_MACHINE_PROPERTIES = {
    name: "Pitching Machine",
    type: "Model",
    position: {
        x: 0,
        y: 0.8,
        z: -22.3,
    },
    velocity: {
        x: 0,
        y: -0.01,
        z: 0
    },
    gravity: {
        x: 0.0,
        y: -9.8,
        z: 0.0
    },
    registrationPoint: {
        x: 0.5,
        y: 0.5,
        z: 0.5,
    },
    rotation: Quat.fromPitchYawRollDegrees(0, 180, 0),
    modelURL: PITCHING_MACHINE_URL,
    dimensions: {
        x: 0.4,
        y: 0.61,
        z: 0.39
    },
    collisionsWillMove: false,
    shapeType: "Box",
};
PITCHING_MACHINE_PROPERTIES.dimensions = Vec3.multiply(2.5, PITCHING_MACHINE_PROPERTIES.dimensions);


var PITCH_RATE = 5000;

var BASEBALL_MODEL_URL = "atp:7185099f1f650600ca187222573a88200aeb835454bd2f578f12c7fb4fd190fa.fbx";
var BASEBALL_SPEED = 2.7;
var BASEBALL_RADIUS = 0.07468;
var BASEBALL_PROPERTIES = {
    name: "Baseball",
    type: "Model",
    modelURL: BASEBALL_MODEL_URL,
    shapeType: "Sphere",
    position: {
        x: 0,
        y: 0,
        z: 0
    },
    dimensions: {
        x: BASEBALL_RADIUS,
        y: BASEBALL_RADIUS,
        z: BASEBALL_RADIUS
    },
    collisionsWillMove: true,
    angularVelocity: {
        x: 17.0,
        y: 0,
        z: -8.0,
    },
    angularDamping: 0.0,
    damping: 0.0,
    restitution: 0.5,
    friction: 0.0,
    lifetime: 20,
    collisionSoundURL: PITCH_THUNK_SOUND_URL,
    gravity: {
        x: 0,
        y: 0,//-9.8,
        z: 0
    }
};


var pitchingMachineID = Entities.addEntity(PITCHING_MACHINE_PROPERTIES);

var pitchFromPosition = { x: 0, y: 1.0, z: 0 };
var pitchDirection = { x: 0, y: 0, z: 1 };

function shallowCopy(obj) {
    var copy = {}
    for (var key in obj) {
        copy[key] = obj[key];
    }
    return copy;
}

function randomInt(low, high) {
    return low + (Math.random() * (high - low));
}

var ACCELERATION_SPREAD = 10.15;

function createBaseball(position, velocity, ballScale) {
    var properties = shallowCopy(BASEBALL_PROPERTIES);
    properties.position = position;
    properties.velocity = velocity;
    properties.acceleration = {
        x: randomInt(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
        y: randomInt(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
        z: 0.0,
    };
    properties.dimensions = Vec3.multiply(ballScale, properties.dimensions);
    var entityID = Entities.addEntity(properties);
    Script.addEventHandler(entityID, "collisionWithEntity", buildBaseballHitCallback(entityID));
    if (false && Math.random() < 0.5) {
        for (var i = 0; i < 50; i++) {
            Script.setTimeout(function() {
                Entities.editEntity(entityID, {
                    gravity: {
                        x: randomInt(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
                        y: randomInt(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
                        z: 0.0,
                    }
                })
            }, i * 100);
        }
    }
    return entityID;
}

var buildBaseballHitCallback = function(entityID) {
    var f = function(entityA, entityB, collision) {
        var properties = Entities.getEntityProperties(entityID, ['position', 'velocity']);
        var line = new InfiniteLine(properties.position, { red: 255, green: 128, blue: 89 });
        var lastPosition = properties.position;
        Vec3.print("Velocity", properties.velocity);
        Vec3.print("VelocityChange", collision.velocityChange);
        Script.setInterval(function() {
            var properties = Entities.getEntityProperties(entityID, ['position']);
            if (Vec3.distance(properties.position, lastPosition)) {
                line.enqueuePoint(properties.position);
                lastPosition = properties.position;
            }
        }, 50);
        Entities.editEntity(entityID, {
            velocity: Vec3.multiply(3, properties.velocity),
            gravity: {
                x: 0,
                y: -2.8,
                z: 0
            }
        });
        print("Baseball hit!");
        Script.removeEventHandler(entityID, "collisionWithEntity", f);
    };
    return f;
}


function vec3Mult(a, b) {
    return {
        x: a.x * b.x,
        y: a.y * b.y,
        z: a.z * b.z,
    };
}

var injector = null;
function pitchBall() {
    var machineProperties = Entities.getEntityProperties(pitchingMachineID, ["dimensions", "position", "rotation"]);
    var pitchFromPositionBase = machineProperties.position;
    var pitchFromOffset = vec3Mult(machineProperties.dimensions, PITCHING_MACHINE_OUTPUT_OFFSET_PCT);
    pitchFromOffset = Vec3.multiplyQbyV(machineProperties.rotation, pitchFromOffset);
    var pitchFromPosition = Vec3.sum(pitchFromPositionBase, pitchFromOffset);
    var pitchDirection = Quat.getFront(machineProperties.rotation);
    var ballScale = machineProperties.dimensions.x / PITCHING_MACHINE_PROPERTIES.dimensions.x;
    print("Creating baseball");
    var ballEntityID = createBaseball(pitchFromPosition, Vec3.multiply(BASEBALL_SPEED, pitchDirection), ballScale);
    if (!injector) {
        injector = Audio.playSound(pitchSound, {
            position: pitchFromPosition,
            volume: 1.0
        });
    } else {
        injector.restart();
    }
}

Script.scriptEnding.connect(function() {
    Entities.deleteEntity(pitchingMachineID);
})

Script.setInterval(pitchBall, PITCH_RATE);




/******************************************************************************
 * PolyLine 
 *****************************************************************************/
var LINE_DIMENSIONS = { x: 2000, y: 2000, z: 2000 };
var MAX_LINE_LENGTH = 40; // This must be 2 or greater;
var PolyLine = function(position, color, defaultStrokeWidth) {
    //info("Creating polyline");
    //Vec3.print("New line at", position);
    this.position = position;
    this.color = color;
    this.defaultStrokeWidth = 0.10;
    this.points = [
        { x: 0, y: 0, z: 0 },
    ];
    this.strokeWidths = [
        this.defaultStrokeWidth,
    ]
    this.normals = [
        { x: 1, y: 0, z: 0 },
    ]
    this.entityID = Entities.addEntity({
        type: "PolyLine",
        position: position,
        linePoints: this.points,
        normals: this.normals,
        strokeWidths: this.strokeWidths,
        dimensions: LINE_DIMENSIONS,
        color: color,
        lifetime: 20,
    });
};

PolyLine.prototype.enqueuePoint = function(position) {
    if (this.isFull()) {
        error("Hit max PolyLine size");
        return;
    }

    //Vec3.print("pos", position);
    //info("Number of points: " + this.points.length);

    position = Vec3.subtract(position, this.position);
    this.points.push(position);
    this.normals.push({ x: 1, y: 0, z: 0 });
    this.strokeWidths.push(this.defaultStrokeWidth);
    Entities.editEntity(this.entityID, {
        linePoints: this.points,
        normals: this.normals,
        strokeWidths: this.strokeWidths,
    });
};

PolyLine.prototype.dequeuePoint = function() {
    if (this.points.length == 0) {
        error("Hit min PolyLine size");
        return;
    }

    this.points = this.points.slice(1);
    this.normals = this.normals.slice(1);
    this.strokeWidths = this.strokeWidths.slice(1);

    Entities.editEntity(this.entityID, {
        linePoints: this.points,
        normals: this.normals,
        strokeWidths: this.strokeWidths,
    });
};

PolyLine.prototype.getFirstPoint = function() {
    return Vec3.sum(this.position, this.points[0]);
};

PolyLine.prototype.getLastPoint = function() {
    return Vec3.sum(this.position, this.points[this.points.length - 1]);
};

PolyLine.prototype.getSize = function() {
    return this.points.length;
}

PolyLine.prototype.isFull = function() {
    return this.points.length >= MAX_LINE_LENGTH;
};

PolyLine.prototype.destroy = function() {
    Entities.deleteEntity(this.entityID);
    this.points = [];
};


/******************************************************************************
 * InfiniteLine
 *****************************************************************************/
InfiniteLine = function(position, color) {
    this.position = position;
    this.color = color;
    this.lines = [new PolyLine(position, color)];
    this.size = 0;
};

InfiniteLine.prototype.enqueuePoint = function(position) {
    var currentLine;

    if (this.lines.length == 0) {
        currentLine = new PolyLine(position, this.color);
        this.lines.push(currentLine);
    } else {
        currentLine = this.lines[this.lines.length - 1];
    }

    if (currentLine.isFull()) {
        //info("Current line is full, creating new line");
        //Vec3.print("Last line is", currentLine.getLastPoint());
        //Vec3.print("New line is", position);
        var newLine = new PolyLine(currentLine.getLastPoint(), this.color);
        this.lines.push(newLine);
        currentLine = newLine;
    }

    currentLine.enqueuePoint(position);

    ++this.size;
};

InfiniteLine.prototype.dequeuePoint = function() {
    if (this.lines.length == 0) {
        error("Trying to dequeue from InfiniteLine when no points are left");
        return;
    }

    var lastLine = this.lines[0];
    lastLine.dequeuePoint();

    if (lastLine.getSize() <= 1) {
        this.lines = this.lines.slice(1);
    }

    --this.size;
};

InfiniteLine.prototype.getFirstPoint = function() {
    return this.lines.length > 0 ? this.lines[0].getFirstPoint() : null;
};

InfiniteLine.prototype.getLastPoint = function() {
    return this.lines.length > 0 ? this.lines[lines.length - 1].getLastPoint() : null;
};

InfiniteLine.prototype.destroy = function() {
    for (var i = 0; i < this.lines.length; ++i) {
        this.lines[i].destroy();
    }

    this.size = 0;
};
