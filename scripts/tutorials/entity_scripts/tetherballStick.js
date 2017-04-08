"use strict";
/* jslint vars: true, plusplus: true, forin: true*/
/* globals Tablet, Script, AvatarList, Users, Entities, MyAvatar, Camera, Overlays, Vec3, Quat, Controller, print, getControllerWorldLocation */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// tetherballStick.js
//
// Created by Triplelexx on 17/03/04
// Updated by MrRoboman on 17/03/26
// Copyright 2017 High Fidelity, Inc.
//
// Entity script for an equippable stick with a tethered ball
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {
    var _this;

    var NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
    var LINE_WIDTH = 0.02;
    var MAX_DISTANCE_MULTIPLIER = 4;
    var ACTION_DISTANCE = 0.35;
    var ACTION_TIMESCALE = 0.035;
    var COLLISION_SOUND_URL = "http://public.highfidelity.io/sounds/Footsteps/FootstepW3Left-12db.wav";
    var TIP_OFFSET = 0.26;
    var LIFETIME = 3600;

    tetherballStick = function() {
        _this = this;
    };

    tetherballStick.prototype = {
        entityID: NULL_UUID,
        ballID: NULL_UUID,
        lineID: NULL_UUID,

        getUserData: function(key) {
          try {
            var stickProps = Entities.getEntityProperties(this.entityID);
            var userData = JSON.parse(stickProps.userData);
            return userData[key];
          } catch (e) {
            print("Error parsing Tetherball Stick UserData in file " +
                  e.fileName + " on line " + e.lineNumber);
          }
        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.ballID = this.getUserData("ballID");
        },

        update: function(dt) {
          _this.drawLine();
        },

        startEquip: function(id, params) {
          this.removeActions();
          this.createLine();

          Script.update.connect(this.update);
        },

        continueEquip: function(id, params) {
          var stickProps = Entities.getEntityProperties(this.entityID);
          var ballProps = Entities.getEntityProperties(this.ballID);
          var tipPosition = this.getTipPosition();
          var distance = Vec3.distance(tipPosition, ballProps.position);
          var maxDistance = ACTION_DISTANCE;

          var dVel = Vec3.subtract(ballProps.velocity, stickProps.velocity);
          var dPos = Vec3.subtract(ballProps.position, stickProps.position);
          var ballWithinMaxDistance = distance <= maxDistance;
          var ballMovingCloserToStick = Vec3.dot(dVel, dPos) < 0;
          var ballAboveStick = ballProps.position.y > tipPosition.y;

          if(this.hasAction()) {
            if(ballWithinMaxDistance && (ballMovingCloserToStick || ballAboveStick)) {
              this.removeActions();
            } else {
              this.updateOffsetAction();
            }
          } else if(!ballWithinMaxDistance && !ballMovingCloserToStick){
            this.createOffsetAction();
          }

          this.capBallDistance();
        },

        releaseEquip: function(id, params) {
          this.deleteLine();
          this.createSpringAction();

          Script.update.disconnect(this.update);
        },

        getTipPosition: function() {
          var stickProps = Entities.getEntityProperties(this.entityID);
          var stickFront = Quat.getFront(stickProps.rotation);
          var frontOffset = Vec3.multiply(stickFront, TIP_OFFSET);
          var tipPosition = Vec3.sum(stickProps.position, frontOffset);

          return tipPosition;
        },

        getStickFrontPosition: function() {
          var stickProps = Entities.getEntityProperties(this.entityID);
          var stickFront = Quat.getFront(stickProps.rotation);
          var tipPosition = this.getTipPosition();
          var frontPostion = Vec3.sum(tipPosition, Vec3.multiply(TIP_OFFSET * 0.4, stickFront));

          return frontPostion;
        },

        capBallDistance: function() {
            var stickProps = Entities.getEntityProperties(this.entityID);
            var ballProps = Entities.getEntityProperties(this.ballID);
            var tipPosition = this.getTipPosition();
            var distance = Vec3.distance(tipPosition, ballProps.position);
            var maxDistance = ACTION_DISTANCE * MAX_DISTANCE_MULTIPLIER;

            if(distance > maxDistance) {
                var direction = Vec3.normalize(Vec3.subtract(ballProps.position, tipPosition));
                var newPosition = Vec3.sum(tipPosition, Vec3.multiply(maxDistance, direction));
                Entities.editEntity(this.ballID, {
                    position: newPosition
                })
            }
        },

        createLine: function() {
          if(!this.hasLine()) {
            this.lineID = Entities.addEntity({
                type: "PolyLine",
                name: "TetherballStick Line",
                color: {
                    red: 0,
                    green: 120,
                    blue: 250
                },
                textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
                position: this.getTipPosition(),
                dimensions: {
                    x: 10,
                    y: 10,
                    z: 10
                },
                lifetime: this.getUserData("lifetime")
            });
          }
        },

        deleteLine: function() {
          Entities.deleteEntity(this.lineID);
          this.lineID = NULL_UUID;
        },

        hasLine: function() {
            return this.lineID != NULL_UUID;
        },

        drawLine: function() {
          if(this.hasLine()) {
            var stickProps = Entities.getEntityProperties(this.entityID);
            var tipPosition = this.getTipPosition();
            var ballProps = Entities.getEntityProperties(this.ballID);
            var cameraQuat = Vec3.multiplyQbyV(Camera.getOrientation(), Vec3.UNIT_NEG_Z);
            var linePoints = [];
            var normals = [];
            var strokeWidths = [];
            linePoints.push(Vec3.ZERO);
            normals.push(cameraQuat);
            strokeWidths.push(LINE_WIDTH);
            linePoints.push(Vec3.subtract(ballProps.position, tipPosition));
            normals.push(cameraQuat);
            strokeWidths.push(LINE_WIDTH);

            var lineProps = Entities.getEntityProperties(this.lineID);
            Entities.editEntity(this.lineID, {
                linePoints: linePoints,
                normals: normals,
                strokeWidths: strokeWidths,
                position: tipPosition,
            });
          }
        },

        createOffsetAction: function() {
          this.removeActions();

          Entities.addAction("offset", this.ballID, {
              pointToOffsetFrom: this.getTipPosition(),
              linearDistance: ACTION_DISTANCE,
              linearTimeScale: ACTION_TIMESCALE
          });
        },

        createSpringAction: function() {
          this.removeActions();

          Entities.addAction("spring", this.ballID, {
              targetPosition: this.getTipPosition(),
              linearTimeScale: ACTION_TIMESCALE
          });
        },

        updateOffsetAction: function() {
          var actionIDs = Entities.getActionIDs(this.ballID);
          var actionID;

          // Sometimes two offset actions are applied to the ball simultaneously.
          // Here we ensure that only the most recent action is updated
          // and the rest are deleted.
          while(actionIDs.length > 1) {
            actionID = actionIDs.shift();
            Entities.deleteAction(this.ballID, actionID);
          }

          actionID = actionIDs.shift();
          if(actionID) {
            Entities.updateAction(this.ballID, actionID, {
                pointToOffsetFrom: this.getTipPosition()
            });
          }
        },

        removeActions: function() {
          // Ball should only ever have one action, but sometimes sneaky little actions attach themselves
          // So we remove all possible actions in this call.
          var actionIDs = Entities.getActionIDs(this.ballID);
          for(var i = 0; i < actionIDs.length; i++) {
            Entities.deleteAction(this.ballID, actionIDs[i]);
          }
        },

        hasAction: function() {
          return Entities.getActionIDs(this.ballID).length > 0;
        }
    };

    // entity scripts should return a newly constructed object of our type
    return new tetherballStick();
});
