//
//  bottomHudOptions.qml
//  interface/resources/qml/android
//
//  Created by Gabriel Calero & Cristian Duarte on 19 Jan 2018
//  Copyright 2018 High Fidelity, Inc.
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
import "../../styles" as Styles
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls
import ".."
import "."

Item {
    id: bar
    x:0

    property bool shown: true

    signal sendToScript(var message);

    onShownChanged: {
        bar.visible = shown;
    }

    function hide() {
        //shown = false;
        sendToScript({ method: "hide" });
    }

    Styles.HifiConstants { id: hifi }
    HifiConstants { id: android }
    MouseArea {
        anchors.fill: parent
    }

	Rectangle {
        id: background
        anchors.fill : parent
 		color: "#FF000000"
        border.color: "#FFFFFF"
        anchors.bottomMargin: -1
        anchors.leftMargin: -1
        anchors.rightMargin: -1
        Flow {
            id: flowMain
            spacing: 10
            anchors.fill: parent
            anchors.topMargin: 12
            anchors.bottomMargin: 12
            anchors.rightMargin: 12
            anchors.leftMargin: 72
        }


        Rectangle {
            id: hideButton
            height: android.dimen.headerHideWidth
            width: android.dimen.headerHideHeight
            color: "#00000000"
            anchors {
                right: parent.right
                rightMargin: android.dimen.headerHideRightMargin
                top: parent.top
                topMargin: android.dimen.headerHideTopMargin
            }

            Image {
                id: hideIcon
                source: "../../../icons/hide.svg"
                width: android.dimen.headerHideIconWidth
                height: android.dimen.headerHideIconHeight
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.top
                }
            }
            FiraSansRegular {
                anchors {
                    top: hideIcon.bottom
                    horizontalCenter: hideIcon.horizontalCenter
                    topMargin: 12
                }
                text: "HIDE"
                color: "#FFFFFF"
                font.pixelSize: hifi.fonts.pixelSize * 2.5;
            }
        
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    hide();
                }
            }
        }
	}

    Component.onCompleted: {
        // put on bottom
        width = Window.innerWidth;
        height = 255;
        y = Window.innerHeight - height;
    }
    
    function addButton(properties) {
        var component = Qt.createComponent("button.qml");
        if (component.status == Component.Ready) {
            var button = component.createObject(flowMain);
            // copy all properites to button
            var keys = Object.keys(properties).forEach(function (key) {
                button[key] = properties[key];
            });
            return button;
        } else if( component.status == Component.Error) {
            console.log("Load button errors " + component.errorString());
        }
    }

    function urlHelper(src) {
            if (src.match(/\bhttp/)) {
                return src;
            } else {
                return "../../../" + src;
            }
        }

}
