//
//  scripts/system/libraries/handTouch.js
//
//  Created by Luis Cuenca on 12/29/17
//  Copyright 2017 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* jslint bitwise: true */

/* global Script, Overlays, Controller, Vec3, Quat, MyAvatar, Entities
*/

(function(){

    var updateFingerWithIndex = 0;
    
    // Keys to access finger data
    var dataKeys = ["pinky", "ring", "middle", "index", "thumb"];
    
    // Additionally close the hands to achieve a grabbing effect 
    var grabPercent = { left: 0, 
                        right: 0 };
    
    // var isGrabbing = false;
    
    // Store which fingers are touching - if all false restate the default poses 
    var isTouching = {
        left: {
            pinky: false, 
            middle: false, 
            ring: false, 
            thumb: false, 
            index: false          
        }, right: {
            pinky: false, 
            middle: false, 
            ring: false, 
            thumb: false, 
            index: false
        }
    }
    
    // joint data for opened pose
    
    var dataOpen = {
        left: {
            pinky: [{x: -0.18262, y:-2.666, z:-25.11229}, {x: 1.28845, y:0, z:1.06604}, {x: -3.967, y:0, z:-0.8351} ], 
            middle: [{x: -0.18262, y:0, z:-3.2809}, {x: 1.28845, y:0, z:-0.71834}, {x: -3.967, y:0, z:0.83978}], 
            ring: [{x: -0.18262, y:-1.11078, z:-16.24391}, {x: 1.28845, y:0, z:0.68153}, {x: -3.967, y:0, z:-0.69295}], 
            thumb: [{x: 5.207, y:2.595, z:38.40092}, {x: -9.869, y:11.755, z:10.50012}, {x: -9.778, y:9.647, z:15.16963}], 
            index: [{x: -0.18262, y:0.0099, z:2.28085}, {x: 1.28845, y:-0.01107, z:0.93037}, {x: -3.967, y:0, z:-2.64018}]          
        }, right: {
            pinky: [{x: -0.111, y: 2.66601, z: 12.06423}, {x: 1.217, y: 0, z: -1.03973}, {x: -3.967, y: 0, z: 0.86424}], 
            middle: [{x: -0.111, y: 0, z: 3.26538}, {x: 1.217, y: 0, z: 0.71427}, {x: -3.967, y: 0, z: -0.85103}], 
            ring: [{x: -0.111, y: 1.11101, z: 3.56312}, {x: 1.217, y: 0, z: -0.64524}, {x: -3.967, y: 0, z: 0.69807}], 
            thumb: [{x: 5.207, y: -2.595, z: -38.26131}, {x: -9.869, y: -11.755, z: -10.51778}, {x: -9.77799, y: -9.647, z: -15.10783}], 
            index: [{x: -0.111, y: 0, z: -2.2816}, {x: 1.217, y: 0, z: -0.90168}, {x: -3.967, y: 0, z: 2.62649}]
        }
    }
    
    // joint data for closed hand

    var dataClose = {
        left: {
            pinky:[{x: 75.45709, y:-8.01347, z:-22.54823}, {x: 69.562, y:0, z:1.06604}, {x: 74.73801, y:0, z:-0.8351}], 
            middle: [{x: 66.0237, y:-2.42536, z:-6.13193}, {x: 72.63042, y:0, z:-0.71834}, {x: 75.19901, y:0, z:0.83978}], 
            ring: [{x: 71.52988, y:-2.35423, z:-16.21694}, {x: 64.44739, y:0, z:0.68153}, {x: 70.518, y:0, z:-0.69295}], 
            thumb: [{x: 33.83371, y:-15.19106, z:34.66116}, {x: 0, y:0, z:-43.42915}, {x: 0, y:0, z:-30.18613}], 
            index: [{x: 35.56082, y:-1.21056, z:-2.07362}, {x: 79.79845, y:-0.01107, z:0.93037}, {x: 68.767, y:0, z:-2.64018}]
        }, right: {
            pinky:[{x: 75.45702, y: 8.013, z: 22.41022}, {x: 69.562, y: 0, z: -1.03973}, {x: 74.738, y: 0, z: 0.86424}], 
            middle: [{x: 66.02399, y: 2.425, z: 6.11638}, {x: 72.63002, y: 0, z: 0.71427}, {x: 72.63, y: 0, z: -0.85103}], 
            ring: [{x: 71.53, y: 5.022, z: 16.33612}, {x: 64.447, y: 0, z: -0.64524}, {x: 70.51801, y: 0, z: 0.69807}], 
            thumb: [{x: 33.834, y: 15.191, z: -34.52131}, {x: 0, y: 0, z: 43.41122}, {x: 0, y: 0, z: 30.24818}], 
            index: [{x: 35.633, y: 1.215, z: -6.6376}, {x: 79.72701, y: 0, z: -0.90168}, {x: 68.76701, y: 0, z: 2.62649}]
        }
    }
    
    // joint data for the current frame
    
    var dataCurrent = {
        left:{
            pinky:[{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            middle: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            ring: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            thumb: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            index: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}]
        },
        right:{
            pinky:[{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            middle: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            ring: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            thumb: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            index: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}]
        }
    } 
    
    // interpolated values on joint data to smooth movement 
    
    var dataDelta = {
        left:{
            pinky:[{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            middle: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            ring: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            thumb: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            index: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}]
        },
        right:{
            pinky:[{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            middle: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            ring: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            thumb: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}], 
            index: [{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0},{x: 0, y: 0, z: 0}]
        }
    }
    
    // Acquire an updated value per hand every 5 frames
    
    var animationSteps = 5;
    
    // Debugging info
    
    var showSphere = false;
    var showLines = false;
    
    // store the rays for the fingers - only for debug purposes
    
    var fingerRays = {  
        left:{pinky: undefined, middle: undefined, ring: undefined, thumb: undefined, index: undefined}, 
        right:{pinky: undefined, middle: undefined, ring: undefined, thumb: undefined, index: undefined}
    };
    
    // Register object with API Debugger 
    
    var varsToDebug = {
        toggleDebugSphere: function(){
            showSphere = !showSphere;
        },
        toggleDebugLines: function(){
            showLines = !showLines;
        },
        fingerPercent: {
            left: {
                pinky: 0.75, 
                middle: 0.75, 
                ring: 0.75, 
                thumb: 0.75, 
                index: 0.75
            } , 
            right: {
                pinky: 0.75, 
                middle: 0.75, 
                ring: 0.75, 
                thumb: 0.75, 
                index: 0.75
            }
        },
        triggerValues: {
            leftTriggerValue: 0,
            leftTriggerClicked: 0,
            rightTriggerValue: 0,
            rightTriggerClicked: 0,
            leftSecondaryValue: 0,
            rightSecondaryValue: 0
        }
    }
    
    // Add/Subtract the joint data - per finger joint
    
    function addVals(val1, val2, sign) {
        var val = [];
        if (val1.length != val2.length) return;
        for (var i = 0; i < val1.length; i++) {
            val.push({x: 0, y: 0, z: 0});
            val[i].x = val1[i].x + sign*val2[i].x;
            val[i].y = val1[i].y + sign*val2[i].y;
            val[i].z = val1[i].z + sign*val2[i].z;
        }
        return val;
    }
    
    // Multiply/Divide  the joint data - per finger joint
    
    function multiplyValsBy(val1, num) {
        var val = [];
        for (var i = 0; i < val1.length; i++) {
            val.push({x: 0, y: 0, z: 0});
            val[i].x = val1[i].x * num;
            val[i].y = val1[i].y * num;
            val[i].z = val1[i].z * num;
        }
        return val;
    }
    
    // Calculate the finger lengths by adding its joint lengths
    
    function getJointDistances(jointNamesArray) {
        var result = {distances: [], totalDistance: 0}
        for (var i = 1; i < jointNamesArray.length; i++) {
            var index0 = MyAvatar.getJointIndex(jointNamesArray[i-1]);
            var index1 = MyAvatar.getJointIndex(jointNamesArray[i]);
            var pos0 = MyAvatar.getJointPosition(index0);
            var pos1 = MyAvatar.getJointPosition(index1);
            var distance = Vec3.distance(pos0, pos1);
            result.distances.push(distance);
            result.totalDistance += distance;
        }
        return result;
    }
    
    // Calculate the sphere that look up for entities, the center of the palm, perpendicular vector from the palm plane and origin of the the finger rays
    
    function estimatePalmData(side) {
        // Return data object
        var data = {position: undefined, perpendicular: undefined, distance: undefined, fingers: {pinky: undefined, middle: undefined, ring: undefined, thumb: undefined, index: undefined}};        
        
        var jointOffset = { x: 0, y: 0, z: 0 }; 
        
        var upperSide = side[0].toUpperCase() + side.substring(1);
        var jointIndexHand = MyAvatar.getJointIndex(upperSide + "Hand");
        
        // Store position of the hand joint
        var worldPosHand = MyAvatar.jointToWorldPoint(jointOffset, jointIndexHand);
        var minusWorldPosHand = {x:-worldPosHand.x, y:-worldPosHand.y, z:-worldPosHand.z};
        
        // Data for finger rays
        var directions = {pinky: undefined, middle: undefined, ring: undefined, thumb: undefined, index: undefined};
        var positions = {pinky: undefined, middle: undefined, ring: undefined, thumb: undefined, index: undefined};
        
        // Calculate palm center
        var palmCenter = {x:0, y:0, z:0};
        palmCenter = Vec3.sum(worldPosHand, palmCenter);
        
        var thumbLength = 0;
        
        for (var i = 0; i < dataKeys.length; i++) {
            var finger = dataKeys[i];
            var jointNames = getJointNames(upperSide, finger, 4);
            var fingerLength = getJointDistances(jointNames).totalDistance;
            
            var jointIndex = MyAvatar.getJointIndex(jointNames[0]);
            positions[finger] = MyAvatar.jointToWorldPoint(jointOffset, jointIndex);
            directions[finger] = Vec3.normalize(Vec3.sum(positions[finger], minusWorldPosHand));
            data.fingers[finger] = Vec3.sum(positions[finger], Vec3.multiply(fingerLength, directions[finger]));
            if (finger != "thumb") {
                palmCenter = Vec3.sum(Vec3.multiply(2, positions[finger]), palmCenter); // Hand joint + 2 * 4 fingers(no thumb) = 9
            } else {
                thumbLength = fingerLength;
            }
        }
        
        data.perpendicular = side == "right" ? Vec3.normalize(Vec3.cross(directions["ring"], directions["pinky"])) : Vec3.normalize(Vec3.cross(directions["pinky"],directions["ring"]));
        
        
        data.position = Vec3.multiply(1.0/9, palmCenter); // Hand joint + 2 * 4 fingers(no thumb) = 9
        
        data.distance = 1.55*Vec3.distance(data.position, positions["index"]);

        // move back thumb check up origin
        
        data.fingers["thumb"] = Vec3.sum(data.fingers["thumb"], Vec3.multiply( -0.2 * thumbLength, data.perpendicular));
        
        return data;
    }
    
    // Register GlobalDebugger for API Debugger
    Script.registerValue("GlobalDebugger", varsToDebug);

    // Create debug overlays - finger rays + palm rays + spheres
    
    for (var i = 0; i < dataKeys.length; i++) {
        fingerRays["left"][dataKeys[i]] = Overlays.addOverlay("line3d", {
            color: { red: 0, green: 0, blue: 255 },
            start: { x:0, y:0, z:0 },
            end: { x:0, y:1, z:0 },
            visible: showLines
        });
        fingerRays["right"][dataKeys[i]] = Overlays.addOverlay("line3d", {
            color: { red: 0, green: 0, blue: 255 },
            start: { x:0, y:0, z:0 },
            end: { x:0, y:1, z:0 },
            visible: showLines
        });
    }
    
    var palmRay = {
        left: Overlays.addOverlay("line3d", {
            color: { red: 255, green: 0, blue: 0 },
            start: { x:0, y:0, z:0 },
            end: { x:0, y:1, z:0 },
            visible: showLines
        }),
        right: Overlays.addOverlay("line3d", {
            color: { red: 255, green: 0, blue: 0 },
            start: { x:0, y:0, z:0 },
            end: { x:0, y:1, z:0 },
            visible: showLines
        })
    }
    
    var sphereHand = {
        right: Overlays.addOverlay("sphere", {
            position: MyAvatar.position,
            color: { red: 0, green: 255, blue: 0 },
            scale: { x: 0.01, y: 0.01, z: 0.01 },
            visible: showSphere
        }),
        left: Overlays.addOverlay("sphere", {
            position: MyAvatar.position,
            color: { red: 0, green: 255, blue: 0 },
            scale: { x: 0.01, y: 0.01, z: 0.01 },
            visible: showSphere
        })
    }
    
    function updateSphereHand(side) {
        
        var palmData = estimatePalmData(side);
        var palmPoint = palmData.position;
        var dist = 1.5*palmData.distance;
        
        // Situate the debugging overlays 
        
        var checkOffset = { x: palmData.perpendicular.x * dist, 
                            y: palmData.perpendicular.y * dist, 
                            z: palmData.perpendicular.z * dist };
        
        
        var spherePos = Vec3.sum(palmPoint, checkOffset);
        var checkPoint = Vec3.sum(palmPoint, Vec3.multiply(2, checkOffset));
        
        Overlays.editOverlay(palmRay[side], {
            start: palmPoint,
            end: checkPoint,
            visible: showLines
        }); 
        
        Overlays.editOverlay(sphereHand[side], {
            position: spherePos,
            scale: {
                x: 2*dist,
                y: 2*dist,
                z: 2*dist
            },
            visible: showSphere
        });     

        for (var i = 0; i < dataKeys.length; i++) {
            Overlays.editOverlay(fingerRays[side][dataKeys[i]], {
                start: palmData.fingers[dataKeys[i]],
                end: checkPoint,
                visible: showLines
            });
        }       
        
        // Update the intersection of only one finger at a time
        
        var finger = dataKeys[updateFingerWithIndex];
        
        var grabbables = Entities.findEntities(spherePos, dist);
                
        if (grabbables.length > 0) {    
            var origin = palmData.fingers[finger];
            var direction = Vec3.normalize(Vec3.subtract(checkPoint, origin));
            var intersection = Entities.findRayIntersection({origin: origin, direction: direction}, true, grabbables, [], true, false);
            var percent = 0.75;
            var isAbleToGrab = intersection.intersects && intersection.distance < 2.5*dist;
            // Store if this finger is touching something
            isTouching[side][finger] = isAbleToGrab;
            if (isAbleToGrab) {
                // update the open/close percentage for this finger 
                percent = intersection.distance/(2.5*dist);
                var grabMultiplier = finger === "thumb" ? 0.2 : 0.05;
                percent += grabMultiplier * grabPercent[side];
            } 
            varsToDebug.fingerPercent[side][finger] = percent; // store the current open/close percentage
        }       
        // Calculate new interpolation data 
        var totalDistance = addVals(dataClose[side][finger], dataOpen[side][finger], -1);
        percent = varsToDebug.fingerPercent[side][finger];
        var newFingerData = addVals(dataOpen[side][finger], multiplyValsBy(totalDistance, percent), 1);
        
        // Assign animation interpolation steps
        dataDelta[side][finger] = multiplyValsBy(addVals(newFingerData, dataCurrent[side][finger], -1), 1.0/animationSteps);
    }
    
    // Recreate the finger joint names - and how many 0 to count
    function getJointNames(side, finger, count) {
        var names = [];
        for (var i = 1; i < count+1; i++) {
            var name = side+"Hand"+finger[0].toUpperCase()+finger.substring(1)+(i);
            names.push(name);
        }
        return names;
    }

    // Capture the controller values
    
    var leftTriggerPress = function (value) {
        varsToDebug.triggerValues.leftTriggerValue = value;
        // the value for the trigger increments the hand-close percentage 
        grabPercent["left"] = value;
    };
    var leftTriggerClick = function (value) {
        varsToDebug.triggerValues.leftTriggerClicked = value;
    };
    var rightTriggerPress = function (value) {
        varsToDebug.triggerValues.rightTriggerValue = value;
        // the value for the trigger increments the hand-close percentage 
        grabPercent["right"] = value;
    };
    var rightTriggerClick = function (value) {
        varsToDebug.triggerValues.rightTriggerClicked = value;
    };
    var leftSecondaryPress = function (value) {
        varsToDebug.triggerValues.leftSecondaryValue = value;
    };
    var rightSecondaryPress = function (value) {
        varsToDebug.triggerValues.rightSecondaryValue = value;
    };
    
    var MAPPING_NAME = "com.highfidelity.handTouch";
    var mapping = Controller.newMapping(MAPPING_NAME);
    mapping.from([Controller.Standard.RT]).peek().to(rightTriggerPress);
    mapping.from([Controller.Standard.RTClick]).peek().to(rightTriggerClick);
    mapping.from([Controller.Standard.LT]).peek().to(leftTriggerPress);
    mapping.from([Controller.Standard.LTClick]).peek().to(leftTriggerClick);

    mapping.from([Controller.Standard.RB]).peek().to(rightSecondaryPress);
    mapping.from([Controller.Standard.LB]).peek().to(leftSecondaryPress);
    mapping.from([Controller.Standard.LeftGrip]).peek().to(leftSecondaryPress);
    mapping.from([Controller.Standard.RightGrip]).peek().to(rightSecondaryPress);

    Controller.enableMapping(MAPPING_NAME);
    
    function getTouching(side) {
        var animating = false;
        for (var i = 0; i < dataKeys.length; i++) {
            var finger = dataKeys[i];
            animating = animating || isTouching[side][finger];
        }
        return animating;
    }
    
    Script.update.connect(function(){
        
        // iterate fingers
        
        updateFingerWithIndex = (updateFingerWithIndex < dataKeys.length-1) ? updateFingerWithIndex + 1 : 0;
        
        // precalculate data
        
        updateSphereHand("right");
        updateSphereHand("left");

        // Assign interpolated values
        var i, j, index, finger, names, quatRot;
		
        for (i = 0; i < dataKeys.length; i++) {
            finger = dataKeys[i];
            names = getJointNames("Right", finger, 3);
            dataCurrent["right"][finger] = addVals(dataCurrent["right"][finger], dataDelta["right"][finger], 1);
            for (j = 0; j < names.length; j++) {
                index = MyAvatar.getJointIndex(names[j]);
                // if no finger is touching restate the default poses
                if (getTouching("right")) {
                    quatRot = Quat.fromVec3Degrees(dataCurrent["right"][finger][j]);
                    MyAvatar.setJointRotation(index, quatRot);  
                } else {
                    MyAvatar.clearJointData(index);
                }
            }
        }

        for (i = 0; i < dataKeys.length; i++) {
            finger = dataKeys[i];
            names = getJointNames("Left", finger, 3);
            dataCurrent["left"][finger] = addVals(dataCurrent["left"][finger], dataDelta["left"][finger], 1);
            for (j = 0; j < names.length; j++) {
                index = MyAvatar.getJointIndex(names[j]);
                // if no finger is touching restate the default poses
                if (getTouching("left")) {
                    quatRot = Quat.fromVec3Degrees(dataCurrent["left"][finger][j]);
                    MyAvatar.setJointRotation(index, quatRot);  
                } else {
                    MyAvatar.clearJointData(index);
                }
            }
        }

    });

}())
