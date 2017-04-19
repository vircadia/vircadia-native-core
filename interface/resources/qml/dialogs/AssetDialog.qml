//
//  AssetDialog.qml
//
//  Created by David Rowe on 18 Apr 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../styles-uit"
import "../windows"

import "assetDialog"

ModalWindow {
    id: root
    resizable: true
    implicitWidth: 480
    implicitHeight: 360

    minSize: Qt.vector2d(360, 240)
    draggable: true

    Settings {
        category: "AssetDialog"
        property alias width: root.width
        property alias height: root.height
        property alias x: root.x
        property alias y: root.y
    }

    // Set from OffscreenUi::assetDialog().
    property alias caption: root.title
    property alias dir: assetDialogContent.dir
    property alias filter: assetDialogContent.filter
    property alias options: assetDialogContent.options

    // Dialog results.
    signal selectedAsset(var asset);
    signal canceled();

    property int titleWidth: 0  // For ModalFrame.

    HifiConstants { id: hifi }

    AssetDialogContent {
        id: assetDialogContent

        width: pane.width
        height: pane.height
        anchors.margins: 0
    }
}
