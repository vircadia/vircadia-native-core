//
//  crazylegsOnClick.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to an entity, that entity will make your avatar do the 
//  crazyLegs dance if you click on it.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){ 
    var cumulativeTime = 0.0;
    var FREQUENCY = 5.0;
    var AMPLITUDE = 45.0;
    var jointList = MyAvatar.getJointNames(); 
    var jointMappings = "\n# Joint list start";
    for (var i = 0; i < jointList.length; i++) {
        jointMappings = jointMappings + "\njointIndex = " + jointList[i] + " = " + i;
    }
    print(jointMappings + "\n# Joint list end");  

    this.crazyLegsUpdate = function(deltaTime) { 
        print("crazyLegsUpdate... deltaTime:" + deltaTime);
        cumulativeTime += deltaTime;
        print("crazyLegsUpdate... cumulativeTime:" + cumulativeTime);
        MyAvatar.setJointData("RightUpLeg", Quat.fromPitchYawRollDegrees(AMPLITUDE * Math.sin(cumulativeTime * FREQUENCY), 0.0, 0.0));
        MyAvatar.setJointData("LeftUpLeg", Quat.fromPitchYawRollDegrees(-AMPLITUDE * Math.sin(cumulativeTime * FREQUENCY), 0.0, 0.0));
        MyAvatar.setJointData("RightLeg", Quat.fromPitchYawRollDegrees(
            AMPLITUDE * (1.0 + Math.sin(cumulativeTime * FREQUENCY)),0.0, 0.0));
        MyAvatar.setJointData("LeftLeg", Quat.fromPitchYawRollDegrees(
            AMPLITUDE * (1.0 - Math.sin(cumulativeTime * FREQUENCY)),0.0, 0.0));
    }; 

    this.stopCrazyLegs = function() { 
        MyAvatar.clearJointData("RightUpLeg");
        MyAvatar.clearJointData("LeftUpLeg");
        MyAvatar.clearJointData("RightLeg");
        MyAvatar.clearJointData("LeftLeg");
    }; 

    this.clickDownOnEntity = function(entityID, mouseEvent) { 
        print("clickDownOnEntity()...");
        cumulativeTime = 0.0;
        Script.update.connect(this.crazyLegsUpdate);
    }; 

    this.clickReleaseOnEntity = function(entityID, mouseEvent) { 
        print("clickReleaseOnEntity()...");
        this.stopCrazyLegs();
        Script.update.disconnect(this.crazyLegsUpdate);
    }; 
})

