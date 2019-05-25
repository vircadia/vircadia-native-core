//
//  bottomHudOptions.qml
//  interface/resources/qml/android
//
//  Created by Cristian Duarte & Gabriel Calero on 24 Nov 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0
import "../../styles" as HifiStyles
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../controls" as HifiControls
import ".."
import "."

Item {
    id: bottomHud

    property bool shown: false

    signal sendToScript(var message);

    HifiConstants { id: android }

    onShownChanged: {
        bottomHud.visible = shown;
    }

    function hide() {
        shown = false;
    }

	Rectangle {
        anchors.fill : parent
 		color: "transparent"
        Flow {
            id: flowMain
            spacing: 0
            flow: Flow.LeftToRight
            layoutDirection: Flow.LeftToRight
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.margins: 12

            Rectangle {
                id: hideButton
                height: android.dimen.headerHideHeight
                width: android.dimen.headerHideWidth
                color: "#00000000"
                Image {
                    id: hideIcon
                    source: "../../../icons/show-up.svg"
                    width: android.dimen.headerHideIconWidth
                    height: android.dimen.headerHideIconHeight
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        sendToScript ({ method: "showUpBar" });
                    }
                }
            }
        }
	}

    Component.onCompleted: {
        width = android.dimen.botomHudWidth;
        height = android.dimen.botomHudHeight;
        x=Window.innerWidth - width;
        y=Window.innerHeight - height;
    }

}
