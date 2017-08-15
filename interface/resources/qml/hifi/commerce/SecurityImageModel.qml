//
//  SecurityImageModel.qml
//  qml/hifi/commerce
//
//  SecurityImageModel
//
//  Created by Zach Fox on 2017-08-15
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

ListModel {
    id: root;
    ListElement{
        sourcePath: "images/01cat.jpg"
        securityImageEnumValue: 1;
    }
    ListElement{
        sourcePath: "images/02car.jpg"
        securityImageEnumValue: 2;
    }
    ListElement{
        sourcePath: "images/03dog.jpg"
        securityImageEnumValue: 3;
    }
    ListElement{
        sourcePath: "images/04stars.jpg"
        securityImageEnumValue: 4;
    }
    ListElement{
        sourcePath: "images/05plane.jpg"
        securityImageEnumValue: 5;
    }
    ListElement{
        sourcePath: "images/06gingerbread.jpg"
        securityImageEnumValue: 6;
    }
}
