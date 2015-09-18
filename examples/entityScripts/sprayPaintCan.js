(function() {
    // Script.include("../libraries/utils.js");
    //Need absolute path for now, for testing before PR merge and s3 cloning. Will change post-merge

    Script.include("../libraries/utils.js");
    GRAB_FRAME_USER_DATA_KEY = "grabFrame";
    this.userData = {};

    var TIP_OFFSET_Z = 0.14;
    var TIP_OFFSET_Y = 0.04;

    var ZERO_VEC = {
        x: 0,
        y: 0,
        z: 0
    }

    var MAX_POINTS_PER_LINE = 40;
    var MIN_POINT_DISTANCE = 0.01;
    var STROKE_WIDTH = 0.02;

    var self = this;

    var timeSinceLastMoved = 0;
    var RESET_TIME_THRESHOLD = 5;
    var DISTANCE_FROM_HOME_THRESHOLD = 0.5;
    var HOME_POSITION = {
        x: 549.12,
        y: 495.555,
        z: 503.77
    };
    this.getUserData = function() {


        if (this.properties.userData) {
            this.userData = JSON.parse(this.properties.userData);
        }
    }

    this.updateUserData = function() {
        Entities.editEntity(this.entityId, {
            userData: JSON.stringify(this.userData)
        });
    }

    this.update = function(deltaTime) {
        self.getUserData();
        self.properties = Entities.getEntityProperties(self.entityId);

        if (Vec3.length(self.properties.velocity) < 0.1 && Vec3.distance(HOME_POSITION, self.properties.position) > DISTANCE_FROM_HOME_THRESHOLD) {
            timeSinceLastMoved += deltaTime;
            if (timeSinceLastMoved > RESET_TIME_THRESHOLD) {
                self.reset();
                timeSinceLastMoved = 0;
            }
        } else {
            timeSinceLastMoved = 0;
        }

        //Only activate for the user who grabbed the object
        if (self.userData.grabKey && self.userData.grabKey.activated === true && self.userData.grabKey.avatarId == MyAvatar.sessionUUID) {
            if (self.activated !== true) {
                //We were just grabbed, so create a particle system 
                self.grab();
            }
            //Move emitter to where entity is always when its activated
            self.sprayStream();
        } else if (self.userData.grabKey && self.userData.grabKey.activated === false && self.activated) {
            self.letGo();
        }
    }

    this.grab = function() {
        this.activated = true;
        var animationSettings = JSON.stringify({
            fps: 30,
            loop: true,
            firstFrame: 1,
            lastFrame: 10000,
            running: true
        });
        var PI = 3.141593;
        var DEG_TO_RAD = PI / 180.0;

        this.paintStream = Entities.addEntity({
            type: "ParticleEffect",
            animationSettings: animationSettings,
            position: this.properties.position,
            textures: "https://raw.githubusercontent.com/ericrius1/SantasLair/santa/assets/smokeparticle.png",
            emitSpeed: 0,
            speedSpread: 0.02,
            polarFinish: 2 * DEG_TO_RAD,
            emitAcceleration: ZERO_VEC,
            emitRate: 100,
            particleRadius: 0.01,
            color: {
                red: 170,
                green: 20,
                blue: 150
            },
            lifetime: 50, //probably wont be holding longer than this straight
        });
    }

    this.letGo = function() {
        this.activated = false;
        Entities.deleteEntity(this.paintStream);
        this.paintStream = null;
    }

    this.reset = function() {
        Entities.editEntity(self.entityId, {
            position: HOME_POSITION,
            rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
            angularVelocity: ZERO_VEC,
            velocity: ZERO_VEC
        });
    }

    this.sprayStream = function() {
        var forwardVec = Quat.getFront(Quat.multiply(self.properties.rotation , Quat.fromPitchYawRollDegrees(0, 90, 0)));
        forwardVec = Vec3.normalize(forwardVec);

        var upVec = Quat.getUp(self.properties.rotation);
        var position = Vec3.sum(self.properties.position, Vec3.multiply(forwardVec, TIP_OFFSET_Z));
        position = Vec3.sum(position, Vec3.multiply(upVec, TIP_OFFSET_Y))
        Entities.editEntity(self.paintStream, {
            position: position,
            emitOrientation: forwardVec,
            emitSpeed: 5
        });

        //Now check for an intersection with an entity
        //move forward so ray doesnt intersect with gun
        var origin = Vec3.sum(position, forwardVec);
        var pickRay = {
            origin: origin,
            direction: Vec3.multiply(forwardVec, 2)
        }

        var intersection = Entities.findRayIntersection(pickRay, true);
        if (intersection.intersects) {
            var normal = Vec3.multiply(-1, Quat.getFront(intersection.properties.rotation));
            this.paint(intersection.intersection, normal);
        }


    }

    this.paint = function(position, normal) {
        if (!this.painting) {

            this.newStroke(position);
            this.painting = true;
        }

        if (this.strokePoints.length > MAX_POINTS_PER_LINE) {
            this.painting = false;
            return;
        }

        var localPoint = Vec3.subtract(position, this.strokeBasePosition);
        //Move stroke a bit forward along normal so it doesnt zfight with mesh its drawing on 
        localPoint = Vec3.sum(localPoint, Vec3.multiply(normal, .1));

        if (this.strokePoints.length > 0 && Vec3.distance(localPoint, this.strokePoints[this.strokePoints.length - 1]) < MIN_POINT_DISTANCE) {
            //need a minimum distance to avoid binormal NANs
            return;
        }

        this.strokePoints.push(localPoint);
        this.strokeNormals.push(normal);
        this.strokeWidths.push(STROKE_WIDTH);
        Entities.editEntity(this.currentStroke, {
            linePoints: this.strokePoints,
            normals: this.strokeNormals,
            strokeWidths: this.strokeWidths
        });


    }

    this.newStroke = function(position) {
        this.strokeBasePosition = position;
        this.currentStroke = Entities.addEntity({
            position: position,
            type: "PolyLine",
            color: {
                red: randInt(160, 250),
                green: randInt(10, 20),
                blue: randInt(190, 250)
            },
            dimensions: {
                x: 50,
                y: 50,
                z: 50
            },
            lifetime: 100
        });
        this.strokePoints = [];
        this.strokeNormals = [];
        this.strokeWidths = [];

        this.strokes.push(this.currentStroke);
    }

    this.preload = function(entityId) {
        this.strokes = [];
        this.activated = false;
        this.entityId = entityId;
        this.properties = Entities.getEntityProperties(self.entityId);
        this.getUserData();

        //Only activate for the avatar who is grabbing the can!
        if (this.userData.grabKey && this.userData.grabKey.activated) {
            this.activated = true;
        }
        if (!this.userData.grabFrame) {
            var data = {
                relativePosition: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                relativeRotation: Quat.fromPitchYawRollDegrees(0, 0, 0)
            }
            setEntityCustomData(GRAB_FRAME_USER_DATA_KEY, this.entityId, data);
        }
    }


    this.unload = function() {
        Script.update.disconnect(this.update);
        if(this.paintStream) {
            Entities.deleteEntity(this.paintStream);
        }
        this.strokes.forEach(function(stroke) {
            Entities.deleteEntity(stroke);
        });
    }
    Script.update.connect(this.update);
});



function randFloat(min, max) {
    return Math.random() * (max - min) + min;
}

function randInt(min, max) {
    return Math.floor(Math.random() * (max - min)) + min;
}