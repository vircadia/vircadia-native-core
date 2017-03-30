//
//  TabletAddressDialog.qml
//
//  Created by Dante Ruiz on 2017/04/24
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtGraphicalEffects 1.0
import "../../controls"
import "../../styles"
import "../../windows"
import "../"
import "../toolbars"
import "../../styles-uit" as HifiStyles
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls


Rectangle {
    id: cardRoot
    HifiStyles.HifiConstants { id: hifi }
    width: parent.width
    height: parent.height
    property string address: ""

    function setUrl(url) {
        cardRoot.address = url;
        webview.url = url;
    }

    function goBack() {
    }

    function visit() {
    }

    Rectangle {
        id: header
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        
        width: parent.width
        height: 50
        color: hifi.colors.white

        Row {
            anchors.fill: parent
            spacing: 80

            Item {
                id: backButton
                anchors {
                    top: parent.top
                    left: parent.left
                    leftMargin: 100
                }
                height: parent.height
                width: parent.height

                HifiStyles.FiraSansSemiBold {
                    text: "BACK"
                    elide: Text.ElideRight
                    anchors.fill: parent
                    size: 16

                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter

                    color: hifi.colors.lightGray

                    MouseArea {
                        id: backButtonMouseArea
                        anchors.fill: parent
                        hoverEnabled: enabled

                        onClicked: {
                            webview.goBack();
                        }
                    }
                }
            }

            Item {
                id: closeButton
                anchors {
                    top: parent.top
                    right: parent.right
                    rightMargin: 100
                }
                height: parent.height
                width: parent.height

                HifiStyles.FiraSansSemiBold {
                    text: "CLOSE"
                    elide: Text.ElideRight
                    anchors.fill: parent
                    size: 16

                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter

                    color: hifi.colors.lightGray

                    MouseArea {
                        id: closeButtonMouseArea
                        anchors.fill: parent
                        hoverEnabled: enabled

                        onClicked: root.pop();
                    }
                }
            }
        }
    }

    HifiControls.WebView {
        id: webview
        anchors {
            top: header.bottom
            right: parent.right
            left: parent.left
            bottom: parent.bottom
        }
    }   
}
