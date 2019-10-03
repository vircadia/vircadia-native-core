//
//  InitialLaunchWindow.qml
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
    property bool landscapeOrientation: root.width > root.height

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    Component.onCompleted: {
        if (Settings.getValue("simplifiedUI/alreadyAutoSelectedAvatarFromInventory", false) || 
            Settings.getValue("simplifiedUI/closedAvatarPageOfInitialLaunchWindow", false)) {
            tempAvatarPageContainer.visible = false;
            controlsContainer.visible = true;
        }
    }

    Image {
        id: topLeftAccentImage
        width: 400
        height: 180
        anchors.left: parent.left
        anchors.top: parent.top
        source: "images/defaultTopLeft.png"
        z: 1
    }

    Image {
        id: bottomRightAccentImage
        width: 80
        height: 250
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: "images/defaultBottomRight.png"
        z: 1
    }

    Item {
        id: tempAvatarPageContainer
        visible: true

        GridLayout {
            id: tempAvatarPageGrid
            anchors.fill: parent
            anchors.leftMargin: 50
            anchors.topMargin: 50
            anchors.rightMargin: 100
            columns: landscapeOrientation ? 2 : 1
            rows: landscapeOrientation ? 1 : 2

            Item {
                id: tempAvatarImageContainer
                Layout.preferredWidth: tempAvatarImage.width
                Layout.preferredHeight: tempAvatarImage.height

                Image {
                    id: tempAvatarImage
                    width: landscapeOrientation ? 428 : 320
                    height: landscapeOrientation ? 800 : 598
                    source: resourceDirectoryUrl + "qml/hifi/simplifiedUI/avatarApp/images/" +
                        MyAvatar.skeletonModelURL.substring(MyAvatar.skeletonModelURL.indexOf("simplifiedAvatar"), MyAvatar.skeletonModelURL.lastIndexOf("/")) + ".png"
                }
            }

            Item {
                id: textAndQRContainer
                Layout.preferredWidth: 680
                Layout.preferredHeight: childrenRect.height
                Layout.topMargin: landscapeOrientation ? 130 : 0

                HifiStylesUit.RalewayBold {
                    id: headerText
                    text: "We know this isn't you..."
                    color: simplifiedUI.colors.text.black
                    size: 48
                    wrapMode: Text.WordWrap
                    anchors.left: parent.left
                    anchors.right: parent.right
                }

                HifiStylesUit.RalewayRegular {
                    id: descriptionText
                    anchors.top: headerText.bottom
                    anchors.topMargin: 10
                    text: "but, we've given you this <b>temporary avatar</b> to use
                    for today. If you see this avatar in-world, walk up and
                    say hello to other new users!<br></br><br></br>
                    <b>We want you to be you</b> so we've built an Avatar Creator
                    App that's as easy as taking a selfie and picking your
                    outfits! Available now on iOS and Android Platforms."
                    color: simplifiedUI.colors.text.black
                    size: 22
                    anchors.left: parent.left
                    anchors.right: parent.right
                    wrapMode: Text.WordWrap
                }

                Item {
                    id: qrAndInstructionsContainer
                    anchors.top: descriptionText.bottom
                    height: avatarAppQRCodeImage.height
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.topMargin: 50

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
                        anchors.left: avatarAppQRCodeImage.right
                        anchors.top: avatarAppQRCodeImage.top
                        anchors.bottom: avatarAppQRCodeImage.bottom
                        anchors.right: parent.right
                        anchors.leftMargin: 30
                        text: "Use your mobile phone to scan this QR code."
                        color: simplifiedUI.colors.text.black
                        size: 22
                        wrapMode: Text.WordWrap
                    }
                }

                HifiStylesUit.RalewayBold {
                    anchors.top: qrAndInstructionsContainer.bottom
                    anchors.topMargin: 50
                    anchors.left: parent.left
                    anchors.right: parent.right
                    horizontalAlignment: Text.AlignLeft
                    text: "Continue >"
                    color: simplifiedUI.colors.text.lightBlue
                    opacity: continueMouseArea.containsMouse ? 1.0 : 0.7
                    size: 36
                    z: 1
                    wrapMode: Text.WordWrap

                    MouseArea {
                        id: continueMouseArea
                        hoverEnabled: true
                        anchors.fill: parent

                        onClicked: {
                            Tablet.playSound(TabletEnums.ButtonClick);
                            tempAvatarPageContainer.visible = false;
                            Settings.setValue("simplifiedUI/closedAvatarPageOfInitialLaunchWindow", true);
                            controlsContainer.visible = true;
                        }
                    }
                }
            }
        }

        SimplifiedControls.VerticalScrollBar {
            parent: tempAvatarPageContainer
        }
    }

    Item {
        id: controlsContainer
        visible: false
        anchors.fill: parent

        SimplifiedControls.VerticalScrollBar {
            parent: tempAvatarPageContainer
        }

        HifiStylesUit.RalewayRegular {
            id: controlsDescriptionText
            text: "These are your avatar controls to <br></br>
                <b>interact with and move around in your new HQ.</b>"
            anchors.top: parent.top
            anchors.topMargin: 100
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            height: 100
            width: 850
            color: simplifiedUI.colors.text.black
            size: 36
            wrapMode: Text.WordWrap
        }

        GridLayout {
            id: controlsPageGrid
            anchors.top: controlsDescriptionText.bottom
            anchors.topMargin: 60
            anchors.bottomMargin: 80
            anchors.horizontalCenter: parent.horizontalCenter
            columnSpacing: 50
            rowSpacing: 40
            columns: 2
            rows: 2
          
            Image {
                Layout.preferredWidth: 360
                Layout.preferredHeight: 225
                id: walkingControls
                source: "images/walkingControls.png"
            }
              
            Image {
                Layout.preferredWidth: 360
                Layout.preferredHeight: 200
                id: mouseControls
                source: "images/mouseControls.png"
            }

            Image {
                Layout.preferredWidth: 240
                Layout.preferredHeight: 160
                Layout.alignment: Qt.AlignHCenter
                id: runJumpControls
                source: "images/runJumpControls.png"
            }

            Image {
                Layout.preferredWidth: 160
                Layout.preferredHeight: 100
                Layout.alignment: Qt.AlignHCenter
                id: cameraControls
                source: "images/cameraControls.png"
            }
        }

        HifiStylesUit.RalewayBold {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 50
            anchors.bottomMargin: 50
            text: "Learn more about our controls."
            width: 200
            height: 25
            color: simplifiedUI.colors.text.lightBlue
            opacity: learnMoreAboutControlsMouseArea.containsMouse ? 1.0 : 0.7
            size: 14
            wrapMode: Text.WordWrap

            MouseArea {
                id: learnMoreAboutControlsMouseArea
                hoverEnabled: true
                anchors.fill: parent

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    Qt.openUrlExternally("https://www.highfidelity.com/knowledge/get-around");
                }
            }
        }

        HifiStylesUit.RalewayBold {
            anchors.top: controlsPageGrid.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 50
            text: "I've got a good grip on the controls."
            width: 650
            height: 60
            color: simplifiedUI.colors.text.lightBlue
            opacity: goodGripMouseArea.containsMouse ? 1.0 : 0.7
            size: 36
            wrapMode: Text.WordWrap

            MouseArea {
                id: goodGripMouseArea
                hoverEnabled: true
                anchors.fill: parent

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    sendToScript({
                        "source": "InitialLaunchWindow.qml",
                        "method": "closeInitialLaunchWindow"
                    });
                }
            }
        }
    }

    signal sendToScript(var message);
}
