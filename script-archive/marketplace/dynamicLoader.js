//
//  dynamicLoader.js
//  examples
//
//  Created by Eric Levin on 11/10/2015.
//  Copyright 2013 High Fidelity, Inc.
//
//  This script is an example of a way to dynamically load and place models in a grid from a specified s3 directory on the hifi-public bucket. 
//  The directory can be specified by changing the query string variable on line 19 to the desired relative path.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html



var BASE_URL = "https://hifi-public.s3.amazonaws.com/";
var models = [];
var floorPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));;
floorPosition.y = MyAvatar.position.y - 5;
var floor = Entities.addEntity({
    type: "Model",
    modelURL: "https://hifi-public.s3.amazonaws.com/ozan/3d_marketplace/props/floor/3d_mp_floor.fbx",
    position: floorPosition,
    shapeType: 'box',
    dimensions: {
        x: 1000,
        y: 9,
        z: 1000
    }
});

var urls = [];
var req = new XMLHttpRequest();
req.open("GET", "https://serene-headland-4300.herokuapp.com/?assetDir=ozan/3d_marketplace/sets", false);
req.send();

var res = req.responseText;
var urls = JSON.parse(res).urls;
if (urls.length > 0) {
        // We've got an array of urls back from server- let's display them in grid
    createGrid();
}

function createGrid() {
    var fbxUrls = urls.filter(function(url) {
        return url.indexOf('fbx') !== -1;
    });

    var modelParams = {
        type: "Model",
        dimensions: {
            x: 10,
            y: 10,
            z: 10
        },
    };

    var modelPosition = {
        x: floorPosition.x + 10,
        y: floorPosition.y + 8.5,
        z: floorPosition.z
    };

    for (var i = 0; i < fbxUrls.length; i++) {
        if(i % 2 === 0) {
          modelPosition.x = floorPosition.x - 40
        } else {
            modelPosition.x = floorPosition.x + 40
        }
        modelPosition.z -= 30;
        modelParams.position = modelPosition;
        modelParams.modelURL = BASE_URL + fbxUrls[i]
        var model = Entities.addEntity(modelParams);
        models.push(model);
    }

    Script.setTimeout(function() {
        //Until we add callbacks on model loaded, we need to set a timeout and hope model is loaded by the time 
        //we hit it in order to set model dimensions correctly
        for(var i = 0; i < models.length; i++){
            var modelDimensions = Entities.getEntityProperties(models[i], 'naturalDimensions').naturalDimensions;
            Entities.editEntity(models[i], {dimensions: modelDimensions});
        }    
    }, 10000);
}

function cleanup() {
    Entities.deleteEntity(floor);
    models.forEach(function(model) {
        Entities.deleteEntity(model);
    });
    Entities.deleteEntity(model);
}

Script.scriptEnding.connect(cleanup);
