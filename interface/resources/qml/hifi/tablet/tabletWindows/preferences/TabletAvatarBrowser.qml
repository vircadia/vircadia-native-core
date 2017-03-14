//
//  TabletAvatarBrowser.qml
//
//  Created by David Rowe on 14 Mar 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebChannel 1.0
import QtWebEngine 1.2

import "../../../../windows"
import "../../../../controls-uit"
import "../../../../styles-uit"

Item {
    id: root
    objectName: "ModelBrowserDialog"

    property string title: "Attachment Model"

    property var result

    signal selected(var modelUrl)
    signal canceled()

    anchors.fill: parent

    /*
    Rectangle {
        id: pane  // Surrogate for ScrollingWindow's pane.
        anchors.fill: parent
    }
    */

    BaseWebView {
        id: webview
        url: "https://metaverse.highfidelity.com/marketplace?category=avatars"
        focus: true

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: footer.top
        }
    }

    Rectangle {
        id: footer
        height: 40

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        color: hifi.colors.baseGray

        Row {
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: hifi.dimensions.contentMargin.x
            }

            Button {
                text: "Cancel"
                color: hifi.buttons.white
                onClicked: root.destroy();
            }
        }
    }
}
