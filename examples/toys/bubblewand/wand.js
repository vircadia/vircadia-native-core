//  wand.js
//  part of bubblewand
//
//  Script Type: Entity Script
//  Created by James B. Pollack @imgntn -- 09/03/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Makes bubbles when you wave the object around.
//  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

// 'use strict';

(function() {

  Script.include("../../utilities.js");
  Script.include("../../libraries/utils.js");

  var BUBBLE_MODEL = "http://hifi-public.s3.amazonaws.com/james/bubblewand/models/bubble/bubble.fbx";
  var BUBBLE_SCRIPT = Script.resolvePath('bubble.js?' + randInt(0, 5000));

  var HAND_SIZE = 0.25;
  var TARGET_SIZE = 0.04;

  var BUBBLE_GRAVITY = {
    x: 0,
    y: -0.1,
    z: 0
  }

  var WAVE_MODE_GROWTH_FACTOR = 0.005;
  var SHRINK_LOWER_LIMIT = 0.02;
  var SHRINK_FACTOR = 0.001;
  var VELOCITY_STRENGTH_LOWER_LIMIT = 0.01;
  var MAX_VELOCITY_STRENGTH = 10;
  var BUBBLE_DIVISOR = 50;
  var BUBBLE_LIFETIME_MIN = 3;
  var BUBBLE_LIFETIME_MAX = 8;
  var WAND_TIP_OFFSET = 0.05;
  var BUBBLE_SIZE_MIN = 1;
  var BUBBLE_SIZE_MAX = 5;
  var VELOCITY_THRESHOLD = 1; 

  function getGustDetectorPosition() {
    //put the zone in front of your avatar's face
    var DISTANCE_IN_FRONT = 0.2;
    var DISTANCE_UP = 0.5;
    var DISTANCE_TO_SIDE = 0.0;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);

    var upOffset = Vec3.multiply(up, DISTANCE_UP);
    var rightOffset = Vec3.multiply(right, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(rightOffset, frontOffset), upOffset);
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
  }


  var wandEntity = this;

  this.preload = function(entityID) {
    this.entityID = entityID;
    this.properties = Entities.getEntityProperties(this.entityID);
    BubbleWand.init();
  }

  this.unload = function(entityID) {
    Entities.editEntity(entityID, {
      name: ""
    });
    Script.update.disconnect(BubbleWand.update);
    Entities.deleteEntity(BubbleWand.currentBubble);
    while (BubbleWand.bubbles.length > 0) {
      Entities.deleteEntity(BubbleWand.bubbles.pop());
    }

  };


  var BubbleWand = {
    bubbles: [],
    timeSinceMoved: 0,
    currentBubble: null,
    update: function(deltaTime) {
      BubbleWand.internalUpdate(deltaTime);
    },
    internalUpdate: function(deltaTime) {
      var properties = Entities.getEntityProperties(wandEntity.entityID);
    
      var _t = this;
      var GRAB_USER_DATA_KEY = "grabKey";
      var defaultGrabData = {
        activated: false,
        avatarId: null
      };
      var grabData = getEntityCustomData(GRAB_USER_DATA_KEY, wandEntity.entityID, defaultGrabData);
      if (grabData.activated && grabData.avatarId == MyAvatar.sessionUUID) {
        // remember we're being grabbed so we can detect being released
        _t.beingGrabbed = true;

        // print out that we're being grabbed
        _t.handleGrabbedWand(properties);

      } else if (_t.beingGrabbed) {

        // if we are not being grabbed, and we previously were, then we were just released, remember that
        // and print out a message
        _t.beingGrabbed = false;

        return
      }
        var wandTipPosition = _t.getWandTipPosition(properties);
        //update the bubble to stay with the wand tip
        Entities.editEntity(_t.currentBubble, {
          position: wandTipPosition,

        });
    },
    getWandTipPosition: function(properties) {
      print('get wand position')
      var upVector = Quat.getUp(properties.rotation);
      var frontVector = Quat.getFront(properties.rotation);
      var upOffset = Vec3.multiply(upVector, WAND_TIP_OFFSET);
      var wandTipPosition = Vec3.sum(properties.position, upOffset);
      return wandTipPosition
    },
    handleGrabbedWand: function(properties) {
      var _t = this;

      var wandPosition = properties.position;

      var wandTipPosition = _t.getWandTipPosition(properties)
      _t.wandTipPosition = wandTipPosition;

      var velocity = Vec3.subtract(wandPosition, _t.lastPosition)
      var velocityStrength = Vec3.length(velocity) * 100;

      if (velocityStrength < VELOCITY_STRENGTH_LOWER_LIMIT) {
        velocityStrength = 0
      }

      //store the last position of the wand for velocity calculations
      _t.lastPosition = wandPosition;

      if (velocityStrength > MAX_VELOCITY_STRENGTH) {
        velocityStrength = MAX_VELOCITY_STRENGTH
      }

      //actually grow the bubble
      var dimensions = Entities.getEntityProperties(_t.currentBubble).dimensions;

      if (velocityStrength > VELOCITY_THRESHOLD) {

        //add some variation in bubble sizes
        var bubbleSize = randInt(BUBBLE_SIZE_MIN, BUBBLE_SIZE_MAX);
        bubbleSize = bubbleSize / BUBBLE_DIVISOR;

        //release the bubble if its dimensions are bigger than the bubble size
        if (dimensions.x > bubbleSize) {
          //bubbles pop after existing for a bit -- so set a random lifetime
          var lifetime = randInt(BUBBLE_LIFETIME_MIN, BUBBLE_LIFETIME_MAX);

          Entities.editEntity(_t.currentBubble, {
            velocity: velocity,
            lifetime: lifetime
          });

          //release the bubble -- when we create a new bubble, it will carry on and this update loop will affect the new bubble
          BubbleWand.spawnBubble();

          return
        } else {
          dimensions.x += WAVE_MODE_GROWTH_FACTOR * velocityStrength;
          dimensions.y += WAVE_MODE_GROWTH_FACTOR * velocityStrength;
          dimensions.z += WAVE_MODE_GROWTH_FACTOR * velocityStrength;

        }
      } else {
        if (dimensions.x >= SHRINK_LOWER_LIMIT) {
          dimensions.x -= SHRINK_FACTOR;
          dimensions.y -= SHRINK_FACTOR;
          dimensions.z -= SHRINK_FACTOR;
        }

      }

      //update the bubble to stay with the wand tip
      Entities.editEntity(_t.currentBubble, {
        position: _t.wandTipPosition,
        dimensions: dimensions
      });
    },
    spawnBubble: function() {
      var _t = this;

      //create a new bubble at the tip of the wand
      //the tip of the wand is going to be in a different place than the center, so we move in space relative to the model to find that position
      var properties = Entities.getEntityProperties(wandEntity.entityID);
      var wandPosition = properties.position;

      wandTipPosition = _t.getWandTipPosition(properties);
      _t.wandTipPosition = wandTipPosition;

      //store the position of the tip for use in velocity calculations
      _t.lastPosition = wandPosition;

      //create a bubble at the wand tip
      _t.currentBubble = Entities.addEntity({
        type: 'Model',
        modelURL: BUBBLE_MODEL,
        position: wandTipPosition,
        dimensions: {
          x: 0.01,
          y: 0.01,
          z: 0.01
        },
        collisionsWillMove: true, //true
        ignoreForCollisions: false, //false
        gravity: BUBBLE_GRAVITY,
        shapeType: "sphere",
        script: BUBBLE_SCRIPT,
      });
      //add this bubble to an array of bubbles so we can keep track of them
      _t.bubbles.push(_t.currentBubble)

    },
    init: function() {
      var _t = this;
      this.spawnBubble();
      Script.update.connect(BubbleWand.update);
    }
  }



})