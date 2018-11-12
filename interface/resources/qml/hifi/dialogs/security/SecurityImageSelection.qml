//
//  SecurityImageSelection.qml
//  qml\hifi\dialogs\security
//
//  Security
//
//  Created by Zach Fox on 2018-10-31
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import stylesUit 1.0 as HifiStylesUit
import controlsUit 1.0 as HifiControlsUit
import "qrc:////qml//controls" as HifiControls

// references XXX from root context

Item {
    HifiStylesUit.HifiConstants { id: hifi; }

    id: root;
    property alias currentIndex: securityImageGrid.currentIndex;

    SecurityImageModel {
        id: gridModel;
    }

    GridView {
        id: securityImageGrid;
        interactive: false;
        clip: true;
        // Anchors
        anchors.fill: parent;
        currentIndex: -1;
        cellWidth: width / 3;
        cellHeight: height / 2;
        model: gridModel;
        delegate: Item {
            width: securityImageGrid.cellWidth;
            height: securityImageGrid.cellHeight;
            Item {
                anchors.fill: parent;
                Image {
                    width: parent.width - 12;
                    height: parent.height - 12;
                    source: sourcePath;
                    anchors.horizontalCenter: parent.horizontalCenter;
                    anchors.verticalCenter: parent.verticalCenter;
                    fillMode: Image.PreserveAspectFit;
                    mipmap: true;
                    cache: false;
                }
            }
            MouseArea {
                anchors.fill: parent;
                propagateComposedEvents: false;
                onClicked: {
                    securityImageGrid.currentIndex = index;
                }
            }
        }
        highlight: Rectangle {
            width: securityImageGrid.cellWidth;
            height: securityImageGrid.cellHeight;
            color: hifi.colors.blueHighlight;
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //
    function getImagePathFromImageID(imageID) {
        return (imageID ? gridModel.getImagePathFromImageID(imageID) : "");
    }

    function getSelectedImageIndex() {
        return gridModel.get(securityImageGrid.currentIndex).securityImageEnumValue;
    }

    function initModel() {
        gridModel.initModel();
        securityImageGrid.currentIndex = -1;
    }

    function resetSelection() {
        securityImageGrid.currentIndex = -1;
    }
    //
    // FUNCTION DEFINITIONS END
    //
}
