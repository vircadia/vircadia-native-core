//
//  SecurityImageModel.qml
//  qml/hifi/commerce
//
//  SecurityImageModel
//
//  Created by Zach Fox on 2017-08-17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

ListModel {
    id: root;

    function initModel() {
        var array = [];
        while (array.length < 6) {
            // We currently have 34 security images to choose from
            var randomNumber = Math.floor(Math.random() * 34) + 1;
            if (array.indexOf(randomNumber) > -1) {
                continue;
            }
            array[array.length] = randomNumber;
        }

        var modelElement;

        for (var i = 0; i < 6; i++) {
            modelElement = { "sourcePath":"images/" + addLeadingZero(array[i]) + ".jpg", "securityImageEnumValue": (i + 1) }
            root.insert(i, modelElement);
        }
    }

    function addLeadingZero(n) {
        return n < 10 ? '0' + n : '' + n;
    }

    function getImagePathFromImageID(imageID) {
        return (imageID ? root.get(imageID - 1).sourcePath : "");
    }
}
