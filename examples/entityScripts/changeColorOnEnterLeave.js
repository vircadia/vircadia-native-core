//
//  changeColorOnEnterLeave.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 3/31/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){ 
    function getRandomInt(min, max) {
        return Math.floor(Math.random() * (max - min + 1)) + min;
    }

    this.enterEntity = function(myID) { 
        print("enterEntity() myID:" + myID);
        Entities.editEntity(myID, { color: { red: getRandomInt(128,255), green: getRandomInt(128,255), blue: getRandomInt(128,255)} });
    }; 

    this.leaveEntity = function(myID) { 
        print("leaveEntity() myID:" + myID);
        Entities.editEntity(myID, { color: { red: getRandomInt(128,255), green: getRandomInt(128,255), blue: getRandomInt(128,255)} });
    }; 
})