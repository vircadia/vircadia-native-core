//
//  lib.js
//  scripts/developer/tests/scriptableResource 
//
//  Created by Zach Pomerantz on 4/20/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Preloads textures to play a simple movie, plays it, and frees those textures.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var NUM_FRAMES = 158; // 158 available
var FRAME_RATE = 30;  // 30  default

function getFrame(callback) {
    // A model exported from blender with a texture named 'Picture' on one face.  
    var FRAME_URL = "https://cdn-1.vircadia.com/us-e-1/Developer/Tutorials/pictureFrame/finalFrame.fbx";

    var model = ModelCache.prefetch(FRAME_URL);
    if (model.state === Resource.State.FINISHED) {
        makeFrame(Resource.State.FINISHED);
    } else {
        model.stateChanged.connect(makeFrame);
    }

    function makeFrame(state) {
        if (state === Resource.State.FAILED) { throw "Failed to load frame"; }
        if (state !== Resource.State.FINISHED) { return; }

        var pictureFrameProperties = {
            name: 'scriptableResourceTest Picture Frame',
            type: 'Model',
            position: getPosition(),
            modelURL: FRAME_URL,
            dynamic: true,
        };

        callback(Entities.addEntity(pictureFrameProperties));
    }

    function getPosition() {
        // Always put it 5 meters in front of you
        var position = MyAvatar.position;
        var yaw = MyAvatar.bodyYaw + MyAvatar.getHeadFinalYaw();
        var rads = (yaw / 180) * Math.PI;

        position.y += 0.5;
        position.x += - 5 * Math.sin(rads);
        position.z += - 5 * Math.cos(rads);

        return position;
    }
}

function prefetch(callback) {
    // A folder full of individual frames.
    var MOVIE_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/james/vidtest/");

    var frames = [];

    var numLoading = 0;
    for (var i = 1; i <= NUM_FRAMES; ++i) {
        var padded = pad(i, 3);
        var filepath = MOVIE_URL + padded + '.jpg';
        var texture = TextureCache.prefetch(filepath);
        frames.push(texture);
        if (texture.state !== Resource.State.FINISHED) {
            numLoading++;
            texture.stateChanged.connect(function(state) {
                if (state === Resource.State.FAILED || state === Resource.State.FINISHED) {
                    --numLoading;
                    if (!numLoading) { callback(frames); }
                }
            });
        }
    }
    if (!numLoading) { callback(frames); }

    function pad(num, size) { // left-pad num with zeros until it is size digits
        var s = num.toString();
        while (s.length < size) { s = "0" + s; }
        return s;
    }
}

function play(model, frames, callback) {
    var frame = 0;
    var movieInterval = Script.setInterval(function() {
        Entities.editEntity(model, { textures: JSON.stringify({ Picture: frames[frame].url }) });
        if (++frame >= frames.length) {
            Script.clearInterval(movieInterval);
            callback();
        }
    }, 1000 / FRAME_RATE);
}

