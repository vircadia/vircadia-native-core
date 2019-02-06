//
//  SortButton.qml
//  qml/hifi/commerce/marketplace
//
//  SortButton
//
//  Created by Roxanne Skelly on 2019-01-18
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.9
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../controls" as HifiControls
import "../common" as HifiCommerceCommon
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.
import "../common/sendAsset"
import "../.." as HifiCommon

Item {
    HifiConstants { id: hifi; }
    
    id: root;
    
    
    property string glyph: "";
    property string text: "";
    property bool checked: false;
    signal clicked();
    
    width: childrenRect.width;
    height: parent.height;

    Rectangle {
        anchors.top: parent.top;
        anchors.left: parent.left;
        height: parent.height;
        width: 2;
        color: hifi.colors.faintGray;
        visible: index > 0;
    }
    
    HiFiGlyphs {
        id: buttonGlyph;
        text: root.glyph;
        // Size
        size: 14;
        // Anchors
        anchors.left: parent.left;
        anchors.leftMargin: 0;
        anchors.top: parent.top;
        anchors.verticalCenter: parent.verticalCenter;
        height: parent.height;
        horizontalAlignment: Text.AlignHCenter;
        // Style
        color: hifi.colors.lightGray;
    }
    RalewayRegular {
        id: buttonText;
        text: root.text;
        // Text size
        size: 14;
        // Style
        color: hifi.colors.lightGray;
        elide: Text.ElideRight;
        horizontalAlignment: Text.AlignHCenter;
        verticalAlignment: Text.AlignVCenter;
        // Anchors
        anchors.left: parent.left;
        anchors.leftMargin: 20;
        anchors.top: parent.top;
        height: parent.height;
    }
    MouseArea {
        anchors.fill: parent;
        hoverEnabled: enabled;
        onClicked: {
            root.clicked();
        }
    }
}