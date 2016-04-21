//
//  scriptableResourceTest.js
//  examples/tests
//
//  Created by Zach Pomerantz on 4/20/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Preloads textures to play a simple movie, plays it, and frees those textures.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// A model exported from blender with a texture named 'Picture' on one face.  
var FRAME_URL = "http://hifi-production.s3.amazonaws.com/tutorials/pictureFrame/finalFrame.fbx";
// A folder full of individual frames.
var MOVIE_URL = "http://hifi-content.s3.amazonaws.com/james/vidtest/";

var center = Vec3.sum(
    Vec3.sum(MyAvatar.position, { x: 0, y: 0.5, z: 0 }),
    Vec3.multiply(1, Quat.getFront(Camera.getOrientation()))
);

// Left-pad num with 0s until it is size digits
function pad(num, size) {
    var s = num + "";
    while (s.length < size) s = "0" + s;
    return s;
}

var pictureFrameProperties = {
    name: 'scriptableResourceTest Picture Frame',
    type: 'Model',
    position: center,
    modelURL: FRAME_URL,
    dynamic: true,
}
var pictureFrame = Entities.addEntity(pictureFrameProperties);

var frames = [];

// Preload
var numLoading = 0;
for (var i = 0; i < 159; i++) {
    var padded = pad(i, 3);
    var filepath = MOVIE_URL + padded + '.jpg';
    var texture = TextureCache.prefetch(filepath);
    frames.push(texture);
    if (!texture.loaded) {
        numLoading++;
        texture.loadedChanged.connect(function() {
            numLoading--;
            if (!numLoading) play();
        });
    }
}

function play() {
    var frame = 0;
    var movieInterval = Script.setInterval(function() {
        Entities.editEntity(pictureFrame, { textures: JSON.stringify({ Picture: frames[frame].url }) })
        frame += 1;
        if (frame == 159) {
            Script.clearInterval(movieInterval);
            Entities.deleteEntity(pictureFrame);
            // free the textures at the next garbage collection
            while (frames.length) frames.pop();
            // alternatively, the textures can be forcibly freed:
            // frames.forEach(function(texture) { texture.release(); });
            Script.requestGarbageCollection();
        }
    }, 33.3);
}
