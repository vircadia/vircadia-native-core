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

    var LIFETIME_IF_LEAVE_DOMAIN = 2;
    var LINE_WIDTH = 0.02;
    var COLLISION_SOUND_URL = "http://public.highfidelity.io/sounds/Footsteps/FootstepW3Left-12db.wav";
    var TIP_OFFSET = 0.26;

    tetherballStick = function() {
        _this = this;
    };

    tetherballStick.prototype = {

        getUserData: function() {
          try {
            var stickProps = Entities.getEntityProperties(this.entityID);
            var userData = JSON.parse(stickProps.userData);
            return userData;
          } catch (e) {
            print("Error parsing Tetherball Stick UserData in file " +
                  e.fileName + " on line " + e.lineNumber);
          }
        },

        preload: function(entityID) {
            this.entityID = entityID;

            var userData = this.getUserData();
            this.ballID = userData.ballID;
            this.lineID = userData.lineID;
            this.actionID = userData.actionID;
            this.maxDistanceBetweenBallAndStick = userData.maxDistanceBetweenBallAndStick;
        },

        update: function(dt) {
          _this.drawLine();
        },

        startEquip: function() {
          Script.update.disconnect(this.update);
        },

        continueEquip: function(id, params) {
          var stickProps = Entities.getEntityProperties(this.entityID);
          [this.entityID, this.ballID, this.lineID].forEach(function(id) {
            Entities.editEntity(id, {
              lifetime: stickProps.age + LIFETIME_IF_LEAVE_DOMAIN
            });
          });
          this.updateOffsetAction();
          this.capBallDistance();
          this.drawLine();
        },

        releaseEquip: function() {
          var userData = this.getUserData();
          [this.entityID, this.ballID, this.lineID].forEach(function(id) {
            Entities.editEntity(id, {
              lifetime: userData.lifetime
            });
          });
          Script.update.connect(this.update);
        },

        getTipPosition: function() {
          var stickProps = Entities.getEntityProperties(this.entityID);
          var stickFront = Quat.getFront(stickProps.rotation);
          var frontOffset = Vec3.multiply(stickFront, TIP_OFFSET);
          var tipPosition = Vec3.sum(stickProps.position, frontOffset);

          return tipPosition;
        },

        updateOffsetAction: function() {
          Entities.updateAction(this.ballID, this.actionID, {
              pointToOffsetFrom: this.getTipPosition()
          });
        },

        capBallDistance: function() {
            var stickProps = Entities.getEntityProperties(this.entityID);
            var ballProps = Entities.getEntityProperties(this.ballID);
            var tipPosition = this.getTipPosition();
            var distance = Vec3.distance(tipPosition, ballProps.position);
            var maxDistance = this.maxDistanceBetweenBallAndStick;

            if(distance > maxDistance) {
                var direction = Vec3.normalize(Vec3.subtract(ballProps.position, tipPosition));
                var newPosition = Vec3.sum(tipPosition, Vec3.multiply(maxDistance, direction));
                Entities.editEntity(this.ballID, {
                    position: newPosition
                })
            }
        },

        drawLine: function() {
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
    };

    // entity scripts should return a newly constructed object of our type
    return new tetherballStick();
});
