//
//  ImageMessageBox.qml
//
//  Created by Dante Ruiz on 7/5/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import "../styles-uit"

Item {
    id: imageBox
    visible: false
    anchors.fill: parent
    property alias source: image.source
    property alias imageWidth: image.width
    proeprty alias imageHeight: image.height
    Rectangle {
        acnhors.fill: parent
        color: "black"
        opacity: 0.5
    }

    Image {
        id: image
    }

}
