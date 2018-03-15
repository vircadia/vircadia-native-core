//
// Sandbox/cubeScript.js
// 
// Author: Liv Erickson
// Copyright High Fidelity 2018
//
// Licensed under the Apache 2.0 License
// See accompanying license file or http://apache.org/
//
(function(){
    var id;

    var Cube = function(){

    };

    Cube.prototype = {
        preload: function(entityID) {
            id = entityID;
        },
        startNearGrab : function() {
            print("I've been grabbed");
            Entities.editEntity(id, {"parentID" : "{00000000-0000-0000-0000-000000000000}", script: ""});
            print(JSON.stringify(Entities.getEntityProperties(id)));
        },
        startDistanceGrab : function() {
            Entities.editEntity(id, {"parentID" : "{00000000-0000-0000-0000-000000000000}", script: ""});
        }
    };

    return new Cube();
});