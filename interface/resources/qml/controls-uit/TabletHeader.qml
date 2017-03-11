//
//  TabletHeader.qml
//
//  Created by David Rowe on 11 Mar 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

import "../styles-uit"

Rectangle {

    property string title: ""

    HifiConstants { id: hifi }

    height: hifi.dimensions.tabletMenuHeader
    z: 100

    gradient: Gradient {
        GradientStop {
            position: 0
            color: "#2b2b2b"
        }

        GradientStop {
            position: 1
            color: "#1e1e1e"
        }
    }

    RalewayBold {
        text: title
        size: 26
        color: "#34a2c7"
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: hifi.dimensions.contentMargin.x
    }
}
