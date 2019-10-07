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

    Item {
        id: tempAvatarPageContainer
        anchors.fill: parent

        Flickable {
            id: tempAvatarPageFlickable
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.bottom: continueLink.top
            clip: true
            contentWidth: parent.width
            contentHeight: tempAvatarPageGrid.heights
            interactive: contentHeight > height

            RowLayout {
                id: tempAvatarPageGrid
                width: parent.width

                Image {
                    Layout.preferredWidth: 428
                    Layout.minimumHeight: 240
                    Layout.maximumHeight: 800
                    Layout.fillHeight: true
                    source: resourceDirectoryUrl + "qml/hifi/simplifiedUI/avatarApp/images/" +
                        MyAvatar.skeletonModelURL.substring(MyAvatar.skeletonModelURL.indexOf("simplifiedAvatar"), MyAvatar.skeletonModelURL.lastIndexOf("/")) + ".png"
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
                        text: "We know this isn't you..."
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
                        text: "but we've given you this <b>temporary avatar</b> to use " +
                            "for today. If you see this avatar in-world, walk up and " +
                            "say hello to other new users!<br><br>" +
                            "<b>We want you to be you</b> so we've built an Avatar Creator " +
                            "App that's as easy as taking a selfie and picking your " +
                            "outfits! Available now on iOS and Android."
                        color: simplifiedUI.colors.text.black
                        size: 22
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
                            size: 22
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
        

        HifiStylesUit.RalewayBold {
            id: continueLink
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: parent.right
            anchors.rightMargin: 16
            height: 96
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: "Continue >"
            color: simplifiedUI.colors.text.lightBlue
            opacity: continueMouseArea.containsMouse ? 1.0 : 0.7
            size: 36

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

        SimplifiedControls.VerticalScrollBar {
            parent: tempAvatarPageFlickable
        }
    }

    Item {
        id: controlsContainer
        visible: false
        anchors.fill: parent

        HifiStylesUit.RalewayRegular {
            id: controlsDescriptionText
            text: "Use these avatar controls to <b>interact with and move around in your new HQ.</b>"
            anchors.top: parent.top
            anchors.topMargin: 48
            anchors.left: parent.left
            anchors.leftMargin: 32
            anchors.right: parent.right
            anchors.rightMargin: 32
            horizontalAlignment: Text.AlignHCenter
            height: paintedHeight
            color: simplifiedUI.colors.text.black
            size: 36
            wrapMode: Text.Wrap
        }

        Item {
            anchors.top: controlsDescriptionText.bottom
            anchors.topMargin: 16
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: bottomBarContainer.top

            GridView {
                id: controlsGrid
                property int maxColumns: 2
                property int idealCellWidth: 361
                anchors.fill: parent
                clip: true
                cellWidth: width / Math.min(Math.floor(width / idealCellWidth), maxColumns)
                cellHeight: 225
                model: ListModel {
                    ListElement {
                        imageHeight: 198
                        imageSource: "images/walkingControls.png"
                    }
                    ListElement {
                        imageHeight: 193
                        imageSource: "images/mouseControls.png"
                    }
                    ListElement {
                        imageHeight: 146
                        imageSource: "images/runJumpControls.png"
                    }
                    ListElement {
                        imageHeight: 96
                        imageSource: "images/cameraControls.png"
                    }
                }
                delegate: Rectangle {
                    height: GridView.view.cellHeight
                    width: GridView.view.cellWidth
                    Image {
                        anchors.centerIn: parent
                        width: parent.GridView.view.idealCellWidth
                        height: model.imageHeight
                        source: model.imageSource
                        fillMode: Image.PreserveAspectFit
                    }
                }
            }

            SimplifiedControls.VerticalScrollBar {
                parent: controlsGrid
                anchors.topMargin: 96
                anchors.bottomMargin: anchors.topMargin
            }
        }

        Item {
            id: bottomBarContainer
            anchors.left: parent.left
            anchors.leftMargin: 32
            anchors.right: parent.right
            anchors.rightMargin: 32
            anchors.bottom: parent.bottom
            height: iHaveAGoodGrip.height + learnMoreLink.height + 48

            HifiStylesUit.RalewayBold {
                id: iHaveAGoodGrip
                anchors.centerIn: parent
                text: "I've got a good grip on the controls."
                width: parent.width
                height: paintedHeight
                color: simplifiedUI.colors.text.lightBlue
                opacity: goodGripMouseArea.containsMouse ? 1.0 : 0.7
                size: 36
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter

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
        
            HifiStylesUit.RalewayBold {
                id: learnMoreLink
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.top: iHaveAGoodGrip.bottom
                anchors.topMargin: 8
                text: "Learn more about our controls."
                width: paintedWidth
                height: paintedHeight
                color: simplifiedUI.colors.text.lightBlue
                opacity: learnMoreAboutControlsMouseArea.containsMouse ? 1.0 : 0.7
                size: 14
                wrapMode: Text.Wrap

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
        }
    }

    Image {
        id: topLeftAccentImage
        width: 400
        height: 180
        anchors.left: parent.left
        anchors.top: parent.top
        source: "images/defaultTopLeft.png"
    }

    Image {
        id: bottomRightAccentImage
        width: 80
        height: 250
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: "images/defaultBottomRight.png"
    }

    signal sendToScript(var message);
}
