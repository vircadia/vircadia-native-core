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
    color: "#ffffff"
    anchors.fill: parent

    Image {
        id: topLeftAccentImage
        width: 180
        height: 400
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
            anchors.topMargin: 50
            anchors.bottomMargin: 50
            anchors.rightMargin: 100
            columns: 2
            // flow: root.width < root.height ? GridLayout.LeftToRight : GridLayout.TopToBottom

            Item {
                id: textAndQRContainer
                Layout.preferredWidth: 650
                Layout.preferredHeight: 670
                Layout.minimumWidth: 200
                Layout.maximumWidth: 800
                Layout.topMargin: 80

                HifiStylesUit.RalewayBold {
                    id: headerText
                    text: "Stand out from the crowd!"
                    color: "#000000"
                    size: 48
                    wrapMode: Text.WordWrap
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 80
                }

                HifiStylesUit.RalewaySemiBold {
                    id: descriptionText
                    anchors.top: headerText.bottom
                    text: "You can create and upload custom avatars from our Avatar Creator App. It's as easy as taking a selfie. Available now on iOS and Android Platforms."
                    color: "#000000"
                    size: 24
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 100
                    wrapMode: Text.WordWrap
                }

                Item {
                    id: qrAndInstructionsContainer
                    anchors.top: descriptionText.bottom
                    height: avatarAppQRCodeImage.height + instructionText.height + 50
                    width: parent.width
                    anchors.topMargin: 50

                    Image {
                        id: avatarAppQRCodeImage
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: "images/qrCode.jpg"
                        height: 200
                        width: 200
                    }

                    HifiStylesUit.RalewayBold {
                        id: instructionText
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: avatarAppQRCodeImage.bottom
                        anchors.horizontalCenter: avatarAppQRCodeImage.horizontalCenter
                        text: "Use your mobile phone to scan this QR code."
                        color: "#000000"
                        size: 24
                        height: 60
                        wrapMode: Text.WordWrap
                    }
                }

                HifiStylesUit.RalewayBold {
                    anchors.top: qrAndInstructionsContainer.bottom
                    anchors.topMargin: 50
                    anchors.horizontalCenter: qrAndInstructionsContainer.horizontalCenter
                    text: "No thanks, I'll keep using my default avatar."
                    color: "#000000"
                    opacity: continueMouseArea.containsMouse ? 1.0 : 0.7
                    size: 20
                    z: 1
                    wrapMode: Text.WordWrap

                    MouseArea {
                        id: continueMouseArea
                        hoverEnabled: true
                        anchors.fill: parent

                        onClicked: {
                            Tablet.playSound(TabletEnums.ButtonClick);
                            print("NO THANKS CLICKED");
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
                Layout.leftMargin: 50
                // these don't change when the window resizes
                Layout.preferredWidth: heroImage.width
                Layout.preferredHeight: heroImage.height

                Image {
                    id: heroImage
                    // if I use preferred width and height, the image does not update when window changes size
                    // width: tempAvatarPageGrid.flow === GridLayout.LeftToRight ? 400 : 100
                    // height: tempAvatarPageGrid.flow === GridLayout.LeftToRight ? 748 : 187
                    width: 400
                    height: 748
                    source: "images/hero.png"
                }
            }
        }
    }

    signal sendToScript(var message);
}
