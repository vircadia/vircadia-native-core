//
// makeHouses.js
//
//
// Created by Stojce Slavkovski on March 14, 2015
// Copyright 2015 High Fidelity, Inc.
//
// This sample script that creates house entities based on parameters.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    /** options **/
    var numHouses = 100;
    var xRange = 300;
    var yRange = 300;

    var sizeOfTheHouse = {
        x: 10,
        y: 15,
        z: 10
    };
    
    var randomizeModels = false;
    /**/
 
    var modelUrlPrefix = "http://public.highfidelity.io/load_testing/3-Buildings-2-SanFranciscoHouse-";
    var modelurlExt = ".fbx";
    var modelVariations = 100;

    var houses = [];

    function addHouseAt(position, rotation) {
        // get house model
        var modelNumber = randomizeModels ?
            1 + Math.floor(Math.random() * (modelVariations - 1)) :
            (houses.length + 1) % modelVariations;
        
        if (modelNumber == 0) {
            modelNumber = modelVariations;
        }
        
        var modelUrl = modelUrlPrefix + (modelNumber + "") + modelurlExt;
        print("Model ID:" + modelNumber);
        print("Model URL:" + modelUrl);
        
        var properties = {
            type: "Model",
            position: position,
            rotation: rotation,
            dimensions: sizeOfTheHouse,
            modelURL: modelUrl
        };

        return Entities.addEntity(properties);
    }

    // calculate initial position
    var posX = MyAvatar.position.x - (xRange / 2);
    var measures = calculateParcels(numHouses, xRange, yRange);
    var dd = 0;

    // avatar facing rotation
    var rotEven = Quat.fromPitchYawRollDegrees(0, 270.0 + MyAvatar.bodyYaw, 0.0);

    // avatar opposite rotation
    var rotOdd = Quat.fromPitchYawRollDegrees(0, 90.0 + MyAvatar.bodyYaw, 0.0);
    var housePos = Vec3.sum(MyAvatar.position, Quat.getFront(Camera.getOrientation()));

    var housePositions = []
    for (var j = 0; j < measures.rows; j++) {

        var posX1 = 0 - (xRange / 2);
        dd += measures.parcelLength;

        for (var i = 0; i < measures.cols; i++) {

            // skip reminder of houses
            if (houses.length > numHouses) {
                break;
            }

            var posShift = {
                x: posX1,
                y: 0,
                z: dd
            };
            
            housePositions.push(Vec3.sum(housePos, posShift));
            posX1 += measures.parcelWidth;
        }
    }

    // calculate rows and columns in area, and dimension of single parcel
    function calculateParcels(items, areaWidth, areaLength) {

        var idealSize = Math.min(Math.sqrt(areaWidth * areaLength / items), areaWidth, areaLength);

        var baseWidth = Math.min(Math.floor(areaWidth / idealSize), items);
        var baseLength = Math.min(Math.floor(areaLength / idealSize), items);

        var sirRows = baseWidth;
        var sirCols = Math.ceil(items / sirRows);
        var sirW = areaWidth / sirRows;
        var sirL = areaLength / sirCols;

        var visCols = baseLength;
        var visRows = Math.ceil(items / visCols);
        var visW = areaWidth / visRows;
        var visL = areaLength / visCols;

        var rows = 0;
        var cols = 0;
        var parcelWidth = 0;
        var parcelLength = 0;

        if (Math.min(sirW, sirL) > Math.min(visW, visL)) {
            rows = sirRows;
            cols = sirCols;
            parcelWidth = sirW;
            parcelLength = sirL;
        } else {
            rows = visRows;
            cols = visCols;
            parcelWidth = visW;
            parcelLength = visL;
        }

        print("rows:" + rows);
        print("cols:" + cols);
        print("parcelWidth:" + parcelWidth);
        print("parcelLength:" + parcelLength);

        return {
            rows: rows,
            cols: cols,
            parcelWidth: parcelWidth,
            parcelLength: parcelLength
        };
    }

    var addHouses = function() {
        if (housePositions.length > 0) {
            position = housePositions.pop();
            print("House nr.:" + (houses.length + 1));
            houses.push(
                addHouseAt(position, (housePositions.length % 2 == 0) ? rotEven : rotOdd)
            );
            
            // max 20 per second
            Script.setTimeout(addHouses, 50);
        } else {
            Script.stop();
        }
    };
    
    addHouses();
    
})();
