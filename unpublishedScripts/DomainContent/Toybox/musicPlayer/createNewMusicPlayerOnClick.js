//
//  createNewMusicPlayerOnClick.js
//
//  Created by Brad Hefta-Gaub on 3/3/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Entity Script that you can attach to any entity to have it spawn new "music players"
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function(){ 

    var musicPlayerScript = Script.resolvePath("./musicPlayer.js");
    var imageShader = Script.resolvePath("./imageShader.fs");
    var defaultImage = Script.resolvePath("./defaultImage.jpg");

    function getPositionToCreateEntity() {
        var distance = 0.5;
        var direction = Quat.getFront(Camera.orientation);
        var offset = Vec3.multiply(distance, direction);
        var placementPosition = Vec3.sum(Camera.position, offset);

        var cameraPosition = Camera.position;

        var HALF_TREE_SCALE = 16384;

        var cameraOutOfBounds = Math.abs(cameraPosition.x) > HALF_TREE_SCALE || Math.abs(cameraPosition.y) > HALF_TREE_SCALE || Math.abs(cameraPosition.z) > HALF_TREE_SCALE;
        var placementOutOfBounds = Math.abs(placementPosition.x) > HALF_TREE_SCALE || Math.abs(placementPosition.y) > HALF_TREE_SCALE || Math.abs(placementPosition.z) > HALF_TREE_SCALE;

        if (cameraOutOfBounds && placementOutOfBounds) {
            return null;
        }

        placementPosition.x = Math.min(HALF_TREE_SCALE, Math.max(-HALF_TREE_SCALE, placementPosition.x));
        placementPosition.y = Math.min(HALF_TREE_SCALE, Math.max(-HALF_TREE_SCALE, placementPosition.y));
        placementPosition.z = Math.min(HALF_TREE_SCALE, Math.max(-HALF_TREE_SCALE, placementPosition.z));

        return placementPosition;
    }

    function createNewIPOD() {
        var iPodPosition = { x: 0, y: .5, z: 0}; 
        var iPodDimensions = { x: 0.15, y: 0.3, z: 0.03 };
        var overlayDimensions = { x: 0.13, y: 0.13, z: 0.001 };
        var boxOverlayDimensions = { x: 0.13, y: 0.13, z: 0.0001 };

        var iPodID = Entities.addEntity({
            type: "Box",
            name: "music player",
            position: iPodPosition,
            dimensions: iPodDimensions,
            color: { red: 150, green: 150, blue: 150},
            script: musicPlayerScript,
            dynamic: true
        });
        print(iPodID);

        var textID = Entities.addEntity({
            type: "Text",
            name: "now playing",
            position: { x: 0, y: (0.5+0.07), z: 0.0222},
            dimensions: overlayDimensions,
            color: { red: 150, green: 150, blue: 150},
            parentID: iPodID,
            lineHeight: 0.01,
            text: "Pause"
        });


        var newAlbumArt = JSON.stringify(
                {
                    "ProceduralEntity": {
                        "version":2,
                        "shaderUrl":imageShader,
                        "uniforms":{"iSpeed":0,"iShell":1},
                        "channels":[defaultImage]
                    }
                });


        var albumArtID = Entities.addEntity({
            type: "Box",
            name: "album art",
            position: { x: 0, y: (0.5-0.07), z: 0.01506},
            dimensions: boxOverlayDimensions,
            color: { red: 255, green: 255, blue: 255},
            userData: newAlbumArt,
            parentID: iPodID
        });
        Entities.editEntity(iPodID, { position: getPositionToCreateEntity() });
    }


    this.clickDownOnEntity = function(myID, mouseEvent) { 
        createNewIPOD();
    }; 
})


