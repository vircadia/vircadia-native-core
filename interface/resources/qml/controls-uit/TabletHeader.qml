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

    color: hifi.colors.darkGray

    RalewayBold {
        text: title
        size: 26
        color: hifi.colors.white
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: hifi.dimensions.contentMargin.x
    }
}
