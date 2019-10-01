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

    Item {
        id: tempAvatarPageContainer

        GridLayout {
            id: tempAvatarPageGrid
            anchors.fill: parent
            anchors.leftMargin: 180
            anchors.bottomMargin: 50
            anchors.rightMargin: 100
            columns: 2

            Item {
                id: textAndQRContainer
                Layout.preferredWidth: 650
                Layout.preferredHeight: 670
                Layout.minimumWidth: 200
                Layout.maximumWidth: 800
                Layout.topMargin: 180

                HifiStylesUit.RalewayBold {
                    id: headerText
                    text: "Stand out from the crowd!"
                    color: simplifiedUI.colors.darkGray
                    size: 48
                    wrapMode: Text.WordWrap
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 60
                }

                HifiStylesUit.RalewayRegular {
                    id: descriptionText
                    width: 600
                    anchors.top: headerText.bottom
                    anchors.topMargin: 10
                    text: "You can create and upload custom avatars from our Avatar Creator App. It's as easy as taking a selfie. Available now on iOS and Android Platforms."
                    color: simplifiedUI.colors.darkGray
                    size: 24
                    anchors.left: parent.left
                    height: 80
                    wrapMode: Text.WordWrap
                }

                Item {
                    id: qrAndInstructionsContainer
                    anchors.top: descriptionText.bottom
                    anchors.left: parent.left
                    height: avatarAppQRCodeImage.height + instructionText.height + 50
                    width: descriptionText.width
                    anchors.topMargin: 60

                    Image {
                        id: avatarAppQRCodeImage
                        anchors.horizontalCenter: instructionText.horizontalCenter
                        source: resourceDirectoryUrl + "qml/hifi/simplifiedUI/avatarApp/images/qrCode.jpg"
                        height: 220
                        width: 220
                    }

                    HifiStylesUit.RalewayBold {
                        id: instructionText
                        anchors.left: parent.left
                        anchors.topMargin: 10
                        width: 515
                        anchors.top: avatarAppQRCodeImage.bottom
                        text: "Use your mobile phone to scan this QR code."
                        color: simplifiedUI.colors.darkGray
                        size: 24
                        height: 30
                        wrapMode: Text.WordWrap
                    }
                }

                HifiStylesUit.RalewayBold {
                    anchors.top: qrAndInstructionsContainer.bottom
                    anchors.topMargin: 50
                    anchors.horizontalCenter: qrAndInstructionsContainer.horizontalCenter
                    text: "No thanks, I'll keep using my default avatar."
                    color: simplifiedUI.colors.text.lightBlue
                    opacity: continueMouseArea.containsMouse ? 1.0 : 0.7
                    size: 14
                    z: 1
                    wrapMode: Text.WordWrap

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
            }

            Item {
                id: heroImageContainer
                Layout.leftMargin: 30
                Layout.preferredWidth: heroImage.width
                Layout.preferredHeight: heroImage.height

                Image {
                    id: heroImage
                    width: 428
                    height: 800
                    source: resourceDirectoryUrl + "qml/hifi/simplifiedUI/avatarApp/images/hero.png"
                }
                // TODO move this to be above the rest of the grid layout stuff in landscape mode
            }
        }

        SimplifiedControls.VerticalScrollBar {
            parent: root
        }
    }

    signal sendToScript(var message);
}
