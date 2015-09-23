(function() {
    // Script.include("../libraries/utils.js");
    //Need absolute path for now, for testing before PR merge and s3 cloning. Will change post-merge

    Script.include("../libraries/utils.js");
    this.userData = {};

    this.spraySound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/sprayPaintSound.wav");

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

    this.setRightHand = function() {
        this.hand = 'RIGHT';
    }

    this.setLeftHand = function() {
        this.hand = 'LEFT';
    }

    this.startNearGrab = function() {
        this.whichHand = this.hand;
        var position = Entities.getEntityProperties(this.entityId, "position").position;
        var animationSettings = JSON.stringify({
            fps: 30,
            loop: true,
            firstFrame: 1,
            lastFrame: 10000,
            running: true
        });

        this.paintStream = Entities.addEntity({
            type: "ParticleEffect",
            animationSettings: animationSettings,
            position: position,
            textures: "https://raw.githubusercontent.com/ericrius1/SantasLair/santa/assets/smokeparticle.png",
            emitVelocity: ZERO_VEC,
            emitAcceleration: ZERO_VEC,
            velocitySpread: {
                x: .1,
                y: .1,
                z: 0.1
            },
            emitRate: 100,
            particleRadius: 0.01,
            color: {
                red: 170,
                green: 20,
                blue: 150
            },
            lifetime: 50, //probably wont be holding longer than this straight
        });

        this.sprayInjector = Audio.playSound(this.spraySound, {
            position: position,
            volume: 0.1
        });
    }


    this.releaseGrab = function() {
        Entities.deleteEntity(this.paintStream);
        this.paintStream = null;
        this.painting = false;
        this.sprayInjector.stop();
    }


    this.continueNearGrab = function() {
        var props = Entities.getEntityProperties(this.entityId, ["position, rotation"]);
        var forwardVec = Quat.getFront(Quat.multiply(props.rotation, Quat.fromPitchYawRollDegrees(0, 90, 0)));
        forwardVec = Vec3.normalize(forwardVec);

        var upVec = Quat.getUp(props.rotation);
        var position = Vec3.sum(props.position, Vec3.multiply(forwardVec, TIP_OFFSET_Z));
        position = Vec3.sum(position, Vec3.multiply(upVec, TIP_OFFSET_Y))
        Entities.editEntity(this.paintStream, {
            position: position,
            emitVelocity: Vec3.multiply(5, forwardVec)
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
        this.entityId = entityId;
    }


    this.unload = function() {
        if (this.paintStream) {
            Entities.deleteEntity(this.paintStream);
        }
        this.strokes.forEach(function(stroke) {
            Entities.deleteEntity(stroke);
        });
    }
});



function randFloat(min, max) {
    return Math.random() * (max - min) + min;
}

function randInt(min, max) {
    return Math.floor(Math.random() * (max - min)) + min;
}