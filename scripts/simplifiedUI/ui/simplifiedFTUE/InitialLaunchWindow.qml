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
    // cannot get anything from SimplifiedConstants to work: simplifiedUI.colors.white 
    // TODO figure out why and fix
    color: simplifiedUI.colors.white
    anchors.fill: parent

    Component.onCompleted: {
        if (Settings.getValue("simplifiedUI/alreadyAutoSelectedAvatar", false)) {
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
            anchors.leftMargin: 180
            anchors.topMargin: 50
            anchors.bottomMargin: 50
            anchors.rightMargin: 100
            columns: 2
            // flow: root.width > root.height ? GridLayout.LeftToRight : GridLayout.TopToBottom

            Item {
                id: textAndQRContainer
                Layout.preferredWidth: 600
                Layout.preferredHeight: 670
                Layout.minimumWidth: 200
                Layout.maximumWidth: 800
                Layout.topMargin: 80

                HifiStylesUit.RalewayBold {
                    id: headerText
                    text: "We know this isn't you..."
                    color: "#000000"
                    size: 48
                    wrapMode: Text.WordWrap
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 100
                }

                HifiStylesUit.RalewaySemiBold {
                    id: descriptionText
                    anchors.top: headerText.bottom
                    anchors.topMargin: 20
                    text: "but, we've given you this <b>temporary avatar</b> to use
                    for today. If you see this avatar in-world, walk up and
                    say hello to other new users!<br></br><br></br>
                    <b>We want you to be you</b> so we've built an Avatar Creator
                    App that's as easy as taking a selfie and picking your
                    outfits! Available now on iOS and Android Platforms."
                    color: "#000000"
                    size: 24
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 300
                    wrapMode: Text.WordWrap
                }

                Item {
                    id: qrAndInstructionsContainer
                    anchors.top: descriptionText.bottom
                    height: avatarAppQRCodeImage.height
                    width: parent.width
                    anchors.topMargin: 50

                    Image {
                        id: avatarAppQRCodeImage
                        source: "images/qrCode.jpg"
                        height: 200
                        width: 200
                    }

                    HifiStylesUit.RalewayBold {
                        id: instructionText
                        anchors.left: avatarAppQRCodeImage.right
                        anchors.top: avatarAppQRCodeImage.top
                        anchors.bottom: avatarAppQRCodeImage.bottom
                        anchors.right: parent.right
                        anchors.verticalCenter: avatarAppQRCodeImage.verticalCenter
                        anchors.leftMargin: 50
                        text: "Use your mobile phone to scan this QR code."
                        color: "#000000"
                        size: 24
                        wrapMode: Text.WordWrap
                    }
                }

                HifiStylesUit.RalewayBold {
                    anchors.top: qrAndInstructionsContainer.bottom
                    anchors.topMargin: 50
                    anchors.horizontalCenter: qrAndInstructionsContainer.horizontalCenter
                    text: "Continue"
                    color: "#000000"
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
                            controlsContainer.visible = true;
                        }
                    }
                }
            }

            Item {
                id: tempAvatarImageContainer
                Layout.leftMargin: 50
                // TODO these don't change when the window resizes...still true?
                Layout.preferredWidth: tempAvatarImage.width
                Layout.preferredHeight: tempAvatarImage.height

                Image {
                    id: tempAvatarImage
                    // if I use preferred width and height, the image does not update when window changes size
                    // width: tempAvatarPageGrid.flow === GridLayout.LeftToRight ? 400 : 100
                    // height: tempAvatarPageGrid.flow === GridLayout.LeftToRight ? 748 : 187
                    width: 400
                    height: 748
                    source: "images/DefaultAvatar_" + MyAvatar.skeletonModelURL.substring(123, MyAvatar.skeletonModelURL.length - 11) + ".png"
                }
            }
        }
    }

    Item {
        id: controlsContainer
        visible: false
        anchors.fill: parent

        HifiStylesUit.RalewaySemiBold {
            id: controlsDescriptionText
            text: "These are your avatar controls to <br></br>
                <b>interact with and move around in your new HQ.</b>"
            anchors.top: parent.top
            anchors.topMargin: 100
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            height: 100
            width: 850
            color: "#000000"
            size: 36
            wrapMode: Text.WordWrap
        }

        GridLayout {
            id: controlsPageGrid
            anchors.top: controlsDescriptionText.bottom
            anchors.topMargin: 60
            anchors.bottomMargin: 80
            anchors.horizontalCenter: parent.horizontalCenter
            columns: 2
            columnSpacing: 50
            rowSpacing: 40
            // flow: root.width > root.height ? GridLayout.LeftToRight : GridLayout.TopToBottom
          
            Image {
                Layout.preferredWidth: 450
                Layout.preferredHeight: 280
                id: walkingControls
                source: "images/walkingControls.png"
            }
              
            Image {
                Layout.preferredWidth: 450
                Layout.preferredHeight: 240
                id: mouseControls
                source: "images/mouseControls.png"
            }

            Image {
                Layout.preferredWidth: 300
                Layout.preferredHeight: 200
                // TODO Why is this not aligning correctly? (Same at line 235)
                Layout.alignment: AlignHCenter
                id: runJumpControls
                source: "images/runJumpControls.png"
            }

            Image {
                Layout.preferredWidth: 200
                Layout.preferredHeight: 120
                Layout.alignment: AlignHCenter
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
            color: "#000000"
            opacity: learnMoreAboutControlsMouseArea.containsMouse ? 1.0 : 0.7
            size: 14
            wrapMode: Text.WordWrap

            MouseArea {
                id: learnMoreAboutControlsMouseArea
                hoverEnabled: true
                anchors.fill: parent

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    // TODO open docs in browser
                    Print("LEARN MORE ABOUT CONTROLS CLICKED");
                }
            }
        }

        HifiStylesUit.RalewayBold {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 50
            text: "I've got a good grip on the controls."
            width: 650
            height: 60
            color: "#000000"
            opacity: goodGripMouseArea.containsMouse ? 1.0 : 0.7
            size: 36
            wrapMode: Text.WordWrap

            MouseArea {
                id: goodGripMouseArea
                hoverEnabled: true
                anchors.fill: parent

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    print("GOOD GRIP CLICKED");
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
