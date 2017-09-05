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
    ListElement{
        sourcePath: "images/01.jpg"
        securityImageEnumValue: 1;
    }
    ListElement{
        sourcePath: "images/02.jpg"
        securityImageEnumValue: 2;
    }
    ListElement{
        sourcePath: "images/03.jpg"
        securityImageEnumValue: 3;
    }
    ListElement{
        sourcePath: "images/04.jpg"
        securityImageEnumValue: 4;
    }
    ListElement{
        sourcePath: "images/05.jpg"
        securityImageEnumValue: 5;
    }
    ListElement{
        sourcePath: "images/06.jpg"
        securityImageEnumValue: 6;
    }

    function getImagePathFromImageID(imageID) {
        return (imageID ? root.get(imageID - 1).sourcePath : "");
    }
}
