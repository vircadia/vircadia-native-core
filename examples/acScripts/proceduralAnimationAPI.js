//
// proceduralAnimation.js
// hifi
//
// Created by Ben Arnold on 7/29/14.
// Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
// This is a Procedural Animation API for creating procedural animations in JS.
// To include it in your JS files, simply use the following line at the top:
// Script.include("proceduralAnimation.js");

// You can see a usage example in proceduralBot.js
// The current implementation is quite simple. If you would like a feature
// to be added or expanded, you can contact Ben at brb555@vols.utk.edu

ProcAnimAPI = function() {

    // generateKeyFrames(rightAngles, leftAngles, middleAngles, numFrames)
    // 
    // Parameters:
    //   rightAngles -  An array of tuples. The angles in degrees for the joints
    //                  on the right side of the body
    //   leftAngles -   An array of tuples. The angles in degrees for the joints
    //                  on the left side of the body
    //   middleAngles - An array of tuples. The angles in degrees for the joints
    //                  on the left side of the body
    //   numFrames    - The number of frames in the animation, before mirroring.
    //                  for a 4 frame walk animation, simply supply 2 frames
    //                  and generateKeyFrames will return 4 frames.
    //
    // Return Value:
    //   Returns an array of KeyFrames. Each KeyFrame has an array of quaternions
    //   for each of the joints, generated from the input angles. They will be ordered
    //   R,L,R,L,...M,M,M,M where R ~ rightAngles, L ~ leftAngles, M ~ middlesAngles.
    //   The size of the returned array will be numFrames * 2
    this.generateKeyframes = function(rightAngles, leftAngles, middleAngles, numFrames) {

        if (rightAngles.length != leftAngles.length) {
            print("ERROR: generateKeyFrames(...) rightAngles and leftAngles must have equal length.");
        }

        //for mirrored joints, such as the arms or legs
        var rightQuats = [];
        var leftQuats = [];
        //for non mirrored joints such as the spine
        var middleQuats = [];
        
        for (var i = 0; i < numFrames; i++) {
            rightQuats[i] = [];
            leftQuats[i] = [];
            middleQuats[i] = [];
        }

        var finalKeyFrames = [];
        //Generate quaternions
        for (var i = 0; i < rightAngles.length; i++) {
            for (var j = 0; j < rightAngles[i].length; j++) { 
                rightQuats[i][j] = Quat.fromPitchYawRollDegrees(rightAngles[i][j][0], rightAngles[i][j][1], rightAngles[i][j][2]);
                leftQuats[i][j] = Quat.fromPitchYawRollDegrees(leftAngles[i][j][0], -leftAngles[i][j][1], -leftAngles[i][j][2]);
            }
        }
        for (var i = 0; i < middleAngles.length; i++) {
            for (var j = 0; j < middleAngles[i].length; j++) { 
                 middleQuats[i][j] = Quat.fromPitchYawRollDegrees(middleAngles[i][j][0], middleAngles[i][j][1], middleAngles[i][j][2]);
            }
        }
        
        for (var i = 0; i < numFrames; i++) {
            finalKeyFrames[i] = new this.KeyFrame(rightQuats[i], leftQuats[i], middleQuats[i]);
        }

        //Generate mirrored quaternions for the other half of the animation
        for (var i = 0; i < rightAngles.length; i++) {
            for (var j = 0; j < rightAngles[i].length; j++) { 
                rightQuats[i][j] = Quat.fromPitchYawRollDegrees(rightAngles[i][j][0], -rightAngles[i][j][1], -rightAngles[i][j][2]);
                leftQuats[i][j] = Quat.fromPitchYawRollDegrees(leftAngles[i][j][0], leftAngles[i][j][1], leftAngles[i][j][2]);
            }
        }
        for (var i = 0; i < middleAngles.length; i++) {
            for (var j = 0; j < middleAngles[i].length; j++) { 
                 middleQuats[i][j] = Quat.fromPitchYawRollDegrees(-middleAngles[i][j][0], -middleAngles[i][j][1], -middleAngles[i][j][2]);
            }
        }
        for (var i = 0; i < numFrames; i++) {
            finalKeyFrames[numFrames + i] = new this.KeyFrame(leftQuats[i], rightQuats[i], middleQuats[i]);
        }
        
        //Generate control points
        this.computeBezierControlPoints(finalKeyFrames);
        
        return finalKeyFrames;
    };
    
    //Computes 2 controlPoints to each keyframe to be used in the bezier evaluation.
    //Technique is described at: //https://www.cs.tcd.ie/publications/tech-reports/reports.94/TCD-CS-94-18.pdf
    this.computeBezierControlPoints = function(keyFrames) {
        //Hook up pointers to the next keyframe
        for (var i = 0; i < keyFrames.length - 1; i++) {
            keyFrames[i].nextFrame = keyFrames[i+1];
        }
        keyFrames[keyFrames.length-1].nextFrame = keyFrames[0];

        //Set up all C1
        for (var i = 0; i < keyFrames.length; i++) {
            keyFrames[i].nextFrame.controlPoints = [];
            for (var j = 0; j < keyFrames[i].rotations.length; j++) {
                keyFrames[i].nextFrame.controlPoints[j] = [];
                var R = Quat.slerp(keyFrames[i].rotations[j], keyFrames[i].nextFrame.rotations[j], 2.0);
                var T = Quat.slerp(R, keyFrames[i].nextFrame.nextFrame.rotations[j], 0.5);
                keyFrames[i].nextFrame.controlPoints[j][0] = Quat.slerp(keyFrames[i].nextFrame.rotations[j], T, 0.33333);
            }
        }
        //Set up all C2
        for (var i = 0; i < keyFrames.length; i++) {
            for (var j = 0; j < keyFrames[i].rotations.length; j++) {
                keyFrames[i].controlPoints[j][1] = Quat.slerp(keyFrames[i].nextFrame.rotations[j], keyFrames[i].nextFrame.controlPoints[j][0], -1.0);
            }
        }
    };

    // Animation KeyFrame constructor. rightJoints and leftJoints must be the same size
    this.KeyFrame = function(rightJoints, leftJoints, middleJoints) {
        this.rotations = [];
        
        for (var i = 0; i < rightJoints.length; i++) {
            this.rotations[this.rotations.length] = rightJoints[i];
            this.rotations[this.rotations.length] = leftJoints[i];
        }
        for (var i = 0; i < middleJoints.length; i++) {
            this.rotations[this.rotations.length] = middleJoints[i];
        }
    };

    // DeCasteljau evaluation to evaluate the bezier curve.
    // This is a very natural looking interpolation
    this.deCasteljau = function(k1, k2, c1, c2, f) {
        var a = Quat.slerp(k1, c1, f);
        var b = Quat.slerp(c1, c2, f);
        var c = Quat.slerp(c2, k2, f);
        var d = Quat.slerp(a, b, f);
        var e = Quat.slerp(b, c, f);
        return Quat.slerp(d, e, f);
    };
}