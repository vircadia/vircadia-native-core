  var ball, ballDetector, tiltMaze;
  var MAZE_MODEL_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/tiltMaze/MAZE2.fbx";
  var MAZE_COLLISION_HULL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/tiltMaze/MAZE_COLLISION_HULL.obj";
  var BALL_DETECTOR_SCRIPT = Script.resolvePath('ballDetector.js?' + Math.random())


  var MAZE_DIMENSIONS = {
      x: 1,
      y: 0.3,
      z: 1
  };

  var BALL_DIMENSIONS = {
      x: 0.1,
      y: 0.1,
      z: 0.1
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


  var createBall = function(position) {
      var properties = {
          name: 'Hifi Tilt Maze Ball'
          type: 'Sphere',
          shapeType: 'sphere',
          dimensions: BALL_DIMENSIONS,
          color: BALL_COLOR,
          position: position
          dynamic: true,
      };
      ball = Entities.addEntity(properties);
  };

  var createBallSpawningAnchor: function() {
      var properties = {
          name: 'Hifi Tilt Maze Ball Detector'
          parentID: tiltMaze,
          type: 'Box',
          dimensions: BALL_DETECTOR_DIMENSIONS,
          position: position,
          rotiation: rotation,
          collisionless: true,
          script: BALL_DETECTOR_SCRIPT
      };
  }

  var createBallDetector = function(position, rotation) {
      var properties = {
          name: 'Hifi Tilt Maze Ball Detector'
          parentID: tiltMaze,
          type: 'Box',
          dimensions: BALL_DETECTOR_DIMENSIONS,
          position: position,
          rotiation: rotation,
          collisionless: true,
          script: BALL_DETECTOR_SCRIPT
      };
      ballDetector = Entities.addEntity(properties);
  };

  var createTiltMaze: function(position, rotation) {
      var properties = {
          name: 'Hifi Tilt Maze',
          type: 'Model',
          compoundShapeURL: MAZE_COLLISION_HULL,
          dimensions: MAZE_DIMENSIONS,
          position: position,
          rotation: rotation,
          dynamic: true,
      }
      tiltMaze = Entities.addEntity(properties)
  }

  var createAll = function() {
      createTiltMaze();
      // createBallDetector();
      // createBall();
  }

  createAll();