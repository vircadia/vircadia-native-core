//
//  SecondLaunchWindow.qml
//
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0
import hifi.simplifiedUI.simplifiedConstants 1.0 as SimplifiedConstants
import hifi.simplifiedUI.simplifiedControls 1.0 as SimplifiedControls

Rectangle {
    id: root
    color: simplifiedUI.colors.white
    anchors.fill: parent

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    Image {
        id: topLeftAccentImage
        width: 180
        height: 450
        anchors.left: parent.left
        anchors.top: parent.top
        source: "images/standOutTopLeft.png"
        z: 1
    }

    Image {
        id: bottomRightAccentImage
        width: 250
        height: 80
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: "images/standOutBottomRight.png"
        z: 1
    }

     Flickable {
        id: tempAvatarPageContainer
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: continueLink.top
        clip: true
        contentWidth: parent.width
        contentHeight: tempAvatarPageGrid.height

        GridLayout {
            id: tempAvatarPageGrid
            width: parent.width
            columns: 2
            rows: 1

            Image {
                Layout.preferredWidth: 428
                Layout.minimumHeight: 240
                Layout.maximumHeight: 800
                Layout.fillHeight: true
                source: resourceDirectoryUrl + "qml/hifi/simplifiedUI/avatarApp/images/hero.png"
                mipmap: true
                fillMode: Image.PreserveAspectFit
            }

            Item {
                id: textAndQRContainer
                Layout.minimumWidth: 300
                Layout.maximumWidth: 1680
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height

                HifiStylesUit.RalewayBold {
                    id: headerText
                    text: "Stand out from the crowd!"
                    color: simplifiedUI.colors.text.black
                    size: 48
                    height: paintedHeight
                    wrapMode: Text.Wrap
                    anchors.top: parent.top
                    anchors.topMargin: 120
                    anchors.left: parent.left
                    anchors.right: parent.right
                }

                HifiStylesUit.RalewayRegular {
                    id: descriptionText
                    anchors.top: headerText.bottom
                    anchors.topMargin: 10
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: paintedHeight
                    text: "You can create and upload custom avatars from our Avatar Creator App. " +
                        "It's as easy as taking a selfie. Available now on iOS and Android Platforms."
                    color: simplifiedUI.colors.text.black
                    size: 24
                    wrapMode: Text.Wrap
                }

                Item {
                    id: qrAndInstructionsContainer
                    anchors.top: descriptionText.bottom
                    anchors.topMargin: 50
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: avatarAppQRCodeImage.height

                    Image {
                        id: avatarAppQRCodeImage
                        source: resourceDirectoryUrl + "qml/hifi/simplifiedUI/avatarApp/images/qrCode.jpg"
                        height: 190
                        width: 190
                        mipmap: true
                        fillMode: Image.PreserveAspectFit
                    }

                    HifiStylesUit.RalewayBold {
                        id: instructionText
                        anchors.top: avatarAppQRCodeImage.top
                        anchors.bottom: avatarAppQRCodeImage.bottom
                        anchors.left: avatarAppQRCodeImage.right
                        anchors.leftMargin: 30
                        anchors.right: parent.right
                        text: "Use your mobile phone to scan this QR code."
                        color: simplifiedUI.colors.text.black
                        size: 24
                        wrapMode: Text.Wrap
                    }
                }
            }
        }

        SimplifiedControls.VerticalScrollBar {
            parent: tempAvatarPageContainer
        }
    }

    HifiStylesUit.RalewayBold {
        id: continueLink
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: paintedHeight + 24
        horizontalAlignment: Text.AlignLeft
        text: "Continue >"
        color: simplifiedUI.colors.text.lightBlue
        opacity: continueMouseArea.containsMouse ? 1.0 : 0.7
        size: 36
        z: 1

        MouseArea {
            id: continueMouseArea
            hoverEnabled: true
            anchors.fill: parent

            onClicked: {
                Tablet.playSound(TabletEnums.ButtonClick);
                sendToScript({
                    "source": "SecondLaunchWindow.qml",
                    "method": "closeSecondLaunchWindow"
                });
            }
        }
    }

    signal sendToScript(var message);
}
