  var ball, ballSpawningAnchor, ballDetector, tiltMaze;

  var MAZE_MODEL_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/tiltMaze/MAZE2.fbx";
  var MAZE_COLLISION_HULL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/tiltMaze/MAZE_COLLISION_HULL3.obj";
  var BALL_DETECTOR_SCRIPT = Script.resolvePath('ballDetector.js?' + Math.random())


  var MAZE_DIMENSIONS = {
    x: 1,
    y: 0.3,
    z: 1
  };

  var BALL_DIMENSIONS = {
    x: 0.05,
    y: 0.05,
    z: 0.05
  }

  var BALL_DETECTOR_DIMENSIONS = {
    x: 1,
    y: 0.15,
    z: 1,
  }

  var BALL_COLOR = {
    red: 255,
    green: 0,
    blue: 0
  }

  var DEBUG_COLOR = {
    red: 0,
    green: 255,
    blue: 0
  }

  var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
  }), Vec3.multiply(1.5, Quat.getFront(Camera.getOrientation())));

  var CLEANUP = true;

  var BALL_FORWARD_OFFSET = -0.2;
  var BALL_RIGHT_OFFSET = -0.4;
  var BALL_VERTICAL_OFFSET = 0.02;


  var BALL_FRICTION = 0.7;
  var BALL_RESTITUTION = 0.1;
  var BALL_DAMPING = 0.6;
  var BALL_ANGULAR_DAMPING = 0.2;
  var BALL_DENSITY = 1000;
  var BALL_GRAVITY = {
    x: 0,
    y: -9.8,
    z: 0
  };


  var MAZE_DENSITY = 1000;
  var MAZE_RESTITUTION = 0.1;
  var MAZE_DAMPING = 0.6;
  var MAZE_ANGULAR_DAMPING = 0.6;

  var DETECTOR_VERTICAL_OFFSET = MAZE_DIMENSIONS.y / 2;

  var getBallStartLocation = function() {
    var mazeProps = Entities.getEntityProperties(tiltMaze);
    var right = Quat.getRight(mazeProps.rotation);
    var front = Quat.getFront(mazeProps.rotation);
    var vertical = {
      x: 0,
      y: BALL_VERTICAL_OFFSET,
      z: 0
    };

    var finalOffset = Vec3.sum(vertical, Vec3.multiply(right, BALL_RIGHT_OFFSET));
    finalOffset = Vec3.sum(finalOffset, Vec3.multiply(front, BALL_FORWARD_OFFSET));
    var location = Vec3.sum(mazeProps.position, finalOffset);
    print('BALL START LOCATOIN:: ' + JSON.stringify(location))
    return location;
  }

  var createBall = function(position) {

    print('making ball')
    var properties = {
      name: 'Hifi Tilt Maze Ball',
      type: 'Sphere',
      position: getBallStartLocation(),
      dynamic: true,
      collisionless: false,
      friction: BALL_FRICTION,
      restitution: BALL_RESTITUTION,
      angularDamping: BALL_ANGULAR_DAMPING,
      damping: BALL_DAMPING,
      gravity: BALL_GRAVITY,
      density: BALL_DENSITY,
      color: BALL_COLOR,
      dimensions:BALL_DIMENSIONS

    };

    ball = Entities.addEntity(properties);

  };

  var createBallSpawningAnchor = function() {
    var properties = {
      name: 'Hifi Tilt Maze Ball Detector',
      parentID: tiltMaze,
      type: 'Box',
      color: DEBUG_COLOR,
      dimensions: BALL_DETECTOR_DIMENSIONS,
      position: getBallStartLocation(),
      collisionless: true,
      visible: true,
      script: BALL_DETECTOR_SCRIPT
    };

    ballSpawningAnchor = Entities.addEntity(properties);
  }

  var createBallDetector = function(position, rotation) {
    var properties = {
      name: 'Hifi Tilt Maze Ball Detector',
      parentID: tiltMaze,
      type: 'Box',
      color: DEBUG_COLOR,
      dimensions: BALL_DETECTOR_DIMENSIONS,
      position: position,
      rotiation: rotation,
      collisionless: true,
      visible: true,
      script: BALL_DETECTOR_SCRIPT
    };

    ballDetector = Entities.addEntity(properties);

  };

  var createTiltMaze = function(position) {
    var properties = {
      name: 'Hifi Tilt Maze',
      type: 'Model',
      modelURL: MAZE_MODEL_URL,
      compoundShapeURL: MAZE_COLLISION_HULL,
      dimensions: MAZE_DIMENSIONS,
      position: position,
      restitution: MAZE_RESTITUTION,
      damping:MAZE_DAMPING,
      angularDamping:MAZE_ANGULAR_DAMPING,
      rotation: Quat.fromPitchYawRollDegrees(0, 0, 180),
      dynamic: true,
      density: MAZE_DENSITY
    }

    tiltMaze = Entities.addEntity(properties)

  }

  var createAll = function() {
    createTiltMaze(center);
    //  createBallSpawningAnchor();
    // createBallDetector();
    createBall(center);
  }

  createAll();

  if (CLEANUP === true) {
    Script.scriptEnding.connect(function() {
      Entities.deleteEntity(tiltMaze);
      Entities.deleteEntity(ball);
      Entities.deleteEntity(ballSpawningAnchor);
    })
  };