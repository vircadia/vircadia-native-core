//
//  SecurityImageSelection.qml
//  qml/hifi/commerce/wallet
//
//  SecurityImageSelection
//
//  Created by Zach Fox on 2017-08-17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;
    property int currentIndex: securityImageGrid.currentIndex;
    
    // This will cause a bug -- if you bring up security image selection in HUD mode while
    // in HMD while having HMD preview enabled, then move, then finish passphrase selection,
    // HMD preview will stay off.
    // TODO: Fix this unlikely bug
    onVisibleChanged: {
        if (visible) {
            sendSignalToWallet({method: 'disableHmdPreview'});
        } else {
            sendSignalToWallet({method: 'maybeEnableHmdPreview'});
        }
    }

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
    signal sendSignalToWallet(var msg);

    function getImagePathFromImageID(imageID) {
        return (imageID ? gridModel.getImagePathFromImageID(imageID) : "");
    }

    function getSelectedImageIndex() {
        return gridModel.get(securityImageGrid.currentIndex).securityImageEnumValue;
    }

    function initModel() {
        gridModel.initModel();
    }
    //
    // FUNCTION DEFINITIONS END
    //
}
