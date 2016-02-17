  var ball, ballDetector, tiltMaze;
  var MAZE_MODEL_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/tiltMaze/MAZE2.fbx";
  var MAZE_COLLISION_HULL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/tiltMaze/MAZE_COLLISION_HULL3.obj";
  var BALL_DETECTOR_SCRIPT = Script.resolvePath('ballDetector.js?' + Math.random())


  var MAZE_DIMENSIONS = {
      x: 1,
      y: 0.3,
      z: 1
  };

  var BALL_DIMENSIONS = {
      x: 0.025,
      y: 0.025,
      z: 0.025
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

  var createBall = function(position) {
    var mazeProps = Entities.getEntityProperties(tiltMaze);
    var right = Quat.getRight(mazeProps.rotation);
    var front = Quat.getFront(mazeProps.rotation);
    var offset = {
        x:0,
        y:0.02,
        z:0
    };

    var finalOffset = Vec3.sum(offset,Vec3.multiply(right,-0.4));
    finalOffset = Vec3.sum(finalOffset,Vec3.multiply(front,-0.2));

      var properties = {
          name: 'Hifi Tilt Maze Ball',
          type: 'Sphere',
          shapeType: 'sphere',
          dimensions: BALL_DIMENSIONS,
          color: BALL_COLOR,
          position: Vec3.sum(position,finalOffset),
          dynamic: true,
          collisionless:false,
          damping:0.6,

      };
      ball = Entities.addEntity(properties);
  };

  var createBallSpawningAnchor = function(position) {
      var properties = {
          name: 'Hifi Tilt Maze Ball Detector',
          parentID: tiltMaze,
          type: 'Box',
          color: DEBUG_COLOR,
          dimensions: BALL_DETECTOR_DIMENSIONS,
          position: center,
          collisionless: true,
          visible: true,
          script: BALL_DETECTOR_SCRIPT
      };
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
          rotation: Quat.fromPitchYawRollDegrees(0, 0, 180),
          dynamic: true,
      }
      tiltMaze = Entities.addEntity(properties)
  }

  var createAll = function() {
      createTiltMaze(center);
      // createBallDetector();
      createBall(center);
  }

  createAll();

  if (CLEANUP === true) {
      Script.scriptEnding.connect(function() {
          Entities.deleteEntity(tiltMaze);
          Entities.deleteEntity(ball);
      })
  }