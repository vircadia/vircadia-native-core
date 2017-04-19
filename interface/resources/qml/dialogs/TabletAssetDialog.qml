//
//  TabletAssetDialog.qml
//
//  Created by David Rowe on 18 Apr 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

import "../styles-uit"
import "../windows"

import "assetDialog"

TabletModalWindow {
    id: root
    anchors.fill: parent
    width: parent.width
    height: parent.height

    // Set from OffscreenUi::assetDialog().
    property alias caption: root.title
    property alias dir: assetDialogContent.dir
    property alias filter: assetDialogContent.filter
    property alias options: assetDialogContent.options

    // Dialog results.
    signal selectedAsset(var asset);
    signal canceled();

    property int titleWidth: 0  // For TabletModalFrame.

    TabletModalFrame {
        id: frame
        anchors.fill: parent

        AssetDialogContent {
            id: assetDialogContent
            singleClickNavigate: true
            width: parent.width - 12
            height: parent.height - frame.frameMarginTop - 12
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: parent.top
                topMargin: parent.height - height - 6
            }
        }
    }
}
